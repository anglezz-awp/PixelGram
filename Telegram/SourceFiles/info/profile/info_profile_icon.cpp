/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "info/profile/info_profile_icon.h"

namespace Info {
namespace Profile {

FloatingIcon::FloatingIcon(
	RpWidget *parent,
	const style::icon &icon,
	QPoint position)
: FloatingIcon(parent, icon, position, Tag{}) {
}

FloatingIcon::FloatingIcon(
	RpWidget *parent,
	const style::icon &icon,
	QPoint position,
	const Tag &)
: RpWidget(parent)
, _icon(&icon)
, _point(position) {
	setAttribute(Qt::WA_TransparentForMouseEvents);
	if (parent) {
		parent->sizeValue(
		) | rpl::on_next([=](QSize s) {
			setGeometry(0, 0, s.width(), s.height());
		}, lifetime());
	} else {
		setGeometry(QRect(
			QPoint(0, 0),
			QSize(_point.x() + _icon->width(), _point.y() + _icon->height())));
	}
}

void FloatingIcon::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	const auto iconY = parentWidget()
		? (parentWidget()->height() - _icon->height()) / 2
		: _point.y();
	_icon->paint(p, _point.x(), iconY, width());
}

} // namespace Profile
} // namespace Info
