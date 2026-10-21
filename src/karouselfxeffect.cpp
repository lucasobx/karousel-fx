#include "karouselfxeffect.h"
#include "focusring.h"

#include <effect/effecthandler.h>

using namespace KWin;

KarouselFxEffect::KarouselFxEffect()
    : m_config(effects->config())
{
    reconfigure(ReconfigureAll);
}

KarouselFxEffect::~KarouselFxEffect() = default;

void KarouselFxEffect::reconfigure(ReconfigureFlags)
{
    m_config.read();

    if (!m_config.focusRingEnabled()) {
        m_focusRing.reset();
    } else if (m_focusRing) {
        m_focusRing->reconfigure();
    } else {
        m_focusRing = std::make_unique<FocusRing>(m_config);
    }
}

bool KarouselFxEffect::isActive() const
{
    return false;
}

bool KarouselFxEffect::supported()
{
    return effects->isOpenGLCompositing();
}
