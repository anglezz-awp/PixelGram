#pragma once

#include <QtCore/QPoint>
#include <QtWidgets/QWidget>

namespace PixelGram {

void InitFxOverlays(not_null<QWidget*> window);
void TriggerAnimTouRipple(const QPoint &globalPos);
void TriggerScrollMotionBlur(int dy);

} // namespace PixelGram
