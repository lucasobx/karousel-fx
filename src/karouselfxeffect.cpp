#include "karouselfxeffect.h"
#include "focusring.h"
#include "geometrychange.h"

#include <effect/effecthandler.h>

#include <QEasingCurve>

using namespace KWin;

template<typename Component, typename... Args>
static void reconfigureComponent(std::unique_ptr<Component> &component, bool enabled, Args &&...args)
{
    if (!enabled) {
        component.reset();
    } else if (component) {
        component->reconfigure();
    } else {
        component = std::make_unique<Component>(std::forward<Args>(args)...);
    }
}

KarouselFxEffect::KarouselFxEffect()
    : m_config(effects->config())
{
    reconfigure(ReconfigureAll);
}

KarouselFxEffect::~KarouselFxEffect() = default;

void KarouselFxEffect::reconfigure(ReconfigureFlags)
{
    m_config.read();

    reconfigureComponent(m_focusRing, m_config.focusRingEnabled(), m_config);
    reconfigureComponent(m_geometryChange, m_config.geometryChangeEnabled(), *this, m_config);
    if (m_focusRing && !m_geometryChange) {
        m_focusRing->resetPaintTransform();
    }
}

bool KarouselFxEffect::isActive() const
{
    return AnimationEffect::isActive() || (m_geometryChange && m_geometryChange->isAnimating());
}

void KarouselFxEffect::prePaintScreen(ScreenPrePaintData &data)
{
    if (m_geometryChange) {
        m_geometryChange->advance(data.view);
    }
    AnimationEffect::prePaintScreen(data);
}

void KarouselFxEffect::prePaintWindow(RenderView *view, EffectWindow *window, WindowPrePaintData &data)
{
    if (m_geometryChange && m_geometryChange->isAnimating(window)) {
        data.setTransformed();
    }
    AnimationEffect::prePaintWindow(view, window, data);
}

void KarouselFxEffect::paintWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *window, int mask,
                                   const Region &deviceRegion, WindowPaintData &data)
{
    if (m_geometryChange) {
        m_geometryChange->applyTransform(window, data);
    }
    AnimationEffect::paintWindow(renderTarget, viewport, window, mask, deviceRegion, data);
}

void KarouselFxEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *window, int mask,
                                  const Region &deviceRegion, WindowPaintData &data)
{
    if (m_capturing) {
        Effect::drawWindow(renderTarget, viewport, window, mask, deviceRegion, data);
        return;
    }
    if (m_focusRing && m_geometryChange && m_geometryChange->isAnimating(window)) {
        m_focusRing->applyPaintTransform(window, data);
    }
    AnimationEffect::drawWindow(renderTarget, viewport, window, mask, deviceRegion, data);
}

void KarouselFxEffect::postPaintScreen()
{
    if (m_geometryChange) {
        m_geometryChange->scheduleRepaints();
    }
    AnimationEffect::postPaintScreen();
}

void KarouselFxEffect::geometryAnimationStopped(EffectWindow *window)
{
    if (m_focusRing) {
        m_focusRing->resetPaintTransform(window);
    }
}

quint64 KarouselFxEffect::crossFade(EffectWindow *window, std::chrono::milliseconds duration)
{
    m_capturing = true;
    const quint64 animation = animate(window, CrossFadePrevious, 0, duration, FPx2(1), QEasingCurve(QEasingCurve::OutExpo), 0, FPx2(0));
    m_capturing = false;
    return animation;
}

bool KarouselFxEffect::supported()
{
    return effects->isOpenGLCompositing();
}
