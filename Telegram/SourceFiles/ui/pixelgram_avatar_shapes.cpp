#include "ui/pixelgram_avatar_shapes.h"
#include "settings/pixelgram_settings.h"
#include "ui/empty_userpic.h"

#include <cmath>
#include <atomic>
#include <algorithm>

namespace PixelGram {
namespace {

std::atomic<uint32> globalAvatarShapeVersion = 1;

constexpr double kPi = 3.14159265358979323846;

} // namespace

uint32 AvatarShapeVersion() {
	return globalAvatarShapeVersion.load();
}

void IncrementAvatarShapeVersion() {
	globalAvatarShapeVersion.fetch_add(1);
}

QPainterPath AvatarShapePath(
		const QRectF &rect,
		AvatarShape shape,
		int cornerRadiusPercent) {
	QPainterPath path;
	const auto w = rect.width();
	const auto h = rect.height();
	if (w <= 0 || h <= 0) {
		return path;
	}
	const auto cx = rect.x() + w * 0.5;
	const auto cy = rect.y() + h * 0.5;
	const auto rx = w * 0.5;
	const auto ry = h * 0.5;

	switch (shape) {
	case AvatarShape::Circle: {
		path.addEllipse(rect);
		return path;
	}
	case AvatarShape::Custom: {
		const auto radius = std::min(rx, ry) * (std::clamp(cornerRadiusPercent, 0, 50) / 50.0);
		if (CurrentConfig().singleCornerRadius) {
			path.moveTo(rect.left() + radius, rect.top());
			path.lineTo(rect.right() - radius, rect.top());
			path.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + radius);
			path.lineTo(rect.right(), rect.bottom() - radius);
			path.quadTo(rect.right(), rect.bottom(), rect.right() - radius, rect.bottom());
			path.lineTo(rect.left(), rect.bottom());
			path.lineTo(rect.left(), rect.top() + radius);
			path.quadTo(rect.left(), rect.top(), rect.left() + radius, rect.top());
			path.closeSubpath();
			return path;
		}
		path.addRoundedRect(rect, radius, radius);
		return path;
	}
	case AvatarShape::Squircle: {
		constexpr int kSteps = 72;
		for (int i = 0; i <= kSteps; ++i) {
			const double theta = (2.0 * kPi * i) / kSteps;
			const double cosT = std::cos(theta);
			const double sinT = std::sin(theta);
			const double signX = (cosT >= 0.0) ? 1.0 : -1.0;
			const double signY = (sinT >= 0.0) ? 1.0 : -1.0;
			const double px = cx + signX * std::pow(std::abs(cosT), 0.5) * rx;
			const double py = cy + signY * std::pow(std::abs(sinT), 0.5) * ry;
			if (i == 0) {
				path.moveTo(px, py);
			} else {
				path.lineTo(px, py);
			}
		}
		path.closeSubpath();
		return path;
	}
	case AvatarShape::Cookie: {
		constexpr int kSteps = 96;
		constexpr int kPetals = 8;
		for (int i = 0; i <= kSteps; ++i) {
			const double theta = (2.0 * kPi * i) / kSteps;
			const double r = 0.88 + 0.12 * std::cos(kPetals * theta);
			const double px = cx + rx * r * std::cos(theta);
			const double py = cy + ry * r * std::sin(theta);
			if (i == 0) {
				path.moveTo(px, py);
			} else {
				path.lineTo(px, py);
			}
		}
		path.closeSubpath();
		return path;
	}
	case AvatarShape::Sunflower: {
		constexpr int kSteps = 120;
		constexpr int kPetals = 12;
		for (int i = 0; i <= kSteps; ++i) {
			const double theta = (2.0 * kPi * i) / kSteps;
			const double r = 0.91 + 0.09 * std::cos(kPetals * theta);
			const double px = cx + rx * r * std::cos(theta);
			const double py = cy + ry * r * std::sin(theta);
			if (i == 0) {
				path.moveTo(px, py);
			} else {
				path.lineTo(px, py);
			}
		}
		path.closeSubpath();
		return path;
	}
	case AvatarShape::Clover: {
		constexpr int kSteps = 96;
		constexpr int kPetals = 4;
		for (int i = 0; i <= kSteps; ++i) {
			const double theta = (2.0 * kPi * i) / kSteps;
			const double r = 0.82 + 0.18 * std::cos(kPetals * theta);
			const double px = cx + rx * r * std::cos(theta);
			const double py = cy + ry * r * std::sin(theta);
			if (i == 0) {
				path.moveTo(px, py);
			} else {
				path.lineTo(px, py);
			}
		}
		path.closeSubpath();
		return path;
	}
	case AvatarShape::Hexagon: {
		constexpr int kSteps = 96;
		constexpr int kPetals = 6;
		for (int i = 0; i <= kSteps; ++i) {
			const double theta = (2.0 * kPi * i) / kSteps;
			const double r = 0.88 + 0.12 * std::cos(kPetals * theta);
			const double px = cx + rx * r * std::cos(theta);
			const double py = cy + ry * r * std::sin(theta);
			if (i == 0) {
				path.moveTo(px, py);
			} else {
				path.lineTo(px, py);
			}
		}
		path.closeSubpath();
		return path;
	}
	}
	path.addEllipse(rect);
	return path;
}

QImage ApplyAvatarMask(
		QImage image,
		AvatarShape shape,
		int cornerRadiusPercent) {
	if (image.isNull()) {
		return image;
	}
	const auto size = image.size();
	auto result = QImage(size, QImage::Format_ARGB32_Premultiplied);
	result.fill(Qt::transparent);

	auto p = QPainter(&result);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);
	const auto path = AvatarShapePath(
		QRectF(0, 0, size.width(), size.height()),
		shape,
		cornerRadiusPercent);
	p.setClipPath(path);
	p.drawImage(0, 0, image);
	p.end();

	return result;
}

void PaintEmptyUserpicCustom(
		QPainter &p,
		const Ui::EmptyUserpic *empty,
		int x,
		int y,
		int w,
		int h,
		AvatarShape shape,
		int cornerRadiusPercent) {
	if (!empty) {
		return;
	}
	p.save();
	p.setRenderHint(QPainter::Antialiasing);
	const auto path = AvatarShapePath(
		QRectF(x, y, w, h),
		shape,
		cornerRadiusPercent);
	p.setClipPath(path);
	empty->paintSquare(p, x, y, w, h);
	p.restore();
}

} // namespace PixelGram