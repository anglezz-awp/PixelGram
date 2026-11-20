/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/pixelgram_link_guard.h"

#include "core/application.h"
#include "core/file_utilities.h"
#include "core/click_handler_types.h"
#include "settings/pixelgram_settings.h"
#include "ui/pixelgram_m3_scheme.h"
#include "ui/layers/generic_box.h"
#include "ui/vertical_list.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/buttons.h"
#include "ui/toast/toast.h"
#include "ui/text/text_utilities.h"
#include "history/history_item.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"
#include "lang/lang_keys.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QRegularExpression>
#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtCore/QtEndian>
#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace PixelGram::LinkGuard {
namespace {

const auto kTrackerExact = std::vector<QString>{
	u"fbclid"_q, u"gclid"_q, u"gbraid"_q, u"wbraid"_q, u"dclid"_q, u"msclkid"_q,
	u"twclid"_q, u"ttclid"_q, u"igshid"_q, u"igsh"_q, u"yclid"_q, u"ysclid"_q,
	u"_openstat"_q, u"mkt_tok"_q, u"vero_id"_q, u"vero_conv"_q, u"mc_cid"_q,
	u"mc_eid"_q, u"oly_anon_id"_q, u"oly_enc_id"_q, u"hsctatracking"_q,
	u"__hssc"_q, u"__hstc"_q, u"__hsfp"_q, u"_hsenc"_q, u"_hsmi"_q,
	u"wickedid"_q, u"rb_clickid"_q, u"s_cid"_q, u"epik"_q, u"otc"_q,
	u"cvid"_q, u"erid"_q, u"ymclid"_q, u"yadclid"_q, u"_ga"_q, u"_gl"_q,
	u"mc_tc"_q, u"sc_cid"_q, u"srsltid"_q, u"cjevent"_q, u"irclickid"_q
};

const auto kTrackerPrefixes = std::vector<QString>{
	u"utm_"_q, u"pk_"_q, u"mtm_"_q, u"piwik_"_q, u"hsa_"_q,
	u"at_custom"_q, u"stm_"_q, u"matomo_"_q, u"sc_"_q
};

const auto kTrackerAggressive = std::vector<QString>{
	u"ref"_q, u"referer"_q, u"referrer"_q, u"ref_src"_q, u"ref_url"_q,
	u"si"_q, u"spm"_q, u"scm"_q, u"from"_q, u"source"_q,
	u"share_source"_q, u"share_medium"_q, u"share_plat"_q,
	u"_branch_match_id"_q, u"feature"_q, u"trk"_q, u"trkcampaign"_q
};

const auto kShorteners = std::vector<QString>{
	u"bit.ly"_q, u"tinyurl.com"_q, u"goo.gl"_q, u"t.co"_q, u"ow.ly"_q,
	u"is.gd"_q, u"buff.ly"_q, u"cutt.ly"_q, u"rb.gy"_q, u"shorturl.at"_q,
	u"clck.ru"_q, u"vk.cc"_q, u"surl.li"_q, u"u.to"_q, u"qps.ru"_q,
	u"clc.to"_q, u"gg.gg"_q, u"tiny.cc"_q, u"shorte.st"_q, u"lnkd.in"_q,
	u"s.id"_q, u"rebrand.ly"_q, u"t.ly"_q, u"short.io"_q, u"l.ead.me"_q,
	u"dfl.to"_q, u"goo.su"_q, u"clck.su"_q, u"kurl.ru"_q, u"shrturi.com"_q,
	u"tlgg.ru"_q
};

const auto kBrands = std::vector<QString>{
	u"telegram.org"_q, u"t.me"_q, u"telegram.me"_q, u"fragment.com"_q,
	u"walletbot.me"_q, u"google.com"_q, u"gmail.com"_q, u"youtube.com"_q,
	u"instagram.com"_q, u"facebook.com"_q, u"whatsapp.com"_q, u"discord.com"_q,
	u"steampowered.com"_q, u"steamcommunity.com"_q, u"github.com"_q,
	u"apple.com"_q, u"icloud.com"_q, u"microsoft.com"_q, u"office.com"_q,
	u"binance.com"_q, u"bybit.com"_q, u"metamask.io"_q, u"trustwallet.com"_q,
	u"vk.com"_q, u"ok.ru"_q, u"mail.ru"_q, u"yandex.ru"_q, u"dzen.ru"_q,
	u"avito.ru"_q, u"ozon.ru"_q, u"wildberries.ru"_q, u"sberbank.ru"_q,
	u"sbrf.ru"_q, u"tinkoff.ru"_q, u"tbank.ru"_q, u"alfabank.ru"_q,
	u"vtb.ru"_q, u"gosuslugi.ru"_q, u"nalog.gov.ru"_q, u"cdek.ru"_q,
	u"pochta.ru"_q, u"megafon.ru"_q, u"beeline.ru"_q, u"mts.ru"_q,
	u"aliexpress.ru"_q, u"citilink.ru"_q, u"dns-shop.ru"_q, u"mvideo.ru"_q,
	u"eldorado.ru"_q, u"lamoda.ru"_q, u"sportmaster.ru"_q, u"rzd.ru"_q,
	u"aeroflot.ru"_q, u"pochtabank.ru"_q, u"raiffeisen.ru"_q, u"psbank.ru"_q,
	u"gazprombank.ru"_q, u"sovcombank.ru"_q, u"yoomoney.ru"_q, u"sbermarket.ru"_q,
	u"samokat.ru"_q, u"vkusvill.ru"_q, u"rutube.ru"_q, u"twitch.tv"_q,
	u"epicgames.com"_q, u"roblox.com"_q, u"steamgifts.com"_q
};

const auto kGenericLabels = std::vector<QString>{
	u"mail"_q, u"office"_q, u"post"_q, u"link"_q, u"click"_q, u"wallet"_q
};

const auto kRiskyTld = std::vector<QString>{
	u"zip"_q, u"mov"_q, u"top"_q, u"xyz"_q, u"icu"_q, u"cfd"_q, u"sbs"_q,
	u"cyou"_q, u"click"_q, u"link"_q, u"rest"_q, u"quest"_q, u"monster"_q,
	u"bar"_q, u"beauty"_q, u"makeup"_q, u"skin"_q, u"hair"_q, u"boats"_q,
	u"autos"_q, u"lol"_q, u"gdn"_q, u"tk"_q, u"ml"_q, u"ga"_q, u"cf"_q,
	u"gq"_q, u"buzz"_q, u"cricket"_q, u"download"_q, u"loan"_q, u"party"_q,
	u"review"_q, u"science"_q, u"stream"_q, u"trade"_q, u"webcam"_q
};

const auto kBaitWords = std::vector<QString>{
	u"login"_q, u"signin"_q, u"verify"_q, u"confirm"_q, u"secure"_q,
	u"account"_q, u"update"_q, u"wallet"_q, u"seed"_q, u"recovery"_q,
	u"airdrop"_q, u"giveaway"_q, u"bonus"_q, u"promo"_q, u"gift"_q,
	u"premium-free"_q, u"nft-claim"_q, u"podarok"_q, u"vyigrysh"_q,
	u"bonus-ru"_q, u"oplata"_q, u"dostavka"_q, u"posylka"_q, u"shtraf"_q,
	u"vozvrat"_q, u"vyplata"_q, u"kompensaciya"_q, u"podtverdite"_q,
	u"razblokirovka"_q, u"verifikaciya"_q
};

const auto kDangerousExt = std::vector<QString>{
	u".apk"_q, u".exe"_q, u".msi"_q, u".scr"_q, u".bat"_q,
	u".cmd"_q, u".jar"_q, u".vbs"_q, u".apks"_q, u".xapk"_q
};

const auto kDangerousSchemes = std::vector<QString>{
	u"javascript"_q, u"data"_q, u"file"_q, u"intent"_q, u"content"_q, u"jar"_q
};

const auto kTwoLevelTld = std::vector<QString>{
	u"co.uk"_q, u"org.uk"_q, u"ac.uk"_q, u"gov.uk"_q, u"co.jp"_q,
	u"com.br"_q, u"com.au"_q, u"com.tr"_q, u"com.ua"_q, u"com.cn"_q,
	u"co.kr"_q, u"co.in"_q, u"com.mx"_q, u"gov.ru"_q, u"org.ru"_q,
	u"net.ru"_q, u"com.ru"_q, u"edu.ru"_q, u"ac.ru"_q, u"pp.ua"_q, u"in.ua"_q
};

const auto kLureWords = std::vector<QString>{
	u"shop"_q, u"store"_q, u"market"_q, u"magazin"_q, u"pay"_q, u"payment"_q,
	u"bank"_q, u"banking"_q, u"online"_q, u"official"_q, u"support"_q,
	u"help"_q, u"service"_q, u"servis"_q, u"login"_q, u"signin"_q,
	u"secure"_q, u"security"_q, u"verify"_q, u"verification"_q, u"account"_q,
	u"cabinet"_q, u"kabinet"_q, u"wallet"_q, u"claim"_q, u"gift"_q,
	u"bonus"_q, u"promo"_q, u"prize"_q, u"win"_q, u"winner"_q, u"refund"_q,
	u"update"_q, u"confirm"_q, u"center"_q, u"centre"_q, u"centr"_q,
	u"auth"_q, u"access"_q, u"recovery"_q, u"restore"_q, u"unlock"_q,
	u"oplata"_q, u"dostavka"_q, u"vyplata"_q, u"viplata"_q, u"vozvrat"_q,
	u"podarok"_q, u"vhod"_q, u"lk"_q, u"id"_q, u"app"_q, u"apk"_q,
	u"download"_q, u"mobile"_q, u"invest"_q, u"bot"_q, u"premium"_q,
	u"vip"_q, u"free"_q
};

constexpr auto kDbBaseUrl = "https://github.com/L0lopop/Link-Guard/releases/download/feeds/";
constexpr auto kDbName = "full.lgdb";
constexpr auto kDbManifestName = "manifest.json";
constexpr auto kDbFileName = "link_guard_domains.lgdb";

// Bypass cache: URLs approved by user within 15 seconds bypass interception
QMap<QString, crl::time> g_bypassList;

// RDAP age cache: domain -> (cachedTime, days)
QMap<QString, QPair<QDateTime, int>> g_ageCache;

// Global Network Manager
QNetworkAccessManager *g_networkManager = nullptr;
QTimer *g_dbTimer = nullptr;

QNetworkAccessManager *GetNetworkManager() {
	if (!g_networkManager) {
		g_networkManager = new QNetworkAccessManager(QCoreApplication::instance());
	}
	return g_networkManager;
}

QString DatabasePath() {
	return cWorkingDir() + u"tdata/"_q + kDbFileName;
}

// ------------------ Levenshtein Distance ------------------
int Levenshtein(const QString &s1, const QString &s2) {
	const int m = s1.length();
	const int n = s2.length();
	if (m == 0) return n;
	if (n == 0) return m;

	std::vector<int> v0(n + 1);
	std::vector<int> v1(n + 1);

	for (int i = 0; i <= n; ++i) v0[i] = i;

	for (int i = 0; i < m; ++i) {
		v1[0] = i + 1;
		for (int j = 0; j < n; ++j) {
			const int cost = (s1[i] == s2[j]) ? 0 : 1;
			v1[j + 1] = std::min({ v1[j] + 1, v0[j + 1] + 1, v0[j] + cost });
		}
		v0 = v1;
	}
	return v0[n];
}

// ------------------ Lookalike Letter Defusing ------------------
QChar DefuseLookalike(QChar ch) {
	const auto u = ch.unicode();
	switch (u) {
	case 0x0430: return 'a'; // а
	case 0x0431: return 'b'; // б
	case 0x0432: return 'b'; // в
	case 0x0435: return 'e'; // е
	case 0x0451: return 'e'; // ё
	case 0x043A: return 'k'; // к
	case 0x043C: return 'm'; // м
	case 0x043D: return 'h'; // н
	case 0x043E: return 'o'; // о
	case 0x0440: return 'p'; // р
	case 0x0441: return 'c'; // с
	case 0x0442: return 't'; // т
	case 0x0443: return 'y'; // у
	case 0x0445: return 'x'; // х
	case 0x0456: return 'i'; // і
	case 0x0458: return 'j'; // ј
	case 0x0455: return 's'; // ѕ
	case 0x0501: return 'd'; // ԁ
	case 0x051B: return 'q'; // ԛ
	case 0x051D: return 'w'; // ԝ
	case 0x0261: return 'g'; // ɡ
	case 0x03BF: return 'o'; // ο
	case 0x03B1: return 'a'; // α
	case 0x03BD: return 'v'; // ν
	case 0x03C1: return 'p'; // ρ
	case 0x03F2: return 'c'; // ϲ
	case 0x03C4: return 't'; // τ
	case 0x03C5: return 'u'; // υ
	default: return ch;
	}
}

QString ToLatinLookalike(const QString &text) {
	QString res;
	res.reserve(text.length());
	for (const auto &ch : text.toLower()) {
		res.append(DefuseLookalike(ch));
	}
	return res;
}

// ------------------ Binary Domain Database (LGDB) ------------------
class DomainDatabase {
public:
	explicit DomainDatabase(const QByteArray &data) {
		parse(data);
	}

