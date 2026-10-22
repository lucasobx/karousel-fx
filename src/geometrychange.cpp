#include "geometrychange.h"
#include "karouselfxconfig.h"
#include "karouselfxeffect.h"

#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <scene/scene.h>
#include <window.h>

#include <QEasingCurve>

#include <algorithm>
#include <cmath>

using namespace KWin;

static bool isGrabbedByAnotherEffect(const EffectWindow *window)
{
    constexpr std::array roles{WindowAddedGrabRole, WindowClosedGrabRole, WindowMinimizedGrabRole, WindowUnminimizedGrabRole};
    return std::ranges::any_of(roles, [window](DataRole role) {
        return window->data(role).isValid();
    });
}

static bool isAnyWindowUserResized()
{
    const QList<EffectWindow *> windows = effects->stackingOrder();
    return std::ranges::any_of(windows, [](const EffectWindow *window) {
        return window->isUserResize();
    });
}

static std::array<qreal, 4> toArray(const RectF &rect)
{
    return {rect.x(), rect.y(), rect.width(), rect.height()};
}

static RectF visualGeometry(const EffectWindow *window, const std::array<qreal, 4> &rect)
{
    const RectF frame = window->frameGeometry();
    const RectF expanded = window->expandedGeometry();
    if (frame.width() <= 0 || frame.height() <= 0) {
        return expanded;
    }

    const qreal xScale = rect[2] / frame.width();
    const qreal yScale = rect[3] / frame.height();
    return RectF(rect[0] + (expanded.x() - frame.x()) * xScale, rect[1] + (expanded.y() - frame.y()) * yScale,
                 expanded.width() * xScale, expanded.height() * yScale);
}

GeometryChange::GeometryChange(KarouselFxEffect &effect, const KarouselFxConfig &config)
    : m_effect(effect)
    , m_config(config)
{
    connect(effects, &EffectsHandler::windowAdded, this, &GeometryChange::manage);
    connect(effects, &EffectsHandler::windowDeleted, this, [this](EffectWindow *window) {
        m_windows.remove(window);
        stop(window);
    });
    const QList<EffectWindow *> windows = effects->stackingOrder();
    for (EffectWindow *window : windows) {
        manage(window);
    }

    reconfigure();
}

GeometryChange::~GeometryChange()
{
    for (auto it = m_motions.cbegin(); it != m_motions.cend(); ++it) {
        addRepaint(it.key(), *it);
        it.key()->setData(WindowForceBlurRole, QVariant());
    }
}

void GeometryChange::reconfigure()
{
    m_duration = Effect::animationTime(std::chrono::milliseconds(m_config.geometryChangeDuration()));
    m_angularFrequency = m_duration.count() > 0 ? angularFrequencyScale / std::chrono::duration<qreal>(m_duration).count() : 0;
    m_excludedWindowClasses = m_config.geometryChangeExcludedWindowClasses().split(QLatin1Char(','));
    m_crossFade = m_config.geometryChangeCrossFade();
    m_openingDuration = Effect::animationTime(GeometryChange::openingDuration);

    if (m_duration.count() <= 0) {
        const QList<EffectWindow *> windows = m_motions.keys();
        for (EffectWindow *window : windows) {
            stop(window);
        }
    }
}

void GeometryChange::manage(EffectWindow *window)
{
    m_windows.insert(window, WindowData{.createdTime = std::chrono::steady_clock::now(), .fullScreen = window->isFullScreen()});
    connect(window, &EffectWindow::windowFrameGeometryChanged, this, &GeometryChange::onWindowFrameGeometryChanged);
    connect(window, &EffectWindow::windowMaximizedStateAboutToChange, this, [this, window](EffectWindow *, bool horizontal, bool vertical) {
        const auto data = m_windows.find(window);
        if (data != m_windows.end()) {
            const auto requested = static_cast<MaximizeMode>((horizontal ? MaximizeHorizontal : MaximizeRestore) | (vertical ? MaximizeVertical : MaximizeRestore));
            data->maximizeChangePending = requested != window->window()->maximizeMode();
        }
    });
    connect(window, &EffectWindow::windowMaximizedStateChanged, this, [this, window] {
        const auto data = m_windows.find(window);
        if (data != m_windows.end()) {
            data->maximizeChangePending = false;
        }
    });
    connect(window, &EffectWindow::windowFullScreenChanged, this, [this, window] {
        const auto data = m_windows.find(window);
        if (data != m_windows.end()) {
            data->fullScreen = window->isFullScreen();
        }
    });
}

bool GeometryChange::isAnimating() const
{
    return !m_motions.isEmpty();
}

bool GeometryChange::isAnimating(EffectWindow *window) const
{
    return m_motions.contains(window);
}

