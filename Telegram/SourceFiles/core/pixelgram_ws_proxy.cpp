/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/pixelgram_ws_proxy.h"

#include "core/websocket_client.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "storage/localstorage.h"
#include "settings/pixelgram_settings.h"
#include "mtproto/mtproto_auth_key.h"
#include "mtproto/mtproto_proxy_data.h"
#include "base/bytes.h"
#include "base/random.h"
#include <crl/crl_on_main.h>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <rpl/variable.h>

#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <atomic>

namespace PixelGram {
namespace WsProxy {
namespace {

std::atomic<uint64> g_sessionCounter = 0;
rpl::variable<QString> g_status = u"Не активен"_q;

void LogProxy(const QString &msg) {
	const auto line = QDateTime::currentDateTime().toString(u"[yyyy.MM.dd hh:mm:ss.zzz] "_q) + msg + u"\n"_q;
	DEBUG_LOG(("WsProxy: %1").arg(msg));

	const QString paths[] = {
		cWorkingDir() + u"tdata/ws_proxy.log"_q,
		cWorkingDir() + u"ws_proxy.log"_q,
	};
	for (const auto &path : paths) {
		QFile f(path);
		if (f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
			f.write(line.toUtf8());
			f.flush();
		}
	}
}

QString EnsureSecret() {
	auto &cfg = CurrentConfig();
	const auto isBadSecret = (cfg.wsProxySecret.size() != 32)
		|| (cfg.wsProxySecret == u"00000000000000000000000000000001"_q)
		|| (cfg.wsProxySecret == u"00000000000000000000000000000000"_q);
	if (isBadSecret) {
		cfg.wsProxySecret = GenerateRandomSecret();
		SaveConfig();
	}
	return cfg.wsProxySecret;
}

// 20 Cloudflare proxy domains from Flowseal/tg-ws-proxy
static const QStringList kCfDomains = {
	u"pclead.co.uk"_q,
	u"offshor.co.uk"_q,
	u"cakeisalie.co.uk"_q,
	u"noskomnadzor.co.uk"_q,
	u"lovetrue.co.uk"_q,
	u"sorokdva.co.uk"_q,
	u"pyatdesyatdva.co.uk"_q,
	u"kartoshka.co.uk"_q,
	u"sorokodin.co.uk"_q,
	u"pyatdesyatodin.co.uk"_q,
	u"notelega.co.uk"_q,
	u"ebally.co.uk"_q,
	u"nebally.co.uk"_q,
	u"havegreatday.co.uk"_q,
	u"pomogite.co.uk"_q,
	u"fixtelega.co.uk"_q,
	u"sadnews.co.uk"_q,
	u"onedaychamp.co.uk"_q,
	u"stopblocking.co.uk"_q,
	u"nothingthere.co.uk"_q,
};

QString GetWsDomain(int dc, int mirrorIndex) {
	if (mirrorIndex < kCfDomains.size()) {
		return u"kws%1.%2"_q.arg(dc).arg(kCfDomains[mirrorIndex]);
	}
	return u"kws%1.web.telegram.org"_q.arg(dc);
}

struct RelayInitResult {
	QByteArray relayInit;        // 64 bytes
	QByteArray relayEncKey;      // 32 bytes
	MTP::CTRState relayEncState; // counter at 64
	QByteArray relayDecKey;      // 32 bytes
	MTP::CTRState relayDecState; // counter at 0
};

RelayInitResult GenerateRelayInit(uint32 protoTag, int16 dcIdx) {
	QByteArray rnd(64, Qt::Uninitialized);
	while (true) {
		base::RandomFill(rnd.data(), 64);
		const auto first = uchar(rnd[0]);
		if (first == 0xEF) continue;
		const auto start = *reinterpret_cast<const uint32*>(rnd.constData());
		if (start == 0x44414548 || start == 0x54534F50 || start == 0x20544547
			|| start == 0xEEEEEEEE || start == 0xDDDDDDDD) {
			continue;
		}
		const auto cont = *reinterpret_cast<const uint32*>(rnd.constData() + 4);
		if (cont == 0) continue;
		break;
	}

	RelayInitResult res;
	res.relayEncKey = rnd.mid(8, 32);
	std::memcpy(res.relayEncState.ivec, rnd.constData() + 40, 16);
	res.relayEncState.num = 0;
	std::memset(res.relayEncState.ecount, 0, 16);

	QByteArray reversed = rnd.mid(8, 48);
	std::reverse(reversed.begin(), reversed.end());
	res.relayDecKey = reversed.left(32);
	std::memcpy(res.relayDecState.ivec, reversed.constData() + 32, 16);
	res.relayDecState.num = 0;
	std::memset(res.relayDecState.ecount, 0, 16);

	QByteArray dummy(64, '\0');
	std::memcpy(dummy.data() + 56, &protoTag, 4);
	std::memcpy(dummy.data() + 60, &dcIdx, 2);
	base::RandomFill(dummy.data() + 62, 2);

	res.relayInit = rnd;
	MTP::aesCtrEncrypt(bytes::make_span(dummy.data(), dummy.size()), res.relayEncKey.constData(), &res.relayEncState);
	res.relayInit.replace(56, 8, dummy.mid(56, 8));

	return res;
}

// 1-to-1 port of Flowseal/tg-ws-proxy MsgSplitter
class MsgSplitter {
public:
	explicit MsgSplitter(uint32 protoTag)
	: _proto(protoTag) {
	}

