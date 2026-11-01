#pragma once

#include "karouselfxconfig.h"

#include <effect/animationeffect.h>

#include <memory>

class FocusRing;
class GeometryChange;

class KarouselFxEffect : public KWin::AnimationEffect
{
    Q_OBJECT

public:
    KarouselFxEffect();
    ~KarouselFxEffect() override;

    void reconfigure(ReconfigureFlags flags) override;
    bool isActive() const override;
    void prePaintScreen(KWin::ScreenPrePaintData &data) override;
    void prePaintWindow(KWin::RenderView *view, KWin::EffectWindow *window, KWin::WindowPrePaintData &data) override;
    bool paintWindow(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, KWin::EffectWindow *window, int mask,
                     const KWin::Region &deviceRegion, KWin::WindowPaintData &data) override;
    bool drawWindow(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, KWin::EffectWindow *window, int mask,
                    const KWin::Region &deviceRegion, KWin::WindowPaintData &data) override;
    void postPaintScreen() override;

    void geometryAnimationStopped(KWin::EffectWindow *window);
    quint64 crossFade(KWin::EffectWindow *window, std::chrono::milliseconds duration);

    static bool supported();

    using KWin::AnimationEffect::cancel;
    using KWin::AnimationEffect::freezeInTime;
    using KWin::AnimationEffect::retarget;

private:
    KarouselFxConfig m_config;
    std::unique_ptr<FocusRing> m_focusRing;
    std::unique_ptr<GeometryChange> m_geometryChange;
    bool m_capturing = false;
};