	bool isValid() const {
		return _valid;
	}

	uint32 builtDay() const {
		return _builtDay;
	}

	int ageDays() const {
		const auto nowSec = QDateTime::currentSecsSinceEpoch();
		const auto nowDay = uint32(nowSec / 86400);
		return (nowDay >= _builtDay) ? int(nowDay - _builtDay) : 0;
	}

	int totalEntries() const {
		return _totalEntries;
	}

	bool contains(const QString &tag, const QString &host) const {
		auto it = _sections.find(tag);
		if (it == _sections.end()) return false;
		const auto &raw = it.value();
		if (raw.size() < 11) return false;

		const uint8 bits = static_cast<uint8>(raw[0]);
		const uint32 count = qFromBigEndian<uint32>(raw.constData() + 1);
		const uint16 block = qFromBigEndian<uint16>(raw.constData() + 5);
		const uint32 indexCount = qFromBigEndian<uint32>(raw.constData() + 7);

		if (!count || !indexCount) return false;

		const auto hash = QCryptographicHash::hash(host.toLower().toUtf8(), QCryptographicHash::Sha256);
		const uint64 digest = qFromBigEndian<uint64>(hash.constData());
		const uint64 key = (bits >= 64) ? digest : (digest >> (64 - bits));

		const int indexAt = 11;
		constexpr int kIndexEntrySize = 12;
		const int dataAt = indexAt + int(indexCount * kIndexEntrySize);
		if (dataAt > raw.size()) return false;

		int low = 0;
		int high = int(indexCount);
		while (low < high) {
			const int mid = (low + high) / 2;
			const int spot = indexAt + mid * kIndexEntrySize;
			const uint64 midKey = qFromBigEndian<uint64>(raw.constData() + spot);
			if (midKey <= key) {
				low = mid + 1;
			} else {
				high = mid;
			}
		}

		if (low == 0) return false;

		const int spot = indexAt + (low - 1) * kIndexEntrySize;
		uint64 value = qFromBigEndian<uint64>(raw.constData() + spot);
		const uint32 start = qFromBigEndian<uint32>(raw.constData() + spot + 8);

		if (value == key) return true;

		int position = dataAt + int(start);
		const int limit = raw.size();
		int seen = 0;

		while (seen < block && position < limit) {
			int shift = 0;
			uint64 delta = 0;
			while (position < limit) {
				const uint8 b = static_cast<uint8>(raw[position++]);
				delta |= uint64(b & 0x7F) << shift;
				if (!(b & 0x80)) break;
				shift += 7;
			}
			if (seen > 0) {
				value += delta;
				if (value == key) return true;
				if (value > key) return false;
			}
			seen++;
		}
		return false;
	}