	std::vector<QByteArray> split(const QByteArray &chunk, const QByteArray &plain) {
		if (chunk.isEmpty()) return {};
		if (_disabled) return { chunk };

		_cipherBuf.append(chunk);
		_plainBuf.append(plain);

		std::vector<QByteArray> parts;
		int offset = 0;
		const int bufLen = _cipherBuf.size();
		while (offset < bufLen) {
			const int packetLen = nextPacketLen(offset, bufLen - offset);
			if (packetLen < 0) {
				break;
			}
			if (packetLen == 0) {
				parts.push_back(offset == 0 ? _cipherBuf : _cipherBuf.mid(offset));
				offset = bufLen;
				_disabled = true;
				break;
			}
			parts.push_back(_cipherBuf.mid(offset, packetLen));
			offset += packetLen;
		}

		if (offset > 0) {
			_cipherBuf.remove(0, offset);
			_plainBuf.remove(0, offset);
		}
		return parts;
	}

	std::vector<QByteArray> flush() {
		if (_cipherBuf.isEmpty()) return {};
		std::vector<QByteArray> parts = { _cipherBuf };
		_cipherBuf.clear();
		_plainBuf.clear();
		return parts;
	}

private:
	int nextPacketLen(int offset, int avail) const {
		if (avail <= 0) return -1;
		if (_proto == 0xEFEFEFEFU) {
			return nextAbridgedLen(offset, avail);
		}
		if (_proto == 0xEEEEEEEEU || _proto == 0xDDDDDDDDU) {
			return nextIntermediateLen(offset, avail);
		}
		return 0;
	}

	int nextAbridgedLen(int offset, int avail) const {
		const auto first = uchar(_plainBuf[offset]);
		int headerLen = 0;
		int payloadLen = 0;
		if (first == 0x7F || first == 0xFF) {
			if (avail < 4) return -1;
			uint32 raw = uint32(uchar(_plainBuf[offset + 1]))
				| (uint32(uchar(_plainBuf[offset + 2])) << 8)
				| (uint32(uchar(_plainBuf[offset + 3])) << 16);
			payloadLen = int(raw * 4);
			headerLen = 4;
		} else {
			payloadLen = int((first & 0x7F) * 4);
			headerLen = 1;
		}
		if (payloadLen <= 0) return 0;
		const int packetLen = headerLen + payloadLen;
		if (avail < packetLen) return -1;
		return packetLen;
	}

	int nextIntermediateLen(int offset, int avail) const {
		if (avail < 4) return -1;
		uint32 raw = 0;
		std::memcpy(&raw, _plainBuf.constData() + offset, 4);
		const auto payloadLen = int(raw & 0x7FFFFFFFU);
		if (payloadLen <= 0) return 0;
		const int packetLen = 4 + payloadLen;
		if (avail < packetLen) return -1;
		return packetLen;
	}

