#include "focusring.h"
#include "karouselfxconfig.h"

#include <effect/effect.h>
#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <scene/outlinedborderitem.h>
#include <scene/windowitem.h>
#include <wayland/surface.h>
#include <window.h>

#include <KColorScheme>
#include <KDecoration3/Decoration>
#include <KSharedConfig>

#include <algorithm>
#include <cmath>

using namespace KWin;

static qreal clientCornerRadius(const Window *window)
{
    const SurfaceInterface *surface = window->surface();
    if (!surface) {
        return 0;
    }

    const RectF frame(window->frameGeometry().topLeft() - window->bufferGeometry().topLeft(), window->frameGeometry().size());
    const RegionF opaque = surface->opaque();
    const RectF rect = opaque.boundingRect();
    if (rect.isEmpty() || !opaque.contains(rect)) {
        return 0;
    }

    const qreal left = rect.left() - frame.left();
    const qreal right = frame.right() - rect.right();
    const qreal top = rect.top() - frame.top();
    const qreal bottom = frame.bottom() - rect.bottom();
    qreal radius = 0;
    if (std::abs(top) < 1 && std::abs(bottom) < 1 && std::abs(left - right) < 2) {
        radius = std::round((left + right) / 2);
    } else if (std::abs(left) < 1 && std::abs(right) < 1 && std::abs(top - bottom) < 2) {
        radius = std::round((top + bottom) / 2);
    }
    return radius > 0 && 2 * radius < std::min(frame.width(), frame.height()) ? radius : 0;
}

static BorderRadius ringRadius(const Window *window)
{
    if (const KDecoration3::Decoration *decoration = window->decoration()) {
        const KDecoration3::BorderOutline outline = decoration->borderOutline();
        if (!outline.isNull()) {
            return BorderRadius::from(outline.radius());
        }
    }
    if (!window->borderRadius().isNull()) {
        return window->borderRadius();
    }
    return BorderRadius(clientCornerRadius(window));
}

static bool wantsRing(const Window *window)
{
    return window->isClient()
        && (window->inherits("KWin::XdgToplevelWindow") || window->inherits("KWin::X11Window"))
        && (window->isNormalWindow() || window->isDialog())
        && !window->isFullScreen()
        && window->maximizeMode() != MaximizeFull
        && !effects->hasActiveFullScreenEffect()
        && !effects->isScreenLocked();
}

FocusRing::FocusRing(const KarouselFxConfig &config)
    : m_config(config)
    , m_kdeglobals(KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("kdeglobals"))))
{
    connect(m_kdeglobals.data(), &KConfigWatcher::configChanged, this, &FocusRing::updateColor);
    connect(effects, &EffectsHandler::windowActivated, this, [this](EffectWindow *window) {
        setWindow(window ? window->window() : nullptr);
    });
    connect(effects, &EffectsHandler::windowClosed, this, [this](EffectWindow *window) {
        if (window->window() == m_window) {
            setWindow(nullptr);
        }
    });
    connect(effects, &EffectsHandler::hasActiveFullScreenEffectChanged, this, &FocusRing::updateRing);
    connect(effects, &EffectsHandler::screenLockingChanged, this, &FocusRing::updateRing);

    updateColor();

    EffectWindow *active = effects->activeWindow();
    setWindow(active ? active->window() : nullptr);
}

FocusRing::~FocusRing()
{
    removeRing();
}

void FocusRing::reconfigure()
{
    updateColor();
}

void FocusRing::applyPaintTransform(EffectWindow *window, const WindowPaintData &data)
{
    const qreal xScale = data.xScale();
    const qreal yScale = data.yScale();
    if (!m_ring || window->window() != m_window || xScale <= 0 || yScale <= 0) {
        return;
    }

    const qreal thickness = m_config.focusRingThickness();
    const auto size = m_window->frameGeometry().size();
    m_ring->setInnerRect(RectF(0, 0, size.width() * xScale, size.height() * yScale));
    m_ring->setTransform(QTransform(1 / xScale, 0, 0, 1 / yScale, thickness * (1 - 1 / xScale), thickness * (1 - 1 / yScale)));
}

void FocusRing::resetPaintTransform(EffectWindow *window)
{
    if (!m_ring || (window && window->window() != m_window) || m_ring->transform().isIdentity()) {
        return;
    }

    m_ring->setTransform(QTransform());
    updateRing();
}

void FocusRing::setWindow(Window *window)
{
    if (m_window == window) {
        return;
    }

    removeRing();
    if (m_window) {
        disconnect(m_window, nullptr, this, nullptr);
    }
    disconnect(m_surfaceConnection);

    m_window = window;
    if (m_window) {
        connect(m_window, &Window::frameGeometryChanged, this, &FocusRing::updateRing);
        connect(m_window, &Window::bufferGeometryChanged, this, &FocusRing::updateRing);
        connect(m_window, &Window::borderRadiusChanged, this, &FocusRing::updateRing);
        connect(m_window, &Window::fullScreenChanged, this, &FocusRing::updateRing);
        connect(m_window, &Window::maximizedChanged, this, &FocusRing::updateRing);
        connect(m_window, &Window::decorationChanged, this, &FocusRing::updateDecoration);
        if (SurfaceInterface *surface = m_window->surface()) {
            m_surfaceConnection = connect(surface, &SurfaceInterface::opaqueChanged, this, &FocusRing::updateRing);
        }
    }
    updateDecoration();
}

void FocusRing::removeRing()
{
    if (m_ring) {
        m_ring->setVisible(false);
        m_ring.reset();
    }
}

void FocusRing::updateColor()
{
    if (m_config.focusRingUseAccentColor()) {
        m_color = KColorScheme(QPalette::Active, KColorScheme::Window, m_kdeglobals->config()).decoration(KColorScheme::FocusColor).color();
    } else {
        m_color = m_config.focusRingColor();
    }
    updateRing();
}

void FocusRing::updateDecoration()
{
    disconnect(m_decorationConnection);
    if (m_window && m_window->decoration()) {
        m_decorationConnection = connect(m_window->decoration(), &KDecoration3::Decoration::borderOutlineChanged, this, &FocusRing::updateRing);
    }
    updateRing();
}

void FocusRing::updateRing()
{
    if (!m_window || !wantsRing(m_window)) {
        removeRing();
        return;
    }

    const RectF innerRect(QPointF(0, 0), m_window->frameGeometry().size());
    const BorderOutline outline(m_config.focusRingThickness(), m_color, ringRadius(m_window));
    if (m_ring) {
        m_ring->setInnerRect(innerRect);
        m_ring->setOutline(outline);
    } else {
        m_ring = std::make_unique<OutlinedBorderItem>(innerRect, outline, m_window->windowItem());
    }
}