	bool isPopular(const QString &host) const {
		return contains(u"WHIT"_q, host);
	}

	QString maliciousHit(const QString &host) const {
		const auto parts = host.split('.');
		for (int i = 0; i < parts.size() - 1; ++i) {
			const auto candidate = QStringList(parts.mid(i)).join('.');
			if (contains(u"MALW"_q, candidate)) {
				return candidate;
			}
			if (_platforms.contains(candidate)) {
				break;
			}
		}
		return QString();
	}

	int freshness(const QString &host) const {
		if (contains(u"FR10"_q, host)) return 10;
		if (contains(u"FR30"_q, host)) return 30;
		return 0;
	}

	int zoneLevel(const QString &zone) const {
		return _zones.value(zone.toLower(), 0);
	}

	const std::vector<QString> &suffixes() const {
		return _suffixes;
	}

	const std::vector<QString> &brands() const {
		return _brands;
	}

private:
	void parse(const QByteArray &blob) {
		if (blob.size() < 10 || !blob.startsWith("LGDB")) {
			return;
		}
		const uint8 version = static_cast<uint8>(blob[4]);
		if (version != 1) return;

		_builtDay = qFromBigEndian<uint32>(blob.constData() + 5);
		const uint8 count = static_cast<uint8>(blob[9]);

		int offset = 10;
		for (int i = 0; i < count; ++i) {
			if (offset + 8 > blob.size()) return;
			const auto tag = QString::fromLatin1(blob.constData() + offset, 4);
			const uint32 len = qFromBigEndian<uint32>(blob.constData() + offset + 4);
			offset += 8;
			if (offset + int(len) > blob.size()) return;
			_sections[tag] = blob.mid(offset, len);
			offset += int(len);
		}

		if (!_sections.contains(u"MALW"_q)) return;
		const auto &malw = _sections[u"MALW"_q];
		if (malw.size() >= 5) {
			_totalEntries = qFromBigEndian<uint32>(malw.constData() + 1);
		}

		auto parseLines = [&](const QString &tag) {
			std::vector<QString> lines;
			if (_sections.contains(tag)) {
				const auto str = QString::fromUtf8(_sections[tag]);
				for (const auto &line : str.split('\n', Qt::SkipEmptyParts)) {
					const auto trimmed = line.trimmed();
					if (!trimmed.isEmpty()) lines.push_back(trimmed);
				}
			}
			return lines;
		};

		for (const auto &p : parseLines(u"PLAT"_q)) _platforms.insert(p);
		_brands = parseLines(u"BRND"_q);
		_suffixes = parseLines(u"SUFX"_q);

		for (const auto &line : parseLines(u"TLDR"_q)) {
			const auto parts = line.split(' ');
			if (parts.size() == 2) {
				_zones[parts[0]] = parts[1].toInt();
			}
		}
		_valid = true;
	}

	bool _valid = false;
	uint32 _builtDay = 0;
	int _totalEntries = 0;
	QMap<QString, QByteArray> _sections;
	QSet<QString> _platforms;
	std::vector<QString> _brands;
	std::vector<QString> _suffixes;
	QMap<QString, int> _zones;
};

std::unique_ptr<DomainDatabase> g_database;

// ------------------ Registrable Domain Extraction ------------------
QString Registrable(const QString &host) {
	const auto parts = host.split('.');
	if (parts.size() < 2) return host;

	if (parts.size() >= 3) {
		const auto tail = parts[parts.size() - 2] + '.' + parts.last();
		bool isTwoLevel = false;
		for (const auto &t : kTwoLevelTld) {
			if (tail.compare(t, Qt::CaseInsensitive) == 0) {
				isTwoLevel = true;
				break;
			}
		}
		if (!isTwoLevel && g_database) {
			for (const auto &s : g_database->suffixes()) {
				if (tail.compare(s, Qt::CaseInsensitive) == 0) {
					isTwoLevel = true;
					break;
				}
			}
		}
		if (isTwoLevel) {
			return parts[parts.size() - 3] + '.' + tail;
		}
	}
	return parts[parts.size() - 2] + '.' + parts.last();
}

bool IsPrivateIp(const QString &host) {
	const auto parts = host.split('.');
	if (parts.size() != 4) return false;
	bool ok1 = false, ok2 = false;
	const int first = parts[0].toInt(&ok1);
	const int second = parts[1].toInt(&ok2);
	if (!ok1 || !ok2) return false;

	return (first == 10)
		|| (first == 127)
		|| (first == 192 && second == 168)
		|| (first == 172 && second >= 16 && second <= 31)
		|| (first == 169 && second == 254);
}

QString PluralForm(int count, const QString &one, const QString &few, const QString &many) {
	const int n = std::abs(count);
	const int tail = n % 100;
	if (tail >= 11 && tail <= 14) return many;
	const int rem = tail % 10;
	if (rem == 1) return one;
	if (rem >= 2 && rem <= 4) return few;
	return many;
}

} // namespace