void GeometryChange::advance(const RenderView *view)
{
    if (m_motions.isEmpty() || !view) {
        return;
    }

    const std::chrono::nanoseconds timestamp = view->nextPresentationTimestamp();
    if (m_lastPresentation && timestamp <= *m_lastPresentation) {
        return;
    }
    const qreal dt = m_lastPresentation ? std::chrono::duration<qreal>(std::min(timestamp - *m_lastPresentation, maximumFrameInterval)).count() : 0;
    const qreal decay = std::exp(-m_angularFrequency * dt);
    m_lastPresentation = timestamp;

    for (auto it = m_motions.begin(); it != m_motions.end();) {
        EffectWindow *window = it.key();
        const std::array<qreal, 4> target = toArray(window->frameGeometry());
        bool settled = true;
        for (size_t i = 0; i < target.size(); ++i) {
            const qreal offset = it->rect[i] - target[i];
            const qreal impulse = it->velocity[i] + m_angularFrequency * offset;
            it->rect[i] = target[i] + (offset + impulse * dt) * decay;
            it->velocity[i] = (it->velocity[i] - m_angularFrequency * impulse * dt) * decay;
            settled = settled && std::abs(it->rect[i] - target[i]) < settleDistance && std::abs(it->velocity[i]) < settleVelocity;
        }

        if (settled) {
            const Motion motion = *it;
            it = m_motions.erase(it);
            finish(window, motion);
        } else {
            it->painted = visualGeometry(window, it->rect);
            ++it;
        }
    }
}

void GeometryChange::applyTransform(EffectWindow *window, WindowPaintData &data) const
{
    const auto motion = m_motions.constFind(window);
    const RectF frame = window->frameGeometry();
    if (motion == m_motions.cend() || frame.width() <= 0 || frame.height() <= 0) {
        return;
    }

    data.setXScale(data.xScale() * motion->rect[2] / frame.width());
    data.setYScale(data.yScale() * motion->rect[3] / frame.height());
    data.translate(motion->rect[0] - frame.x(), motion->rect[1] - frame.y());
}

void GeometryChange::scheduleRepaints() const
{
    for (auto it = m_motions.cbegin(); it != m_motions.cend(); ++it) {
        addRepaint(it.key(), *it);
    }
}

bool GeometryChange::isWindowClassExcluded(const QString &windowClass) const
{
    const QStringList parts = windowClass.split(QLatin1Char(' '));
    return std::ranges::any_of(parts, [this](const QString &part) {
        return m_excludedWindowClasses.contains(part);
    });
}

void GeometryChange::onWindowFrameGeometryChanged(EffectWindow *window, const RectF &oldGeometry)
{
    const auto data = m_windows.find(window);
    if (data == m_windows.end()) {
        return;
    }

    const bool windowTypeSupportsAnimation = window->isNormalWindow() || window->isDialog() || window->isModal();
    const bool isUserMoveResize = window->isUserMove() || isAnyWindowUserResized();
    const bool maximizationChange = data->maximizeChangePending;
    data->maximizeChangePending = false;
    const bool fullScreenChange = window->window()->isRequestedFullScreen() != window->isFullScreen() || window->isFullScreen() != data->fullScreen;
    if (m_duration.count() <= 0
        || !window->isManaged()
        || !window->isVisible()
        || !window->isOnCurrentDesktop()
        || window->isMinimized()
        || !windowTypeSupportsAnimation
        || (isUserMoveResize && !maximizationChange)
        || isWindowClassExcluded(window->windowClass())
        || isGrabbedByAnotherEffect(window)) {
        stop(window);
        return;
    }

    if (maximizationChange && effects->activeEffects().contains(QStringLiteral("maximize"))) {
        stop(window);
        return;
    }

    if (fullScreenChange && effects->isEffectLoaded(QStringLiteral("fullscreen"))) {
        stop(window);
        return;
    }

    const RectF newGeometry = window->frameGeometry();
    const std::array<qreal, 4> delta{
        newGeometry.x() - oldGeometry.x(),
        newGeometry.y() - oldGeometry.y(),
        newGeometry.width() - oldGeometry.width(),
        newGeometry.height() - oldGeometry.height(),
    };
    const bool resized = std::max(std::abs(delta[2]), std::abs(delta[3])) > resizeThreshold;
    const bool moved = std::max(std::abs(delta[0]), std::abs(delta[1])) > moveThreshold;
    const bool opening = std::chrono::steady_clock::now() - data->createdTime < m_openingDuration;
    auto motion = m_motions.find(window);
    if (opening || (!resized && !moved)) {
        if (motion != m_motions.end()) {
            for (size_t i = 0; i < delta.size(); ++i) {
                motion->rect[i] += delta[i];
            }
            addRepaint(window, *motion);
        }
        return;
    }

    if (motion == m_motions.end()) {
        if (m_motions.isEmpty()) {
            m_lastPresentation.reset();
        }
        const std::array<qreal, 4> rect = toArray(oldGeometry);
        motion = m_motions.insert(window, Motion{.rect = rect, .painted = visualGeometry(window, rect)});
        window->setData(WindowForceBlurRole, true);
    }
    if (!resized) {
        motion->rect[2] += delta[2];
        motion->rect[3] += delta[3];
    }
    addRepaint(window, *motion);

    if (m_crossFade) {
        m_effect.animate(window, AnimationEffect::CrossFadePrevious, 0, m_duration, FPx2(1), QEasingCurve(QEasingCurve::OutExpo), 0, FPx2(0));
    }
}

void GeometryChange::addRepaint(EffectWindow *window, const Motion &motion) const
{
    const RectF area = motion.painted.united(visualGeometry(window, motion.rect)).united(window->expandedGeometry());
    effects->addRepaint(area.adjusted(-repaintMargin, -repaintMargin, repaintMargin, repaintMargin));
}

void GeometryChange::finish(EffectWindow *window, const Motion &motion)
{
    addRepaint(window, motion);
    window->setData(WindowForceBlurRole, QVariant());
    m_effect.geometryAnimationStopped(window);
}

void GeometryChange::stop(EffectWindow *window)
{
    if (m_motions.contains(window)) {
        finish(window, m_motions.take(window));
    }
}
