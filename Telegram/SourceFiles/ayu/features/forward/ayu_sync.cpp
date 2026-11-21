// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/forward/ayu_sync.h"

#include "api/api_common.h"
#include "api/api_sending.h"
#include "apiwrap.h"
#include "ayu/utils/telegram_helpers.h"
#include "base/base_file_utilities.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "core/file_utilities.h"
#include "data/data_channel.h"
#include "data/data_document.h"
#include "data/data_file_origin.h"
#include "data/data_peer.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_helpers.h"
#include "main/main_session.h"
#include "storage/localimageloader.h"

#include <algorithm>
#include <mutex>
#include <utility>

namespace AyuSync {
namespace {

constexpr auto kDocumentDownloadTimeout = std::chrono::minutes(15);
constexpr auto kDownloadWaitStep = std::chrono::milliseconds(100);

enum class DownloadWaitResult {
	Completed,
	Cancelled,
	TimedOut,
};

struct ActiveDocumentDownload {
	QString path;
	QString readyPath;
	TimedCountDownLatch done{ 1 };
	rpl::lifetime lifetime;
	int consumers = 1;
	bool allowSizeFallback = false;
};

struct DocumentDownloadRegistration {
	std::shared_ptr<ActiveDocumentDownload> state;
	bool owner = false;
};

using ActiveDocumentDownloads = base::flat_map<
	DocumentData*,
	std::shared_ptr<ActiveDocumentDownload>>;

[[nodiscard]] std::mutex &DocumentDownloadsMutex() {
	static auto result = std::mutex();
	return result;
}

[[nodiscard]] ActiveDocumentDownloads &DocumentDownloads() {
	static auto result = ActiveDocumentDownloads();
	return result;
}

[[nodiscard]] DocumentDownloadRegistration RegisterDocumentDownload(
		not_null<DocumentData*> document) {
	const auto lock = std::lock_guard(DocumentDownloadsMutex());
	auto &active = DocumentDownloads();
	const auto i = active.find(document);
	if (i != active.end()) {
		++i->second->consumers;
		return { i->second, false };
	}
	auto state = std::make_shared<ActiveDocumentDownload>();
	active.emplace(document, state);
	return { std::move(state), true };
}

void FinishDocumentDownload(
		DocumentData *document,
		const std::shared_ptr<ActiveDocumentDownload> &state) {
	{
		const auto lock = std::lock_guard(DocumentDownloadsMutex());
		auto &active = DocumentDownloads();
		const auto i = active.find(document);
		if (i != active.end() && i->second == state) {
			active.erase(i);
		}
	}
	state->done.countDown();
}

void ReleaseDocumentDownloadConsumer(
		DocumentData *document,
		const std::shared_ptr<ActiveDocumentDownload> &state) {
	auto released = false;
	{
		const auto lock = std::lock_guard(DocumentDownloadsMutex());
		Expects(state->consumers > 0);
		--state->consumers;
		if (state->consumers == 0) {
			auto &active = DocumentDownloads();
			const auto i = active.find(document);
			if (i != active.end() && i->second == state) {
				active.erase(i);
				released = true;
			}
		}
	}
	if (released) {
		state->done.countDown();
	}
}

[[nodiscard]] DownloadWaitResult WaitForDownload(
		TimedCountDownLatch &done,
		std::chrono::milliseconds timeout,
		const Fn<bool()> &cancelled) {
	const auto deadline = std::chrono::steady_clock::now() + timeout;
	while (true) {
		if (done.await(std::chrono::milliseconds(0))) {
			return DownloadWaitResult::Completed;
		}
		if (cancelled && cancelled()) {
			return DownloadWaitResult::Cancelled;
		}
		const auto remaining = std::chrono::duration_cast<
			std::chrono::milliseconds>(
			deadline - std::chrono::steady_clock::now());
		if (remaining <= std::chrono::milliseconds(0)) {
			return DownloadWaitResult::TimedOut;
		}
		if (done.await(std::min(
				remaining,
				kDownloadWaitStep))) {
			return DownloadWaitResult::Completed;
		}
	}
}

[[nodiscard]] bool SameFilePath(const QString &left, const QString &right) {
	const auto leftInfo = QFileInfo(left);
	const auto rightInfo = QFileInfo(right);
	const auto leftCanonical = leftInfo.canonicalFilePath();
	const auto rightCanonical = rightInfo.canonicalFilePath();
	if (!leftCanonical.isEmpty() && !rightCanonical.isEmpty()) {
		return leftCanonical == rightCanonical;
	}
#ifdef Q_OS_WIN
	return leftInfo.absoluteFilePath().compare(
		rightInfo.absoluteFilePath(),
		Qt::CaseInsensitive) == 0;
#else // Q_OS_WIN
	return leftInfo.absoluteFilePath() == rightInfo.absoluteFilePath();
#endif // !Q_OS_WIN
}

[[nodiscard]] bool FileHasExactSize(const QString &path, int64 expectedSize) {
	const auto info = QFileInfo(path);
	return !path.isEmpty()
		&& info.isFile()
		&& info.size() == expectedSize;
}

[[nodiscard]] QString LoadedDocumentPath(not_null<DocumentData*> document) {
	auto result = QString();
	crl::on_main_sync([&] {
		result = document->filepath(true);
	});
	return result;
}

[[nodiscard]] bool DocumentReady(
		not_null<DocumentData*> document,
		const QString &path) {
	const auto loaded = document->filepath(true);
	return !path.isEmpty()
		&& !loaded.isEmpty()
		&& SameFilePath(path, loaded)
		&& FileHasExactSize(path, document->size);
}

void CompleteDocumentDownload(
		not_null<DocumentData*> document,
		const std::shared_ptr<ActiveDocumentDownload> &state) {
	if (document->loading()) {
		return;
	}
	const auto released = state->done.await(std::chrono::milliseconds(0));
	const auto ready = state->allowSizeFallback
		? FileHasExactSize(state->path, document->size)
		: (document->status != FileDownloadFailed
			&& !document->cancelled()
			&& DocumentReady(document, state->path));
	if (!ready) {
		QFile::remove(state->path);
		state->path.clear();
	} else if (!released) {
		state->readyPath = state->path;
	}
	state->lifetime.destroy();
	if (!released) {
		FinishDocumentDownload(document, state);
	}
}

[[nodiscard]] QString GeneratedDocumentName(
		not_null<DocumentData*> document,
		const QString &prefix,
		const QString &extension) {
	return prefix
		+ QString::number(document->getDC())
		+ u"_"_q
		+ QString::number(document->id)
		+ extension;
}

[[nodiscard]] QString DocumentFileName(not_null<DocumentData*> document) {
	if (!document->filename().isEmpty()) {
		return base::FileNameFromUserString(document->filename());
	}
	if (document->isVoiceMessage()) {
		return GeneratedDocumentName(document, u"audio_"_q, u".ogg"_q);
	}
	if (document->isVideoMessage()) {
		return GeneratedDocumentName(document, u"round_"_q, u".mp4"_q);
	}
	if (document->isGifv()) {
		return GeneratedDocumentName(document, u"gif_"_q, u".gif"_q);
	}
	if (document->isVideoFile()) {
		return GeneratedDocumentName(document, u"video_"_q, u".mp4"_q);
	}
	return {};
}

[[nodiscard]] QString NextDocumentPath(
		not_null<Main::Session*> session,
		not_null<DocumentData*> document) {
	const auto directory = pathForSave(session);
	const auto filename = DocumentFileName(document);
	if (directory.isEmpty() || filename.isEmpty()) {
		return {};
	}
	return filedialogNextFilename(filename, QString(), directory);
}

} // namespace

QString pathForSave(not_null<Main::Session*> session) {
	auto path = Core::App().settings().downloadPath();
	if (path.isEmpty()) {
		return File::DefaultDownloadPath(session);
	}
	if (path == FileDialog::Tmp()) {
		return session->local().tempDirectory();
	}
	return path;
}

QString documentFileName(not_null<DocumentData*> document) {
	return DocumentFileName(document);
}

QString filePath(not_null<Main::Session*> session, not_null<PhotoData*> photo) {
	const auto directory = pathForSave(session);
	if (directory.isEmpty()) {
		return {};
	}
	const auto filename = QString::number(photo->getDC())
		+ u"_"_q
		+ QString::number(photo->id)
		+ u".jpg"_q;
	return QDir(directory).filePath(filename);
}

qint64 fileSize(const QString &path) {
	if (path.isEmpty()) {
		return 0;
	}
	QFile file(path);
	return file.exists() ? file.size() : 0;
}

DocumentPaths loadDocuments(
		not_null<Main::Session*> session,
		const std::vector<not_null<HistoryItem*>> &items,
		const Fn<bool()> &cancelled) {
	auto result = DocumentPaths();
	for (const auto &item : items) {
		if (cancelled && cancelled()) {
			break;
		}
		if (const auto data = item->media()->document()) {
			const auto path = loadDocumentSync(
				session,
				data,
				item->fullId(),
				cancelled);
			if (!path.isEmpty()) {
				result.emplace(data, path);
			}
		} else if (auto photo = item->media()->photo()) {
			if (fileSize(filePath(session, photo))
					== photo->imageByteSize(Data::PhotoSize::Large)) {
				continue;
			}

			loadPhotoSync(
				session,
				photo,
				item->fullId(),
				cancelled);
		}
	}
	return result;
}

QString loadDocumentSync(
		not_null<Main::Session*> session,
		not_null<DocumentData*> data,
		Data::FileOrigin origin,
		const Fn<bool()> &cancelled) {
	if (cancelled && cancelled()) {
		return {};
	}
	const auto expectedSize = data->size;
	const auto registration = RegisterDocumentDownload(data);
	const auto releaseConsumer = gsl::finally([&] {
		ReleaseDocumentDownloadConsumer(data, registration.state);
	});
	if (!registration.owner) {
		const auto waitResult = WaitForDownload(
				registration.state->done,
				kDocumentDownloadTimeout,
				cancelled);
		if (waitResult != DownloadWaitResult::Completed) {
			return {};
		}
		const auto path = registration.state->readyPath;
		return FileHasExactSize(path, expectedSize) ? path : QString();
	}
	auto registrationActive = true;
	auto reserved = false;
	const auto cleanup = gsl::finally([&] {
		if (!registrationActive) {
			return;
		}
		if (reserved) {
			QFile::remove(registration.state->path);
		}
		registration.state->path.clear();
		FinishDocumentDownload(data, registration.state);
	});
	auto path = LoadedDocumentPath(data);
	if (FileHasExactSize(path, expectedSize)) {
		registration.state->path = path;
		registration.state->readyPath = path;
		registrationActive = false;
		FinishDocumentDownload(data, registration.state);
		return path;
	}
	while (true) {
		path = NextDocumentPath(session, data);
		if (path.isEmpty()) {
			return {};
		}
		if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
			return {};
		}
		auto reservation = QFile(path);
		if (reservation.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
			reservation.close();
			reserved = true;
			break;
		}
		if (!QFile::exists(path)) {
			return {};
		}
	}
	registration.state->path = path;
	if (cancelled && cancelled()) {
		return {};
	}

