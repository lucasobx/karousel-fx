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

    static bool supported();

    using KWin::AnimationEffect::animate;

protected:
    void animationEnded(KWin::EffectWindow *window, Attribute attribute, uint meta) override;

private:
    KarouselFxConfig m_config;
    std::unique_ptr<FocusRing> m_focusRing;
    std::unique_ptr<GeometryChange> m_geometryChange;
};