	uint32 _proto = 0;
	QByteArray _cipherBuf;
	QByteArray _plainBuf;
	bool _disabled = false;
};

class WsProxySession final : public QObject {
public:
	WsProxySession(QTcpSocket *clientSocket, QObject *parent)
	: QObject(parent)
	, _id(++g_sessionCounter)
	, _client(clientSocket) {
		LogProxy(u"[%1] Incoming TCP connection from client"_q.arg(_id));

		if (_client) {
			_client->setParent(this);
			connect(_client, &QTcpSocket::readyRead, this, [this] {
				onClientReadyRead();
			});
			connect(_client, &QTcpSocket::disconnected, this, [this] {
				LogProxy(u"[%1] Client TCP disconnected"_q.arg(_id));
				close();
			});
			connect(
				_client,
				&QAbstractSocket::errorOccurred,
				this,
				[this](QAbstractSocket::SocketError err) {
					LogProxy(u"[%1] Client TCP socket error: %2"_q.arg(_id).arg(_client ? _client->errorString() : QString::number(int(err))));
					close();
				});
		}
	}

	~WsProxySession() {
		close();
	}

	void close() {
		if (_closed) return;
		_closed = true;
		_state = State::Closed;

		LogProxy(u"[%1] Session finished. Total stats: client->ws %2 pkts (%3 bytes), ws->client %4 pkts (%5 bytes)"_q
			.arg(_id)
			.arg(_upPackets)
			.arg(_upBytes)
			.arg(_downPackets)
			.arg(_downBytes));

		if (_client) {
			_client->disconnect(this);
			_client->disconnectFromHost();
			_client->deleteLater();
			_client = nullptr;
		}
		if (_ws) {
			auto oldWs = _ws.release();
			oldWs->onClosed = nullptr;
			oldWs->onBinary = nullptr;
			oldWs->onConnected = nullptr;
			oldWs->onError = nullptr;
			oldWs->close();
			oldWs->deleteLater();
		}
		this->deleteLater();
	}

private:
	enum class State {
		Handshake,
		Streaming,
		Closed,
	};

