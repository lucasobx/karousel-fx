#pragma once

#include "karouselfxconfig.h"

#include <effect/effect.h>

#include <memory>

class FocusRing;

class KarouselFxEffect : public KWin::Effect
{
    Q_OBJECT

public:
    KarouselFxEffect();
    ~KarouselFxEffect() override;

    void reconfigure(ReconfigureFlags flags) override;
    bool isActive() const override;

    static bool supported();

private:
    KarouselFxConfig m_config;
    std::unique_ptr<FocusRing> m_focusRing;
};