RiskLevel Verdict::risk() const {
	if (flags.empty()) return RiskLevel::Info;
	auto maxLevel = RiskLevel::Info;
	for (const auto &f : flags) {
		if (int(f.level) > int(maxLevel)) {
			maxLevel = f.level;
		}
	}
	return maxLevel;
}

bool Verdict::suspicious() const {
	return int(risk()) >= int(RiskLevel::Medium);
}

// ------------------ Clean URL Implementation ------------------
QString CleanUrl(const QString &url, bool aggressive, std::vector<QString> *outRemoved) {
	QUrl qurl(url);
	if (!qurl.hasQuery()) return url;

	QUrlQuery query(qurl);
	const auto items = query.queryItems(QUrl::FullyDecoded);
	QUrlQuery newQuery;
	std::vector<QString> removed;

	for (const auto &item : items) {
		const auto key = item.first;
		const auto keyLower = key.toLower();
		bool drop = false;

		for (const auto &exact : kTrackerExact) {
			if (keyLower == exact) {
				drop = true;
				break;
			}
		}
		if (!drop) {
			for (const auto &prefix : kTrackerPrefixes) {
				if (keyLower.startsWith(prefix)) {
					drop = true;
					break;
				}
			}
		}
		if (!drop && aggressive) {
			for (const auto &aggr : kTrackerAggressive) {
				if (keyLower == aggr) {
					drop = true;
					break;
				}
			}
		}

		if (drop) {
			removed.push_back(key);
		} else {
			newQuery.addQueryItem(item.first, item.second);
		}
	}

	if (removed.empty()) return url;
	if (outRemoved) *outRemoved = removed;

	qurl.setQuery(newQuery);
	return qurl.toString(QUrl::FullyEncoded);
}

void CleanOutgoing(TextWithTags &textWithTags) {
	const auto &cfg = CurrentConfig();
	if (!cfg.linkGuardCleanOutgoing) return;

	static const auto kUrlRe = QRegularExpression(
		u"(?:(?:https?|ftp)://|www\\.)[^\\s<>\"']*"_q,
		QRegularExpression::CaseInsensitiveOption);

	QString text = textWithTags.text;
	int offsetShift = 0;
	int removedTotal = 0;

	auto it = kUrlRe.globalMatch(text);
	QString resultText;
	int lastPos = 0;

	while (it.hasNext()) {
		const auto match = it.next();
		resultText.append(text.mid(lastPos, match.capturedStart() - lastPos));

		const auto originalUrl = match.captured();
		std::vector<QString> removed;
		const auto cleaned = CleanUrl(originalUrl, cfg.linkGuardAggressive, &removed);

		resultText.append(cleaned);
		lastPos = match.capturedEnd();
		removedTotal += int(removed.size());
	}
	resultText.append(text.mid(lastPos));

	if (removedTotal > 0) {
		textWithTags.text = resultText;
		IncrementCleaned(removedTotal);
	}
}