	const auto state = registration.state;
	const auto documentId = data->id;
	crl::on_main_sync([=] {
		data->save(origin, path);
		state->allowSizeFallback = !data->loading();
		if (!data->loading()) {
			CompleteDocumentDownload(data, state);
			return;
		}

		session->data().documentLoadProgress(
		) | rpl::filter([=](not_null<DocumentData*> changed) {
			return changed->id == documentId
				&& !changed->loading();
		}) | rpl::on_next([=](not_null<DocumentData*> changed) {
			CompleteDocumentDownload(changed, state);
		}, state->lifetime);
	});
	registrationActive = false;
	const auto waitResult = WaitForDownload(
			registration.state->done,
			kDocumentDownloadTimeout,
			cancelled);
	if (waitResult != DownloadWaitResult::Completed) {
		return {};
	}
	path = registration.state->readyPath;
	return FileHasExactSize(path, expectedSize) ? path : QString();
}

void forwardMessagesSync(not_null<Main::Session*> session,
						 const std::vector<not_null<HistoryItem*>> &items,
						 const ApiWrap::SendAction &action,
						 Data::ForwardOptions options) {
	auto latch = std::make_shared<TimedCountDownLatch>(1);

	crl::on_main([=]
	{
		session->api().forwardMessages(Data::ResolvedForwardDraft(items, options),
									   action,
									   [=]
									   {
										   latch->countDown();
									   });
	});

	latch->await(std::chrono::minutes(1));
}

