#include "karouselfxeffect.h"
#include "focusring.h"
#include "geometrychange.h"

#include <effect/effecthandler.h>

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

void KarouselFxEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *window, int mask,
                                  const Region &deviceRegion, WindowPaintData &data)
{
    if (m_focusRing && m_geometryChange && m_geometryChange->isAnimating(window)) {
        m_focusRing->applyPaintTransform(window, data);
    }
    AnimationEffect::drawWindow(renderTarget, viewport, window, mask, deviceRegion, data);
}

bool KarouselFxEffect::supported()
{
    return effects->isOpenGLCompositing();
}

void KarouselFxEffect::animationEnded(EffectWindow *window, Attribute attribute, uint meta)
{
    AnimationEffect::animationEnded(window, attribute, meta);
    if (m_geometryChange) {
        m_geometryChange->restoreForceBlurState(window);
        if (m_focusRing && !m_geometryChange->isAnimating(window)) {
            m_focusRing->resetPaintTransform(window);
        }
    }
}
