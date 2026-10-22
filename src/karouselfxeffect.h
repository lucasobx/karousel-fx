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
    void paintWindow(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, KWin::EffectWindow *window, int mask,
                     const KWin::Region &deviceRegion, KWin::WindowPaintData &data) override;
    void drawWindow(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, KWin::EffectWindow *window, int mask,
                    const KWin::Region &deviceRegion, KWin::WindowPaintData &data) override;
    void postPaintScreen() override;

    void geometryAnimationStopped(KWin::EffectWindow *window);

    static bool supported();

    using KWin::AnimationEffect::animate;

private:
    KarouselFxConfig m_config;
    std::unique_ptr<FocusRing> m_focusRing;
    std::unique_ptr<GeometryChange> m_geometryChange;
};