void loadPhotoSync(
		not_null<Main::Session*> session,
		not_null<PhotoData*> photo,
		Data::FileOrigin origin,
		const Fn<bool()> &cancelled) {
	const auto path = pathForSave(session);
	if (path.isEmpty() || !QDir().mkpath(path)) {
		return;
	}

	auto latch = std::make_shared<TimedCountDownLatch>(1);
	auto lifetime = std::make_shared<rpl::lifetime>();

	crl::on_main([=]
	{
		const auto view = photo->createMediaView();
		if (!view) {
			latch->countDown();
			return;
		}
		view->wanted(Data::PhotoSize::Large, origin);

		const auto saveToFiles = [=]
		{
			view->saveToFile(filePath(session, photo));
		};

		if (view->loaded()) {
			saveToFiles();
			latch->countDown();
			return;
		}

		session->downloaderTaskFinished() | rpl::filter([=]
		{
			return view->loaded();
		}) | rpl::on_next([=]
		{
			saveToFiles();
			latch->countDown();
		}, *lifetime);
	});

	(void)WaitForDownload(*latch, std::chrono::minutes(5), cancelled);
	crl::on_main([lifetime = base::take(lifetime)]
	{
		lifetime->destroy();
	});
}

void sendMessageSync(not_null<Main::Session*> session, Api::MessageToSend &&message) {
	const auto action = message.action;
	crl::on_main([=, message = std::move(message)]() mutable
	{
		session->api().sendMessage(std::move(message));
	});

	waitForMsgSync(session, action);
}

