#include "ayu/data/message_revisions.h"

#include "base/flat_map.h"
#include "boxes/abstract_box.h"
#include "history/history_item.h"
#include "ui/boxes/confirm_box.h"
#include "ui/vertical_list.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"

#include <QtCore/QDateTime>
#include <mutex>

namespace PixelGram::Revisions {
namespace {

std::mutex _revisionsMutex;
base::flat_map<FullMsgId, std::vector<Revision>> _revisions;

} // namespace

void RecordRevision(not_null<HistoryItem*> item) {
	if (item->isLocal()) {
		return;
	}
	const auto &msg = item->originalText();
	if (msg.empty()) {
		return;
	}

	std::unique_lock lock(_revisionsMutex);
	auto &list = _revisions[item->fullId()];
	if (!list.empty()) {
		const auto &last = list.back();
		if (last.text.text == msg.text) {
			return;
		}
	}
	list.push_back({ item->date(), msg });
}

bool HasRevisions(not_null<HistoryItem*> item) {
	std::unique_lock lock(_revisionsMutex);
	const auto it = _revisions.find(item->fullId());
	return (it != _revisions.end()) && !it->second.empty();
}

std::vector<Revision> GetRevisions(not_null<HistoryItem*> item) {
	std::unique_lock lock(_revisionsMutex);
	const auto it = _revisions.find(item->fullId());
	if (it != _revisions.end()) {
		return it->second;
	}
	return {};
}

void ShowHistoryBox(not_null<Window::SessionController*> controller, not_null<HistoryItem*> item) {
	const auto revs = GetRevisions(item);
	if (revs.empty()) {
		controller->showToast(QString::fromUtf8("История правок отсутствует."));
		return;
	}
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(QString::fromUtf8("История правок (%1)").arg(revs.size())));
		for (int i = int(revs.size()) - 1; i >= 0; --i) {
			const auto &rev = revs[i];
			const auto timeStr = QDateTime::fromSecsSinceEpoch(rev.date).toString(u"dd.MM.yyyy hh:mm:ss"_q);
			const auto row = box->addRow(object_ptr<Ui::VerticalLayout>(box));
			Ui::AddSubsectionTitle(row, rpl::single(QString::fromUtf8("Версия %1 • %2").arg(i + 1).arg(timeStr)));
			const auto label = row->add(object_ptr<Ui::FlatLabel>(
				row,
				rpl::single(rev.text),
				st::defaultFlatLabel));
			label->setSelectable(true);
			Ui::AddSkip(row, 6);
			Ui::AddDivider(row);
			Ui::AddSkip(row, 6);
		}
		box->addButton(rpl::single(QString::fromUtf8("Закрыть")), [=] { box->closeBox(); });
	}));
}

} // namespace PixelGram::Revisions