	void onClientReadyRead() {
		if (_closed || !_client) return;
		_clientBuffer.append(_client->readAll());

		if (_state == State::Handshake) {
			if (_clientBuffer.size() < 64) {
				return;
			}
			const auto handshake = _clientBuffer.left(64);
			_clientBuffer.remove(0, 64);

			const auto prekey = handshake.mid(8, 32);
			const auto iv = handshake.mid(40, 16);
			const auto sec = EnsureSecret();
			const auto secretBytes = QByteArray::fromHex(sec.toLatin1());

			LogProxy(u"[%1] 64-byte handshake received. Decoding with secret dd%2..."_q.arg(_id).arg(sec));

			_cltDecKey = QCryptographicHash::hash(prekey + secretBytes, QCryptographicHash::Sha256);
			std::memcpy(_cltDecState.ivec, iv.constData(), 16);
			_cltDecState.num = 0;
			std::memset(_cltDecState.ecount, 0, 16);

			auto decrypted = handshake;
			MTP::aesCtrEncrypt(bytes::make_span(decrypted.data(), decrypted.size()), _cltDecKey.constData(), &_cltDecState);

			uint32 protoTag = 0;
			std::memcpy(&protoTag, decrypted.constData() + 56, 4);
			int16 dcIdx = 0;
			std::memcpy(&dcIdx, decrypted.constData() + 60, 2);

			const bool valid = (protoTag == 0xDDDDDDDDU || protoTag == 0xEEEEEEEEU || protoTag == 0xEFEFEFEFU);
			_targetDc = (dcIdx != 0) ? std::abs(int(dcIdx)) : 2;
			if (_targetDc < 1 || _targetDc > 5) {
				_targetDc = 2;
			}
			_isMedia = (dcIdx < 0);

			LogProxy(u"[%1] Handshake decoded: protoTag=0x%2, dcIdx=%3 (DC%4%5), valid=%6"_q
				.arg(_id)
				.arg(protoTag, 8, 16, QChar('0'))
				.arg(dcIdx)
				.arg(_targetDc)
				.arg(_isMedia ? u" media"_q : QString())
				.arg(valid ? u"yes"_q : u"no (INVALID SECRET / PROTO)"_q));

			if (!valid) {
				close();
				return;
			}
			_protoTag = protoTag;
			_splitter = std::make_unique<MsgSplitter>(_protoTag);

			// Setup client encryptor (reversed nonce[8..56])
			QByteArray reversed = handshake.mid(8, 48);
			std::reverse(reversed.begin(), reversed.end());
			_cltEncKey = QCryptographicHash::hash(reversed.left(32) + secretBytes, QCryptographicHash::Sha256);
			std::memcpy(_cltEncState.ivec, reversed.constData() + 32, 16);
			_cltEncState.num = 0;
			std::memset(_cltEncState.ecount, 0, 16);

			// Generate relay init for Telegram upstream
			auto relay = GenerateRelayInit(_protoTag, dcIdx);
			_relayInit = relay.relayInit;
			_relayEncKey = relay.relayEncKey;
			_relayEncState = relay.relayEncState;
			_relayDecKey = relay.relayDecKey;
			_relayDecState = relay.relayDecState;

			_state = State::Streaming;
			startWebSocket();
		}

		if (_state == State::Streaming) {
			if (_clientBuffer.isEmpty()) return;
			const auto chunk = _clientBuffer;
			_clientBuffer.clear();

			auto plain = chunk;
			MTP::aesCtrEncrypt(bytes::make_span(plain.data(), plain.size()), _cltDecKey.constData(), &_cltDecState);

			auto cipher = plain;
			MTP::aesCtrEncrypt(bytes::make_span(cipher.data(), cipher.size()), _relayEncKey.constData(), &_relayEncState);

			const auto parts = _splitter ? _splitter->split(cipher, plain) : std::vector<QByteArray>{ cipher };
			for (const auto &part : parts) {
				if (_wsConnected && _ws) {
					_upPackets++;
					_upBytes += part.size();
					LogProxy(u"[%1] client -> ws: frame %2 bytes (total up: %3 pkts, %4 bytes)"_q
						.arg(_id).arg(part.size()).arg(_upPackets).arg(_upBytes));
					_ws->sendBinary(part);
				} else {
					_pendingFrames.append(part);
				}
			}
		}
	}

	void startWebSocket() {
		if (_closed) return;
		_ws = std::make_unique<Core::WebSocketClient>(this);
		const auto domain = GetWsDomain(_targetDc, _mirrorIndex);
		const auto disableTls = CurrentConfig().wsProxyDisableTls;
		const auto port = disableTls ? 80 : 443;
		const auto scheme = disableTls ? u"ws"_q : u"wss"_q;

		LogProxy(u"[%1] Connecting upstream WebSocket: %2://%3:%4/apiws (attempt %5)"_q
			.arg(_id)
			.arg(scheme)
			.arg(domain)
			.arg(port)
			.arg(_mirrorIndex + 1));

		_ws->onConnected = [this, domain] {
			if (_closed) return;
			_wsConnected = true;
			LogProxy(u"[%1] Upstream WebSocket connected to %2! 101 Switching Protocols. Sending relay_init (64 bytes)..."_q
				.arg(_id).arg(domain));

			if (_ws) {
				_ws->sendBinary(_relayInit);
				for (const auto &frame : _pendingFrames) {
					_upPackets++;
					_upBytes += frame.size();
					LogProxy(u"[%1] client -> ws (buffered): frame %2 bytes (total up: %3 pkts, %4 bytes)"_q
						.arg(_id).arg(frame.size()).arg(_upPackets).arg(_upBytes));
					_ws->sendBinary(frame);
				}
				_pendingFrames.clear();
			}
		};

		_ws->onBinary = [this](QByteArray data) {
			if (_closed || data.isEmpty()) return;
			// Decrypt from Telegram upstream
			MTP::aesCtrEncrypt(bytes::make_span(data.data(), data.size()), _relayDecKey.constData(), &_relayDecState);
			// Encrypt for tdesktop client
			MTP::aesCtrEncrypt(bytes::make_span(data.data(), data.size()), _cltEncKey.constData(), &_cltEncState);

			_downPackets++;
			_downBytes += data.size();
			LogProxy(u"[%1] ws -> client: frame %2 bytes (total down: %3 pkts, %4 bytes)"_q
				.arg(_id).arg(data.size()).arg(_downPackets).arg(_downBytes));

			if (_client && _client->isOpen()) {
				_client->write(data);
			}
		};

		_ws->onError = [this](QString err) {
			LogProxy(u"[%1] Upstream WebSocket error: %2"_q.arg(_id).arg(err));
		};

		_ws->onClosed = [this] {
			if (_closed) return;
			if (!_wsConnected && tryNextMirror()) {
				return;
			}
			LogProxy(u"[%1] Upstream WebSocket closed. Terminating session."_q.arg(_id));
			close();
		};

		_ws->connectTo(domain, port, u"/apiws"_q, u"binary"_q, !disableTls);
	}

