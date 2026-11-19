#pragma once

#include <QtGui/QPainterPath>
#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtCore/QRectF>

namespace Ui {
class EmptyUserpic;
} // namespace Ui

namespace PixelGram {

enum class AvatarShape : int;

[[nodiscard]] QPainterPath AvatarShapePath(
	const QRectF &rect,
	AvatarShape shape,
	int cornerRadiusPercent = 50);

[[nodiscard]] QImage ApplyAvatarMask(
	QImage image,
	AvatarShape shape,
	int cornerRadiusPercent = 50);

void PaintEmptyUserpicCustom(
	QPainter &p,
	const Ui::EmptyUserpic *empty,
	int x,
	int y,
	int w,
	int h,
	AvatarShape shape,
	int cornerRadiusPercent = 50);

[[nodiscard]] uint32 AvatarShapeVersion();
void IncrementAvatarShapeVersion();

} // namespace PixelGram