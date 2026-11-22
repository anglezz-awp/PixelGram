/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "history/view/controls/history_view_voice_record_button.h"

#include "lottie/lottie_icon.h"
#include "settings/pixelgram_settings.h"
#include "ui/paint/blobs.h"
#include "ui/painter.h"
#include "styles/style_chat_helpers.h"
#include "lang/lang_keys.h"

#include <QtMath>

namespace HistoryView::Controls {

namespace {

constexpr auto kMaxLevel = 1800.;
constexpr auto kBlobAlpha = 76. / 255.;
constexpr auto kBlobMaxSpeed = 5.0;
constexpr auto kLevelDuration = 100. + 500. * 0.33;
constexpr auto kBlobsScaleEnterDuration = crl::time(250);
constexpr auto kVoiceIconIndex = 0;
constexpr auto kRoundIconIndex = 1;

auto Blobs() {
	return std::vector<Ui::Paint::Blobs::BlobData>{
		{
			.segmentsCount = 9,
			.minScale = 0.605229,
			.minRadius = (float)st::historyRecordMinorBlobMinRadius,
			.maxRadius = (float)st::historyRecordMinorBlobMaxRadius,
			.speedScale = 1.,
			.alpha = kBlobAlpha,
			.maxSpeed = kBlobMaxSpeed,
		},
		{
			.segmentsCount = 12,
			.minScale = 0.553943,
			.minRadius = (float)st::historyRecordMajorBlobMinRadius,
			.maxRadius = (float)st::historyRecordMajorBlobMaxRadius,
			.speedScale = 1.,
			.alpha = kBlobAlpha,
			.maxSpeed = kBlobMaxSpeed,
		},
	};
}

} // namespace

VoiceRecordButton::VoiceRecordButton(
	not_null<Ui::RpWidget*> parent,
	const style::RecordBar &st)
: AbstractButton(parent)
, _blobs(std::make_unique<Ui::Paint::Blobs>(
	Blobs(),
	kLevelDuration,
	kMaxLevel))
, _center(_blobs->maxRadius()) {
	resize(_center * 2, _center * 2);
	init();
}

VoiceRecordButton::~VoiceRecordButton() = default;

void VoiceRecordButton::requestPaintLevel(quint16 level) {
	if (_blobsHideLastTime) {
		 return;
	}
	_blobs->setLevel(level);
	update();
}

void VoiceRecordButton::init() {
	const auto currentState = lifetime().make_state<Type>(_state.current());

	rpl::single(
		anim::Disabled()
	) | rpl::then(
		anim::Disables()
	) | rpl::on_next([=](bool hide) {
		if (hide) {
			_blobs->setLevel(0.);
		}
		_blobsHideLastTime = hide ? crl::now() : 0;
		if (!hide && !_animation.animating() && isVisible()) {
			_animation.start();
		}
	}, lifetime());

	const auto &mainRadiusMin = st::historyRecordMainBlobMinRadius;
	const auto mainRadiusDiff = st::historyRecordMainBlobMaxRadius
		- mainRadiusMin;
	paintRequest(
	) | rpl::on_next([=](const QRect &clip) {
		auto p = QPainter(this);

		const auto hideProgress = _blobsHideLastTime
			? 1. - std::clamp(
				((crl::now() - _blobsHideLastTime)
					/ (float64)kBlobsScaleEnterDuration),
				0.,
				1.)
			: 1.;
		const auto showProgress = _showProgress.current();
		const auto complete = (showProgress == 1.);

		p.translate(_center, _center);
		PainterHighQualityEnabler hq(p);
		const auto brush = QBrush(anim::color(
			st::historyRecordVoiceFgInactive,
			st::historyRecordVoiceFgActive,
			_colorProgress));

		const auto radius = (mainRadiusMin
			+ (mainRadiusDiff * _blobs->currentLevel())) * showProgress;

		p.setPen(Qt::NoPen);
		p.setBrush(brush);

		const auto &config = PixelGram::CurrentConfig();
		if (config.dynamicVoiceMorph && radius > 0.1) {
			if (!_animation.animating()) {
				const_cast<VoiceRecordButton*>(this)->_animation.start();
			}
			const auto now = crl::now();
			const auto phase = (now % 360000) * (2.0 * 3.14159265358979323846 / 2800.0);
			const auto level = _blobs ? float(_blobs->currentLevel()) : 0.0f;
			const auto amplitude = 0.15f + 0.08f * std::clamp(level, 0.0f, 1.0f);
			constexpr int steps = 96;
			QPainterPath path;
			for (int i = 0; i <= steps; ++i) {
				const auto theta = (i * 2.0 * 3.14159265358979323846 / steps);
				const auto wave = std::cos(8.0 * theta - phase);
				const auto subWave = 0.15 * std::cos(16.0 * theta - 2.0 * phase);
				const auto r = radius * (1.0 + amplitude * (wave + subWave));
				const auto x = r * std::cos(theta);
				const auto y = r * std::sin(theta);
				if (i == 0) {
					path.moveTo(x, y);
				} else {
					path.lineTo(x, y);
				}
			}
			path.closeSubpath();
			p.fillPath(path, brush);
		} else {
			if (!config.dynamicVoiceMorph) {
				_blobs->paint(p, brush, showProgress * hideProgress);
			}
			p.drawEllipse(QPointF(), radius, radius);
		}

		if (!complete) {
			p.setOpacity(showProgress);
		}

		// Paint icon.
		{
			const auto stateProgress = _stateChangedAnimation.value(0.);
			const auto scale = (std::cos(M_PI * 2 * stateProgress) + 1.) * .5;
			if (scale < 1.) {
				p.scale(scale, scale);
			}
			const auto state = *currentState;
			if (state == Type::Send) {
				const auto icon = st::historySendIcon;
				const auto position = st::historyRecordSendIconPosition;
				icon.paint(
					p,
					-icon.width() / 2 + position.x(),
					-icon.height() / 2 + position.y(),
					0,
					st::historyRecordVoiceFgActiveIcon->c);
			} else {
				const auto index = (state == Type::Record)
					? kVoiceIconIndex
					: kRoundIconIndex;
				const auto &icon = _voiceRoundIcons[index];
				if (!icon) {
					initVoiceRoundIcon(index);
				}
				icon->paintInCenter(
					p,
					rect().translated(-_center, -_center),
					st::historyRecordVoiceFgActiveIcon->c);
			}
		}
	}, lifetime());

	_animation.init([=](crl::time now) {
		const auto dt = (_lastUpdateTime > 0)
			? std::min(now - _lastUpdateTime, crl::time(100))
			: 16;
		_lastUpdateTime = now;

		if (const auto &last = _blobsHideLastTime; (last > 0)
			&& (now - last >= kBlobsScaleEnterDuration)) {
			const auto &cfg = PixelGram::CurrentConfig();
			if (!cfg.dynamicVoiceMorph) {
				_animation.stop();
				return false;
			}
		}
		_blobs->updateLevel(dt);
		update();
		return true;
	});

	rpl::merge(
		shownValue(),
		_showProgress.value(
		) | rpl::map(rpl::mappers::_1 != 0.) | rpl::distinct_until_changed()
	) | rpl::on_next([=](bool show) {
		setVisible(show);
		setMouseTracking(show);
		if (!show) {
			_animation.stop();
			_showProgress = 0.;
			_blobs->resetLevel();
			_state = Type::Record;
		} else {
			if (!_animation.animating()) {
				_animation.start();
			}
		}
	}, lifetime());

	actives(
	) | rpl::distinct_until_changed(
	) | rpl::on_next([=](bool active) {
		setPointerCursor(active);
	}, lifetime());

	_state.changes(
	) | rpl::on_next([=](Type newState) {
		const auto to = 1.;
		auto callback = [=](float64 value) {
			if (value >= (to * .5)) {
				*currentState = newState;
			}
			update();
		};
		constexpr auto kDuration = st::universalDuration * 2;
		_stateChangedAnimation.start(std::move(callback), 0., to, kDuration);
	}, lifetime());
}

void VoiceRecordButton::initVoiceRoundIcon(int index) {
	Expects(index >= 0 && index < 2);

	_voiceRoundIcons[index] = Lottie::MakeIcon({
		.path = ((index == kVoiceIconIndex)
			? u":/animations/chat/voice_to_video.tgs"_q
			: u":/animations/chat/video_to_voice.tgs"_q),
		.sizeOverride = st::historySend.recordSize,
		.colorizeUsingAlpha = true,
	});
}

rpl::producer<bool> VoiceRecordButton::actives() const {
	return events(
	) | rpl::filter([=](not_null<QEvent*> e) {
		return (e->type() == QEvent::MouseMove
			|| e->type() == QEvent::Leave
			|| e->type() == QEvent::Enter);
	}) | rpl::map([=](not_null<QEvent*> e) {
		switch(e->type()) {
		case QEvent::MouseMove:
			return inCircle((static_cast<QMouseEvent*>(e.get()))->pos());
		case QEvent::Leave: return false;
		case QEvent::Enter: return inCircle(mapFromGlobal(QCursor::pos()));
		default: return false;
		}
	});
}

rpl::producer<> VoiceRecordButton::clicks() const {
	return Ui::AbstractButton::clicks(
	) | rpl::to_empty | rpl::filter([=] {
		return inCircle(mapFromGlobal(QCursor::pos()));
	});
}

bool VoiceRecordButton::inCircle(const QPoint &localPos) const {
	const auto &radii = st::historyRecordMainBlobMaxRadius;
	const auto dx = std::abs(localPos.x() - _center);
	if (dx > radii) {
		return false;
	}
	const auto dy = std::abs(localPos.y() - _center);
	if (dy > radii) {
		return false;
	} else if (dx + dy <= radii) {
		return true;
	}
	return ((dx * dx + dy * dy) <= (radii * radii));
}

void VoiceRecordButton::requestPaintProgress(float64 progress) {
	_showProgress = progress;
	if (progress > 0. && !_animation.animating()) {
		_animation.start();
	}
	update();
}

void VoiceRecordButton::requestPaintColor(float64 progress) {
	if (_colorProgress == progress) {
		return;
	}
	_colorProgress = progress;
	update();
}

void VoiceRecordButton::setType(Type state) {
	_state = state;
	if (!_animation.animating()) {
		_animation.start();
	}

	setAccessibleName([&] {
		switch (state) {
		case Type::Send:
			return tr::lng_send_button(tr::now);
		case Type::Record:
			return tr::lng_send_action_record_round(tr::now);
		case Type::Round:
			return tr::lng_send_action_record_round(tr::now);
		}
		Unexpected("Voice record button type.");
	}());

}

} // namespace HistoryView::Controls
