#pragma once

#include "base/basic_types.h"
#include "ui/text/text_entity.h"
#include <vector>
#include <QtCore/QString>

namespace Window {
class SessionController;
} // namespace Window

class HistoryItem;

namespace PixelGram::Revisions {

struct Revision {
	TimeId date = 0;
	TextWithEntities text;
};

void RecordRevision(not_null<HistoryItem*> item);
[[nodiscard]] bool HasRevisions(not_null<HistoryItem*> item);
[[nodiscard]] std::vector<Revision> GetRevisions(not_null<HistoryItem*> item);
void ShowHistoryBox(not_null<Window::SessionController*> controller, not_null<HistoryItem*> item);

} // namespace PixelGram::Revisions