void waitForMsgSync(not_null<Main::Session*> session, const Api::SendAction &action) {
	auto latch = std::make_shared<TimedCountDownLatch>(1);
	auto lifetime = std::make_shared<rpl::lifetime>();

	crl::on_main([=]
	{
		session->data().itemIdChanged()
			| rpl::filter([=](const Data::Session::IdChange &update)
			{
				return action.history->peer->id == update.newId.peer;
			}) | rpl::on_next([=]
			{
				latch->countDown();
			}, *lifetime);
	});

	latch->await(std::chrono::minutes(5));
	crl::on_main([lifetime = base::take(lifetime)]
	{
		lifetime->destroy();
	});
}

void sendDocumentSync(not_null<Main::Session*> session,
					  Ui::PreparedGroup &group,
					  SendMediaType type,
					  TextWithTags &&caption,
					  const Api::SendAction &action) {
	auto groupId = std::make_shared<SendingAlbum>();
	groupId->groupId = base::RandomValue<uint64>();

	crl::on_main([=, lst = std::move(group.list), caption = std::move(caption)]() mutable
	{
		auto size = lst.files.size();
		if (!lst.files.empty()) {
			lst.files.front().caption = std::move(caption);
		}
		session->api().sendFiles(
			std::move(lst),
			type,
			size > 1 ? groupId : nullptr,
			action);
	});

	waitForMsgSync(session, action);
}

void sendStickerSync(not_null<Main::Session*> session,
					 Api::MessageToSend &&message,
					 not_null<DocumentData*> document) {
	const auto action = message.action;
	crl::on_main([=, message = std::move(message)]() mutable
	{
		Api::SendExistingDocument(std::move(message), document, std::nullopt);
	});

	waitForMsgSync(session, action);
}

void sendVoiceSync(not_null<Main::Session*> session,
				   const QByteArray &data,
				   int64_t duration,
				   bool video,
				   Api::MessageToSend &&message) {
	const auto action = message.action;

	crl::on_main([=]
	{
		const auto to = FileLoadTo(
			action.history->peer->id,
			action.options,
			action.replyTo,
			action.replaceMediaOf);
		session->api().fileLoader()->addTask(std::make_unique<FileLoadTask>(FileLoadTask::VoiceArgs{
			.session = session,
			.voice = data,
			.duration = duration,
			.waveform = VoiceWaveform(),
			.video = video,
			.to = to,
			.caption = message.textWithTags
		}));
	});
	waitForMsgSync(session, action);
}

} // namespace AyuSync