// ------------------ Deep Link Analysis ------------------
Verdict Analyze(const QString &url, const QString &anchor) {
	Verdict verdict;
	verdict.original = url;

	const auto &cfg = CurrentConfig();
	verdict.cleaned = CleanUrl(url, cfg.linkGuardAggressive, &verdict.removedParams);

	QUrl qurl(url.contains(u"://"_q) ? url : (u"http://"_q + url));
	const auto scheme = qurl.scheme().toLower();

	for (const auto &badScheme : kDangerousSchemes) {
		if (scheme == badScheme) {
			verdict.flags.push_back({ RiskLevel::High, u"Схема «%1» — это выполнение кода или открытие файла, а не сайт"_q.arg(scheme) });
			return verdict;
		}
	}

	const auto host = qurl.host().toLower();
	verdict.host = host;
	verdict.displayHost = QUrl::fromAce(host.toLatin1());

	if (host.isEmpty()) {
		verdict.flags.push_back({ RiskLevel::Medium, u"В адресе отсутствует домен"_q });
		return verdict;
	}

	const auto reg = Registrable(host);
	const auto shownHost = verdict.displayHost;
	const auto shownReg = Registrable(shownHost);

	// Whitelist check
	if (IsWhitelisted(host) || IsWhitelisted(reg) || IsWhitelisted(shownHost) || IsWhitelisted(shownReg)) {
		return verdict;
	}

	bool knownGood = false;
	if (g_database && g_database->isValid()) {
		const auto hit = g_database->maliciousHit(host);
		if (!hit.isEmpty()) {
			if (hit == host) {
				verdict.flags.push_back({ RiskLevel::High, u"Сайт числится в базах мошеннических ресурсов"_q });
			} else {
				verdict.flags.push_back({ RiskLevel::High, u"Домен %1 числится в базах мошеннических ресурсов"_q.arg(hit) });
			}
		} else {
			knownGood = g_database->isPopular(host) || g_database->isPopular(reg);
		}
	}

	// 1. Userinfo / @ check (e.g. https://t.me@evil.com)
	if (!qurl.userName().isEmpty() || url.contains('@')) {
		verdict.flags.push_back({ RiskLevel::High, u"Перед символом @ указан текст-обманка — настоящий домен здесь %1"_q.arg(host) });
	}

	// 2. Punycode check
	if (shownHost != host) {
		verdict.flags.push_back({ RiskLevel::Low, u"Домен записан в punycode и выглядит как «%1»"_q.arg(shownHost) });
	}

	// 3. Mixed scripts check (Cyrillic + Latin)
	bool hasLatin = false;
	bool hasCyrillic = false;
	for (const auto &ch : shownHost) {
		const auto u = ch.unicode();
		if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z')) hasLatin = true;
		else if (u >= 0x0400 && u <= 0x04FF) hasCyrillic = true;
	}
	if (hasLatin && hasCyrillic) {
		verdict.flags.push_back({ RiskLevel::High, u"В имени домена смешаны кириллица и латиница — типичная маскировка фишинга"_q });
	}

	// 4. Lookalike & Typo check against brands
	const auto defused = ToLatinLookalike(shownHost);
	const auto defusedReg = Registrable(defused);

	std::vector<QString> allBrands = kBrands;
	if (g_database && g_database->isValid()) {
		for (const auto &b : g_database->brands()) allBrands.push_back(b);
	}

	bool brandMatch = false;
	for (const auto &b : allBrands) {
		if (defusedReg == b) {
			brandMatch = true;
			break;
		}
	}

	if (defusedReg != reg && brandMatch) {
		verdict.flags.push_back({ RiskLevel::High, u"Домен визуально копирует известный сервис %1 с подменой букв"_q.arg(defusedReg) });
	} else if (!brandMatch && !knownGood) {
		for (const auto &brand : allBrands) {
			if (std::abs(defusedReg.length() - brand.length()) > 2) continue;
			const int dist = Levenshtein(defusedReg, brand);
			if (dist >= 1 && dist <= 2) {
				const auto unit = PluralForm(dist, u"символ"_q, u"символа"_q, u"символов"_q);
				verdict.flags.push_back({ RiskLevel::High, u"Домен отличается от %1 всего на %2 %3 (возможная опечатка/фишинг)"_q.arg(brand).arg(dist).arg(unit) });
				break;
			}
		}
	}

	// 5. Brand in subdomains (e.g. telegram.evil.com)
	if (!knownGood) {
		const auto labels = shownHost.split('.');
		for (const auto &brand : kBrands) {
			const auto head = brand.split('.').first();
			if (head.length() < 5) continue;
			for (int i = 0; i < labels.size() - 2; ++i) {
				if (labels[i].compare(head, Qt::CaseInsensitive) == 0) {
					verdict.flags.push_back({ RiskLevel::High, u"«%1» находится в поддомене, а настоящий сайт — %2"_q.arg(head).arg(reg) });
					break;
				}
			}
		}
	}

	// 6. Anchor mismatch (e.g. [t.me/durov](https://evil.com))
	if (!anchor.isEmpty()) {
		static const auto kAnchorDomainRe = QRegularExpression(u"^(?:https?://)?([a-z0-9\\-._~\\x{0400}-\\x{04FF}]+\\.[a-z\\x{0400}-\\x{04FF}]{2,})"_q, QRegularExpression::CaseInsensitiveOption);
		const auto match = kAnchorDomainRe.match(anchor.trimmed());
		if (match.hasMatch()) {
			const auto anchorHost = Registrable(QUrl::fromAce(match.captured(1).toLatin1()));
			if (!anchorHost.isEmpty() && anchorHost != reg && anchorHost != shownReg) {
				verdict.flags.push_back({ RiskLevel::High, u"Текст ссылки указывает на %1, но переход ведёт на %2"_q.arg(anchorHost).arg(reg) });
			}
		}
	}

	// 7. IP Address
	static const auto kIpRe = QRegularExpression(u"^\\d{1,3}(?:\\.\\d{1,3}){3}$"_q);
	if (kIpRe.match(host).hasMatch()) {
		if (IsPrivateIp(host)) {
			verdict.flags.push_back({ RiskLevel::Low, u"Адрес находится в локальной частной сети"_q });
		} else {
			verdict.flags.push_back({ RiskLevel::Medium, u"Вместо доменного имени используется прямой IP-адрес"_q });
		}
	}

	// 8. Non-standard port
	const int port = qurl.port();
	if (port > 0 && port != 80 && port != 443) {
		verdict.flags.push_back({ RiskLevel::Low, u"Нестандартный порт подключения: %1"_q.arg(port) });
	}

	// 9. Lure words + Brand glued (e.g. sberbank-oplata.ru)
	if (!knownGood) {
		const auto label = reg.split('.').first();
		for (const auto &brand : kBrands) {
			const auto core = brand.split('.').first();
			if (core.length() < 3 || label == core) continue;
			if (label.contains(core)) {
				for (const auto &lure : kLureWords) {
					if (label.contains(lure)) {
						verdict.flags.push_back({ RiskLevel::High, u"В домене используется имя «%1» с приманкой «%2»"_q.arg(core).arg(lure) });
						break;
					}
				}
			}
		}
	}

	// 10. Dangerous extensions
	const auto path = qurl.path().toLower();
	for (const auto &ext : kDangerousExt) {
		if (path.endsWith(ext)) {
			verdict.flags.push_back({ RiskLevel::High, u"Ссылка ведёт на исполняемый файл %1 (установка программы)"_q.arg(ext) });
			break;
		}
	}

	// 11. Bait words in URL path / domain
	if (!knownGood) {
		const auto fullLower = (host + path).toLower();
		QStringList foundBait;
		for (const auto &bait : kBaitWords) {
			if (fullLower.contains(bait)) {
				foundBait.push_back(bait);
				if (foundBait.size() >= 3) break;
			}
		}
		if (!foundBait.isEmpty()) {
			verdict.flags.push_back({ verdict.flags.empty() ? RiskLevel::Low : RiskLevel::Medium, u"В адресе обнаружены слова-приманки: %1"_q.arg(foundBait.join(u", "_q)) });
		}
	}

	// 12. Risky TLD
	if (!knownGood) {
		const auto tld = host.split('.').last().toLower();
		int level = g_database ? g_database->zoneLevel(tld) : 0;
		if (level >= 3) {
			verdict.flags.push_back({ RiskLevel::Medium, u"Доменная зона .%1 известна массовым размещением мошеннических сайтов"_q.arg(tld) });
		} else {
			for (const auto &risky : kRiskyTld) {
				if (tld == risky) {
					verdict.flags.push_back({ RiskLevel::Low, u"Доменная зона .%1 часто используется для одноразовых сайтов"_q.arg(tld) });
					break;
				}
			}
		}
	}

	// 13. Fresh registration from database
	if (g_database && !knownGood) {
		const int fresh = g_database->freshness(reg);
		if (fresh == 10) {
			verdict.flags.push_back({ RiskLevel::High, u"Домен зарегистрирован менее 10 дней назад и несёт имя известного сервиса"_q });
		} else if (fresh == 30) {
			verdict.flags.push_back({ RiskLevel::Medium, u"Домен зарегистрирован менее месяца назад и несёт имя известного сервиса"_q });
		}
	}

	// 14. Shortener check
	for (const auto &sh : kShorteners) {
		if (reg == sh || host == sh) {
			verdict.isShortener = true;
			verdict.flags.push_back({ RiskLevel::Info, u"Сокращённая ссылка — реальный адрес скрыт за редиректом"_q });
			break;
		}
	}

	return verdict;
}