	bool tryNextMirror() {
		if (_mirrorIndex < kCfDomains.size()) {
			_mirrorIndex++;
			LogProxy(u"[%1] Upstream mirror failed. Switching to mirror index %2..."_q.arg(_id).arg(_mirrorIndex));

			if (_ws) {
				auto oldWs = _ws.release();
				oldWs->onClosed = nullptr;
				oldWs->onBinary = nullptr;
				oldWs->onConnected = nullptr;
				oldWs->onError = nullptr;
				oldWs->close();
				oldWs->deleteLater();
			}
			crl::on_main(this, [this] {
				if (!_closed) {
					startWebSocket();
				}
			});
			return true;
		}
		return false;
	}

	uint64 _id = 0;
	QPointer<QTcpSocket> _client;
	std::unique_ptr<Core::WebSocketClient> _ws;
	std::unique_ptr<MsgSplitter> _splitter;
	QByteArray _clientBuffer;
	QList<QByteArray> _pendingFrames;
	QByteArray _relayInit;

	QByteArray _cltDecKey;
	MTP::CTRState _cltDecState;
	QByteArray _cltEncKey;
	MTP::CTRState _cltEncState;

	QByteArray _relayEncKey;
	MTP::CTRState _relayEncState;
	QByteArray _relayDecKey;
	MTP::CTRState _relayDecState;

	uint32 _protoTag = 0;
	int _targetDc = 2;
	bool _isMedia = false;
	int _mirrorIndex = 0;
	bool _wsConnected = false;
	bool _closed = false;
	State _state = State::Handshake;

	uint64 _upPackets = 0;
	uint64 _upBytes = 0;
	uint64 _downPackets = 0;
	uint64 _downBytes = 0;

};

class WsProxyServer final : public QObject {
public:
	WsProxyServer() {
		connect(&_server, &QTcpServer::newConnection, this, [this] {
			while (_server.hasPendingConnections()) {
				auto socket = _server.nextPendingConnection();
				if (!socket) continue;
				auto session = new WsProxySession(socket, this);
				_sessions.push_back(session);
			}
			_sessions.erase(
				std::remove_if(_sessions.begin(), _sessions.end(), [](const QPointer<WsProxySession> &s) {
					return s.isNull();
				}),
				_sessions.end());
		});
	}

	~WsProxyServer() {
		stop();
	}

	bool start(int desiredPort) {
		_server.close();
		for (auto session : _sessions) {
			if (session) {
				session->close();
			}
		}
		_sessions.clear();

		const auto tryPort = [&](int port) {
			return _server.listen(QHostAddress::LocalHost, quint16(port));
		};

		bool ok = tryPort(desiredPort);
		if (!ok) {
			const int fallbacks[] = { 1443, 10808, 1080, 0 };
			for (int p : fallbacks) {
				if (tryPort(p)) {
					ok = true;
					break;
				}
			}
		}

		if (!ok) {
			_activePort = 0;
			_status = u"Ошибка: порт %1 занят"_q.arg(desiredPort);
			g_status = _status;
			LogProxy(u"Proxy server failed to listen on port %1"_q.arg(desiredPort));
			return false;
		}

		_activePort = _server.serverPort();
		if (_activePort == desiredPort) {
			_status = u"Активен · 127.0.0.1:%1"_q.arg(_activePort);
		} else {
			_status = u"Активен (фоллбэк) · 127.0.0.1:%1"_q.arg(_activePort);
		}
		g_status = _status;

		LogProxy(u"Proxy server started on 127.0.0.1:%1 with secret dd%2"_q.arg(_activePort).arg(EnsureSecret()));

		ApplyProxyToTelegram();

		return true;
	}

