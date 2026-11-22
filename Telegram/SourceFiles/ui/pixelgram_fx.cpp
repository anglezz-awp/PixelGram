#include "ui/pixelgram_fx.h"

#include "ui/rp_widget.h"
#include "ui/painter.h"
#include "ui/pixelgram_m3_scheme.h"
#include "settings/pixelgram_settings.h"
#include "ui/effects/animations.h"
#include "base/event_filter.h"

#include <vector>
#include <cmath>
#include <algorithm>

namespace PixelGram {
namespace {

class AnimTouOverlay final : public Ui::RpWidget {
public:
	AnimTouOverlay(not_null<QWidget*> parent)
	: RpWidget(parent) {
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setAttribute(Qt::WA_NoSystemBackground);
		resize(parent->size());
		show();

		base::install_event_filter(this, parent, [=](not_null<QEvent*> e) {
			if (e->type() == QEvent::Resize) {
				resize(parent->size());
			}
			return base::EventFilterResult::Continue;
		});
	}

	void addRipple(const QPoint &globalPos) {
		const auto pos = mapFromGlobal(globalPos);
		if (!rect().contains(pos)) {
			return;
		}

		auto ripple = std::make_shared<RippleData>();
		ripple->pos = pos;
		ripple->anim.start([=] {
			update();
		}, 0., 1., 380, anim::easeOutCirc);

		_ripples.push_back(ripple);
		update();
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		_ripples.erase(
			std::remove_if(_ripples.begin(), _ripples.end(), [](const auto &r) {
				return !r->anim.animating() && r->anim.value(1.) >= 1.;
			}),
			_ripples.end());

		if (_ripples.empty()) {
			return;
		}

		auto p = QPainter(this);
		PainterHighQualityEnabler hq(p);
		const auto &roles = CurrentM3Roles();

		for (const auto &ripple : _ripples) {
			const auto progress = ripple->anim.value(1.);
			if (progress >= 1.) {
				continue;
			}

			const auto baseRadius = 8. + 52. * progress;
			const auto alpha = (1. - progress);

			p.setBrush(Qt::NoBrush);

			// Chromatic Aberration Rings (inspired by animtou AGSL RuntimeShader)
			// Red channel shift (expansion * 1.06)
			{
				QColor redChan = roles.error;
				redChan.setAlphaF(std::clamp(alpha * 0.22, 0., 1.));
				p.setPen(QPen(redChan, 2.5));
				p.drawEllipse(QPointF(ripple->pos), baseRadius * 1.06, baseRadius * 1.06);
			}

			// Green / Center channel (primary M3 role)
			{
				QColor greenChan = roles.primary;
				greenChan.setAlphaF(std::clamp(alpha * 0.40, 0., 1.));
				p.setPen(QPen(greenChan, 2.0));
				p.drawEllipse(QPointF(ripple->pos), baseRadius, baseRadius);
			}

			// Blue channel shift (compression * 0.94)
			{
				QColor blueChan = roles.secondary;
				blueChan.setAlphaF(std::clamp(alpha * 0.25, 0., 1.));
				p.setPen(QPen(blueChan, 2.0));
				p.drawEllipse(QPointF(ripple->pos), baseRadius * 0.94, baseRadius * 0.94);
			}

			// Soft central liquid glow fill
			{
				QColor fill = roles.primaryContainer;
				fill.setAlphaF(std::clamp((1. - progress) * 0.15, 0., 1.));
				p.setPen(Qt::NoPen);
				p.setBrush(fill);
				p.drawEllipse(QPointF(ripple->pos), baseRadius * 0.7, baseRadius * 0.7);
			}
		}
	}

private:
	struct RippleData {
		QPoint pos;
		Ui::Animations::Simple anim;
	};

	std::vector<std::shared_ptr<RippleData>> _ripples;
};

AnimTouOverlay *GlobalAnimTou = nullptr;

} // namespace

void InitFxOverlays(not_null<QWidget*> window) {
	if (!GlobalAnimTou) {
		GlobalAnimTou = Ui::CreateChild<AnimTouOverlay>(window.get());
	}
}

void TriggerAnimTouRipple(const QPoint &globalPos) {
	if (GlobalAnimTou && CurrentConfig().animTouRipples) {
		GlobalAnimTou->addRipple(globalPos);
		GlobalAnimTou->raise();
	}
}

} // namespace PixelGram