// ------------------ Network Operations: Short URL Expansion & RDAP ------------------
void ExpandShortUrl(const QString &url, Fn<void(QString finalUrl, int hops)> callback) {
	auto req = QNetworkRequest(QUrl(url));
	req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	req.setHeader(QNetworkRequest::UserAgentHeader, u"Mozilla/5.0 (Windows NT 10.0; Win64; x64) PixelGram/LinkGuard"_q);

	auto reply = GetNetworkManager()->head(req);
	QObject::connect(reply, &QNetworkReply::finished, [reply, callback, url] {
		reply->deleteLater();
		const auto redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		if (redirect.isValid()) {
			callback(redirect.toString(), 1);
		} else {
			const auto finalUrl = reply->url().toString();
			callback((!finalUrl.isEmpty() && finalUrl != url) ? finalUrl : url, 1);
		}
	});
}

void QueryDomainAge(const QString &domain, Fn<void(std::optional<int> days)> callback) {
	if (g_ageCache.contains(domain)) {
		const auto &entry = g_ageCache[domain];
		if (entry.first.daysTo(QDateTime::currentDateTime()) < 7) {
			callback(entry.second);
			return;
		}
	}

	const auto reqUrl = u"https://rdap.org/domain/"_q + domain;
	auto req = QNetworkRequest(QUrl(reqUrl));
	req.setHeader(QNetworkRequest::UserAgentHeader, u"PixelGram/LinkGuard 1.0"_q);
	req.setRawHeader("Accept", "application/rdap+json");

	auto reply = GetNetworkManager()->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [reply, callback, domain] {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			callback(std::nullopt);
			return;
		}
		const auto doc = QJsonDocument::fromJson(reply->readAll());
		if (!doc.isObject()) {
			callback(std::nullopt);
			return;
		}
		const auto events = doc.object()["events"].toArray();
		for (const auto &evVal : events) {
			const auto evObj = evVal.toObject();
			if (evObj["eventAction"].toString().compare(u"registration"_q, Qt::CaseInsensitive) == 0) {
				const auto dateStr = evObj["eventDate"].toString().left(10);
				const auto born = QDate::fromString(dateStr, Qt::ISODate);
				if (born.isValid()) {
					const int days = int(born.daysTo(QDate::currentDate()));
					g_ageCache[domain] = qMakePair(QDateTime::currentDateTime(), days);
					callback(days);
					return;
				}
			}
		}
		callback(std::nullopt);
	});
}

// ------------------ M3 Breakdown Dialog ------------------
void ShowBreakdownDialog(Verdict verdict, not_null<Window::SessionController*> controller) {
	const auto targetUrl = verdict.finalUrl.isEmpty() ? verdict.cleaned : verdict.finalUrl;
	const auto host = Registrable(verdict.displayHost.isEmpty() ? verdict.host : verdict.displayHost);

	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(st::boxWideWidth);

		const auto risk = verdict.risk();
		if (risk == RiskLevel::High) {
			box->setTitle(rpl::single(u"⛔ Опасная ссылка"_q));
		} else if (risk == RiskLevel::Medium) {
			box->setTitle(rpl::single(u"⚠️ Подозрительная ссылка"_q));
		} else {
			box->setTitle(rpl::single(u"🔗 Разбор ссылки"_q));
		}

		auto content = box->verticalLayout();
		Ui::AddSkip(content, 4);

		// Domain pill
		if (!host.isEmpty()) {
			auto domainLabel = content->add(object_ptr<Ui::FlatLabel>(
				content,
				u"<b>Домен:</b> %1"_q.arg(host),
				st::defaultFlatLabel));
			Ui::AddSkip(content, 4);
		}

		// Destination if expanded
		if (!verdict.finalUrl.isEmpty() && verdict.finalUrl != verdict.cleaned) {
			content->add(object_ptr<Ui::FlatLabel>(
				content,
				u"<b>Ведёт на:</b> %1"_q.arg(verdict.finalUrl),
				st::defaultFlatLabel));
			Ui::AddSkip(content, 4);
		}

		// Removed trackers count
		if (!verdict.removedParams.empty()) {
			const int count = int(verdict.removedParams.size());
			const auto unit = PluralForm(count, u"метка"_q, u"метки"_q, u"меток"_q);
			content->add(object_ptr<Ui::FlatLabel>(
				content,
				u"<b>Убрано меток:</b> %1 %2"_q.arg(count).arg(unit),
				st::defaultFlatLabel));
			Ui::AddSkip(content, 4);
		}

		// Age if available
		if (verdict.ageDays.has_value()) {
			const int days = *verdict.ageDays;
			const auto ageText = (days < 1)
				? u"меньше суток"_q
				: (days < 30)
				? u"%1 %2"_q.arg(days).arg(PluralForm(days, u"день"_q, u"дня"_q, u"дней"_q))
				: u"%1 %2"_q.arg(days / 30).arg(PluralForm(days / 30, u"месяц"_q, u"месяца"_q, u"месяцев"_q));
			content->add(object_ptr<Ui::FlatLabel>(
				content,
				u"<b>Возраст домена:</b> %1"_q.arg(ageText),
				st::defaultFlatLabel));
			Ui::AddSkip(content, 4);
		}

		// Warnings list
		if (!verdict.flags.empty()) {
			Ui::AddDivider(content);
			Ui::AddSkip(content, 6);
			for (const auto &flag : verdict.flags) {
				const auto icon = (flag.level == RiskLevel::High)
					? u"⛔ "_q
					: (flag.level == RiskLevel::Medium)
					? u"⚠️ "_q
					: u"• "_q;
				content->add(object_ptr<Ui::FlatLabel>(
					content,
					icon + flag.text,
					st::defaultFlatLabel));
				Ui::AddSkip(content, 2);
			}
			Ui::AddSkip(content, 6);
			Ui::AddDivider(content);
		}

		// Full URL preview
		Ui::AddSkip(content, 6);
		auto urlLabel = content->add(object_ptr<Ui::FlatLabel>(
			content,
			targetUrl.length() > 200 ? (targetUrl.left(200) + u"…"_q) : targetUrl,
			st::boxLabel));
		urlLabel->setTextColorOverride(st::windowSubTextFg->c);
		Ui::AddSkip(content, 8);

		// Buttons: Open, Trust, Cancel
		box->addButton(rpl::single(u"Открыть"_q), [=] {
			box->closeBox();
			g_bypassList[targetUrl] = crl::now();
			g_bypassList[verdict.original] = crl::now();
			if (!verdict.removedParams.empty()) {
				IncrementCleaned(int(verdict.removedParams.size()));
			}
			File::OpenUrlWithOwnAutoLogin(targetUrl);
		});

		if (!host.isEmpty()) {
			box->addButton(rpl::single(u"Доверять"_q), [=] {
				box->closeBox();
				AddWhitelistDomain(host);
				g_bypassList[targetUrl] = crl::now();
				File::OpenUrlWithOwnAutoLogin(targetUrl);
				if (controller) {
					controller->showToast(u"%1 добавлен в исключения"_q.arg(host));
				}
			});
		}

		box->addButton(rpl::single(u"Отмена"_q), [=] {
			box->closeBox();
		});
	}));
}