	void stop() {
		if (_server.isListening()) {
			_server.close();
		}
		for (auto session : _sessions) {
			if (session) {
				session->close();
			}
		}
		_sessions.clear();
		_activePort = 0;
		_status = u"Не активен"_q;
		g_status = _status;

		LogProxy(u"Proxy server stopped."_q);

		crl::on_main([] {
			const auto &proxySettings = Core::App().settings().proxy();
			if (proxySettings.isEnabled()
				&& proxySettings.selected().type == MTP::ProxyData::Type::Mtproto
				&& proxySettings.selected().host == u"127.0.0.1"_q) {
				Core::App().setCurrentProxy(MTP::ProxyData(), MTP::ProxyData::Settings::System);
				Local::writeSettings();
			}
		});
	}

	[[nodiscard]] bool isRunning() const {
		return _server.isListening();
	}

	[[nodiscard]] int activePort() const {
		return _activePort;
	}

	[[nodiscard]] QString statusText() const {
		return _status;
	}

private:
	QTcpServer _server;
	std::vector<QPointer<WsProxySession>> _sessions;
	int _activePort = 0;
	QString _status = u"Не активен"_q;
};

std::unique_ptr<WsProxyServer> g_proxyServer;

} // namespace

void Start() {
	if (!g_proxyServer) {
		g_proxyServer = std::make_unique<WsProxyServer>();
	}
	if (!g_proxyServer->isRunning()) {
		g_proxyServer->start(CurrentConfig().wsProxyPort);
	}
}

void Stop() {
	if (g_proxyServer) {
		g_proxyServer->stop();
		g_proxyServer.reset();
	}
}

void Restart() {
	Stop();
	Start();
}

bool IsRunning() {
	return g_proxyServer && g_proxyServer->isRunning();
}

int ActivePort() {
	return g_proxyServer ? g_proxyServer->activePort() : 0;
}

QString FullSecret() {
	return u"dd"_q + EnsureSecret();
}

QString ProxyLink() {
	const auto port = ActivePort() ? ActivePort() : CurrentConfig().wsProxyPort;
	return u"tg://proxy?server=127.0.0.1&port=%1&secret=%2"_q
		.arg(port)
		.arg(FullSecret());
}

void ApplyProxyToTelegram() {
	crl::on_main([] {
		const auto port = ActivePort() ? ActivePort() : CurrentConfig().wsProxyPort;
		const auto sec = FullSecret();
		MTP::ProxyData proxy;
		proxy.type = MTP::ProxyData::Type::Mtproto;
		proxy.host = u"127.0.0.1"_q;
		proxy.port = port;
		proxy.password = sec;
		LogProxy(u"Applying MTProto proxy to Telegram: 127.0.0.1:%1 secret=%2"_q.arg(port).arg(sec));

		auto &settings = Core::App().settings().proxy();
		auto existingIndex = -1;
		for (int i = 0; i < int(settings.list().size()); ++i) {
			const auto &item = settings.list()[i];
			if (item.type == MTP::ProxyData::Type::Mtproto && item.host == u"127.0.0.1"_q) {
				existingIndex = i;
				break;
			}
		}
		if (existingIndex >= 0) {
			(void)settings.replaceInList(settings.list()[existingIndex], proxy);
		} else {
			settings.addToList(proxy);
		}

		Core::App().setCurrentProxy(proxy, MTP::ProxyData::Settings::Enabled);
		Local::writeSettings();
	});
}

QString StatusText() {
	return g_status.current();
}

rpl::producer<QString> StatusValue() {
	return g_status.value();
}

} // namespace WsProxy
} // namespace PixelGram
