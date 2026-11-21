// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/utils/telegram_helpers.h"

#include "settings/pixelgram_settings.h"
#include "main/main_session.h"
#include "apiwrap.h"
#include "data/data_histories.h"
#include "data/data_session.h"
#include "data/data_channel.h"
#include "history/history.h"
#include "data/data_media_types.h"
#include "data/data_poll.h"
#include "history/history_item.h"
#include "ui/text/text_utilities.h"

TextWithTags extractText(not_null<HistoryItem*> item) {
	TextWithTags result;

	QString text;
	if (const auto media = item->media()) {
		if (const auto poll = media->poll()) {
			text.append(u"\U0001F4CA "_q)
				.append(poll->question.text).append(u"\n"_q);
			for (const auto &answer : poll->answers) {
				text.append(u"• "_q).append(answer.text.text).append(u"\n"_q);
			}
		}
	}

	result.tags = TextUtilities::ConvertEntitiesToTextTags(item->originalText().entities);
	result.text = text.isEmpty() ? item->originalText().text : text;
	return result;
}

bool mediaDownloadable(const Data::Media *media) {
	if (!media
		|| media->webpage() || media->poll() || media->game()
		|| media->invoice() || media->location() || media->paper()
		|| media->giveawayStart() || media->giveawayResults()
		|| media->sharedContact() || media->call()
	) {
		return false;
	}
	return true;
}

void readHistory(not_null<HistoryItem*> message) {
	const auto history = message->history();
	const auto tillId = message->id;

	history->session().data().histories().sendRequest(
		history,
		Data::Histories::RequestType::ReadInbox,
		[=](Fn<void()> finish) {
			if (const auto channel = history->peer->asChannel()) {
				return history->session().api().request(MTPchannels_ReadHistory(
					channel->inputChannel(),
					MTP_int(tillId)
				)).done([=] {
					finish();
				}).fail([=] {
					finish();
				}).send();
			}

			return history->session().api().request(MTPmessages_ReadHistory(
				history->peer->input(),
				MTP_int(tillId)
			)).done([=](const MTPmessages_AffectedMessages &result) {
				history->session().api().applyAffectedMessages(history->peer, result);
				finish();
			}).fail([=] {
				finish();
			}).send();
		});
}

void markReadAfterAction(not_null<History*> history) {
	const auto &cfg = PixelGram::CurrentConfig();
	if (!cfg.dontReadMessages || !cfg.markReadAfterAction) {
		return;
	}
	if (const auto last = history->lastServerMessage()) {
		readHistory(last);
	}
}