// ------------------ URL Click Handler Interception ------------------
bool IsBypassed(const QString &url) {
	const auto now = crl::now();
	for (auto it = g_bypassList.begin(); it != g_bypassList.end();) {
		if (now - it.value() > 15000) {
			it = g_bypassList.erase(it);
		} else {
			++it;
		}
	}
	return g_bypassList.contains(url);
}

bool HandleUrlClick(
		const QString &url,
		const QVariant &context,
		std::shared_ptr<Ui::Show> show) {
	const auto &cfg = CurrentConfig();
	if (!cfg.linkGuardCheckEnabled) {
		return false;
	}

	if (IsBypassed(url)) {
		return false;
	}

	const auto my = context.value<ClickHandlerContext>();
	const auto controller = my.sessionWindow.get()
		? my.sessionWindow.get()
		: Core::App().activePrimaryWindow()
		? Core::App().activePrimaryWindow()->sessionController()
		: nullptr;

	auto verdict = Analyze(url);

	const bool isSuspiciousOrDangerous = verdict.suspicious();
	const bool modeAlways = (cfg.linkGuardShowMode == 1);
	const bool hasFlags = !verdict.flags.empty();

	const bool worthShowing = modeAlways || isSuspiciousOrDangerous || (hasFlags && verdict.risk() != RiskLevel::Info);
	const bool needsDialog = worthShowing || (verdict.isShortener && cfg.linkGuardExpandShort);

	if (!needsDialog) {
		if (cfg.linkGuardCleanOnOpen && verdict.cleaned != url) {
			g_bypassList[verdict.cleaned] = crl::now();
			g_bypassList[url] = crl::now();
			const int removed = int(verdict.removedParams.size());
			IncrementCleaned(removed);
			if (removed > 0 && controller) {
				controller->showToast(u"Убрано меток слежения: %1"_q.arg(removed));
			}
			File::OpenUrlWithOwnAutoLogin(verdict.cleaned);
			return true;
		}
		return false;
	}

	if (isSuspiciousOrDangerous) {
		IncrementWarned();
	}

	if (!controller) {
		return false;
	}

	const bool expandShort = verdict.isShortener && cfg.linkGuardExpandShort;
	const bool checkAge = cfg.linkGuardCheckAge && !verdict.flags.empty();

	if (expandShort) {
		ExpandShortUrl(verdict.cleaned, [=](QString finalUrl, int hops) mutable {
			if (!finalUrl.isEmpty() && finalUrl != verdict.cleaned) {
				verdict.finalUrl = finalUrl;
				auto inner = Analyze(finalUrl);
				for (const auto &f : inner.flags) {
					verdict.flags.push_back(f);
				}
				if (hops > 0) {
					verdict.flags.push_back({ RiskLevel::Info, u"Редиректов по пути: %1"_q.arg(hops) });
				}
			}
			if (checkAge) {
				const auto target = verdict.finalUrl.isEmpty() ? verdict.cleaned : verdict.finalUrl;
				const auto domain = Registrable(QUrl(target).host());
				QueryDomainAge(domain, [=](std::optional<int> ageDays) mutable {
					verdict.ageDays = ageDays;
					if (ageDays.has_value() && *ageDays <= 30) {
						verdict.flags.push_back({ RiskLevel::High, u"Домену всего %1 дн. — свежая регистрация!"_q.arg(*ageDays) });
					}
					ShowBreakdownDialog(verdict, controller);
				});
			} else {
				ShowBreakdownDialog(verdict, controller);
			}
		});
		return true;
	} else if (checkAge) {
		const auto domain = Registrable(QUrl(verdict.cleaned).host());
		QueryDomainAge(domain, [=](std::optional<int> ageDays) mutable {
			verdict.ageDays = ageDays;
			if (ageDays.has_value() && *ageDays <= 30) {
				verdict.flags.push_back({ RiskLevel::High, u"Домену всего %1 дн. — свежая регистрация!"_q.arg(*ageDays) });
			}
			ShowBreakdownDialog(verdict, controller);
		});
		return true;
	}

	ShowBreakdownDialog(verdict, controller);
	return true;
}

// ------------------ Context Menu Helpers ------------------
std::vector<QString> ExtractUrls(not_null<HistoryItem*> item) {
	std::vector<QString> urls;
	const auto textWithEntities = item->originalText();
	for (const auto &entity : textWithEntities.entities) {
		if (entity.type() == EntityType::CustomUrl) {
			urls.push_back(entity.data());
		} else if (entity.type() == EntityType::Url) {
			urls.push_back(textWithEntities.text.mid(entity.offset(), entity.length()));
		}
	}

	static const auto kUrlRe = QRegularExpression(
		u"(?:(?:https?|ftp)://|www\\.)[^\\s<>\"']*"_q,
		QRegularExpression::CaseInsensitiveOption);
	auto matchIt = kUrlRe.globalMatch(textWithEntities.text);
	while (matchIt.hasNext()) {
		const auto url = matchIt.next().captured();
		bool exists = false;
		for (const auto &u : urls) {
			if (u == url) { exists = true; break; }
		}
		if (!exists) urls.push_back(url);
	}
	return urls;
}

void ShowReportBox(
		not_null<Window::SessionController*> controller,
		const std::vector<QString> &urls) {
	if (urls.empty()) {
		controller->showToast(u"Ссылок в сообщении нет"_q);
		return;
	}

	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(st::boxWideWidth);
		box->setTitle(rpl::single(u"Проверка ссылок (%1)"_q.arg(urls.size())));

		auto content = box->verticalLayout();
		for (size_t i = 0; i < urls.size() && i < 10; ++i) {
			const auto verdict = Analyze(urls[i]);
			const auto risk = verdict.risk();
			const auto icon = (risk == RiskLevel::High) ? u"⛔ "_q : (risk == RiskLevel::Medium) ? u"⚠️ "_q : u"✓ "_q;

			Ui::AddSkip(content, 4);
			content->add(object_ptr<Ui::FlatLabel>(
				content,
				icon + (verdict.displayHost.isEmpty() ? verdict.host : verdict.displayHost),
				st::boxTitle));

			if (!verdict.flags.empty()) {
				for (const auto &f : verdict.flags) {
					content->add(object_ptr<Ui::FlatLabel>(
						content,
						u"  • "_q + f.text,
						st::defaultFlatLabel));
				}
			} else {
				content->add(object_ptr<Ui::FlatLabel>(
					content,
					u"  • Угроз не обнаружено"_q,
					st::boxLabel));
			}
			Ui::AddSkip(content, 4);
			Ui::AddDivider(content);
		}

		box->addButton(rpl::single(u"Понятно"_q), [=] {
			box->closeBox();
		});
	}));
}

void CopyCleanUrls(
		not_null<Window::SessionController*> controller,
		const std::vector<QString> &urls) {
	if (urls.empty()) {
		controller->showToast(u"Ссылок в сообщении нет"_q);
		return;
	}

	const auto &cfg = CurrentConfig();
	QStringList cleanList;
	int totalRemoved = 0;

	for (const auto &u : urls) {
		std::vector<QString> removed;
		cleanList.push_back(CleanUrl(u, cfg.linkGuardAggressive, &removed));
		totalRemoved += int(removed.size());
	}

	QGuiApplication::clipboard()->setText(cleanList.join('\n'));
	if (totalRemoved > 0) {
		IncrementCleaned(totalRemoved);
		controller->showToast(u"Скопировано без трекеров (убрано %1 меток)"_q.arg(totalRemoved));
	} else {
		controller->showToast(u"Ссылки скопированы (трекеров не найдено)"_q);
	}
}

// ------------------ Whitelist ------------------
void AddWhitelistDomain(const QString &domain) {
	const auto normalized = Registrable(domain.trimmed().toLower());
	if (normalized.isEmpty()) return;

	auto &cfg = ModifyConfig();
	for (const auto &d : cfg.linkGuardWhitelist) {
		if (d == normalized) return;
	}
	cfg.linkGuardWhitelist.push_back(normalized);
	SaveConfig();
}

void RemoveWhitelistDomain(const QString &domain) {
	auto &cfg = ModifyConfig();
	auto it = std::remove(cfg.linkGuardWhitelist.begin(), cfg.linkGuardWhitelist.end(), domain.trimmed().toLower());
	if (it != cfg.linkGuardWhitelist.end()) {
		cfg.linkGuardWhitelist.erase(it, cfg.linkGuardWhitelist.end());
		SaveConfig();
	}
}

bool IsWhitelisted(const QString &host) {
	const auto &cfg = CurrentConfig();
	const auto reg = Registrable(host.toLower());
	for (const auto &d : cfg.linkGuardWhitelist) {
		if (d == host || d == reg) return true;
	}
	return false;
}

std::vector<QString> Whitelist() {
	return CurrentConfig().linkGuardWhitelist;
}

// ------------------ Database Management & Background Fetch ------------------
void InitDatabase() {
	const auto path = DatabasePath();
	QFile f(path);
	if (f.open(QIODevice::ReadOnly)) {
		g_database = std::make_unique<DomainDatabase>(f.readAll());
	}

	if (!g_dbTimer) {
		g_dbTimer = new QTimer(QCoreApplication::instance());
		QObject::connect(g_dbTimer, &QTimer::timeout, [] {
			RefreshDatabase(false);
		});
		g_dbTimer->start(3 * 3600 * 1000); // Check every 3 hours
	}

	// Trigger initial background refresh check
	RefreshDatabase(false);
}

void RefreshDatabase(bool manual, Fn<void(bool success, int entries)> callback) {
	const auto &cfg = CurrentConfig();
	if (!cfg.linkGuardUseDatabase && !manual) {
		if (callback) callback(false, 0);
		return;
	}

	const auto manifestUrl = QString(kDbBaseUrl) + kDbManifestName;
	auto req = QNetworkRequest(QUrl(manifestUrl));
	req.setHeader(QNetworkRequest::UserAgentHeader, u"PixelGram/LinkGuard 1.0"_q);

	auto reply = GetNetworkManager()->get(req);
	QObject::connect(reply, &QNetworkReply::finished, [reply, manual, callback] {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			if (callback) callback(false, 0);
			return;
		}

		const auto doc = QJsonDocument::fromJson(reply->readAll());
		if (!doc.isObject()) {
			if (callback) callback(false, 0);
			return;
		}

		const auto builtStamp = doc.object()["built"].toString();
		auto &cfg = ModifyConfig();
		cfg.linkGuardDbCheckedAt = QDateTime::currentSecsSinceEpoch();

		if (!manual && !builtStamp.isEmpty() && builtStamp == cfg.linkGuardDbBuilt && g_database && g_database->isValid()) {
			SaveConfig();
			if (callback) callback(true, g_database->totalEntries());
			return;
		}

		// Download full.lgdb
		const auto dbUrl = QString(kDbBaseUrl) + kDbName;
		auto dbReq = QNetworkRequest(QUrl(dbUrl));
		dbReq.setHeader(QNetworkRequest::UserAgentHeader, u"PixelGram/LinkGuard 1.0"_q);

		auto dbReply = GetNetworkManager()->get(dbReq);
		QObject::connect(dbReply, &QNetworkReply::finished, [dbReply, builtStamp, callback] {
			dbReply->deleteLater();
			if (dbReply->error() != QNetworkReply::NoError) {
				if (callback) callback(false, 0);
				return;
			}

			const auto content = dbReply->readAll();
			auto newDb = std::make_unique<DomainDatabase>(content);
			if (!newDb->isValid()) {
				if (callback) callback(false, 0);
				return;
			}

			const int total = newDb->totalEntries();
			g_database = std::move(newDb);

			const auto path = DatabasePath();
			QDir().mkpath(QFileInfo(path).absolutePath());
			QFile f(path + u".part"_q);
			if (f.open(QIODevice::WriteOnly)) {
				f.write(content);
				f.close();
				QFile::remove(path);
				QFile::rename(path + u".part"_q, path);
			}

			auto &cfg = ModifyConfig();
			cfg.linkGuardDbBuilt = builtStamp;
			cfg.linkGuardDbEntries = total;
			SaveConfig();

			if (callback) callback(true, total);
		});
	});
}

QString DatabaseStatus(bool full) {
	if (!CurrentConfig().linkGuardUseDatabase) {
		return u"База выключена в настройках"_q;
	}
	if (!g_database || !g_database->isValid()) {
		return u"База ещё не загружена"_q;
	}
	const int total = g_database->totalEntries();
	const int days = g_database->ageDays();
	const auto ageText = (days < 1)
		? u"сегодня"_q
		: u"%1 дн. назад"_q.arg(days);

	if (full) {
		return u"В базе %1 записей, обновлена %2"_q.arg(total).arg(ageText);
	}
	return u"%1 записей · %2"_q.arg(total).arg(ageText);
}

// ------------------ Statistics ------------------
void ResetStats() {
	auto &cfg = ModifyConfig();
	cfg.linkGuardStatsCleaned = 0;
	cfg.linkGuardStatsWarned = 0;
	SaveConfig();
}

void IncrementCleaned(int count) {
	if (count <= 0) return;
	auto &cfg = ModifyConfig();
	cfg.linkGuardStatsCleaned += count;
	SaveConfig();
}

void IncrementWarned() {
	auto &cfg = ModifyConfig();
	cfg.linkGuardStatsWarned += 1;
	SaveConfig();
}

} // namespace PixelGram::LinkGuard
