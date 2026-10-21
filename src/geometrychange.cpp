#include "geometrychange.h"
#include "karouselfxconfig.h"
#include "karouselfxeffect.h"

#include <effect/effecthandler.h>
#include <effect/effectwindow.h>

#include <QDateTime>
#include <QEasingCurve>

#include <algorithm>

using namespace KWin;

GeometryChange::GeometryChange(KarouselFxEffect &effect, const KarouselFxConfig &config)
    : m_effect(effect)
    , m_config(config)
{
    connect(effects, &EffectsHandler::windowAdded, this, &GeometryChange::manage);
    connect(effects, &EffectsHandler::windowDeleted, this, [this](EffectWindow *window) {
        m_windows.remove(window);
    });
    const QList<EffectWindow *> windows = effects->stackingOrder();
    for (EffectWindow *window : windows) {
        manage(window);
    }

    reconfigure();
}

GeometryChange::~GeometryChange()
{
    for (auto it = m_windows.cbegin(); it != m_windows.cend(); ++it) {
        if (it->animationInstances > 0) {
            it.key()->setData(WindowForceBlurRole, QVariant());
        }
    }
}

void GeometryChange::reconfigure()
{
    m_duration = Effect::animationTime(std::chrono::milliseconds(m_config.geometryChangeDuration()));
    m_excludedWindowClasses = m_config.geometryChangeExcludedWindowClasses().split(QLatin1Char(','));
    m_crossFade = m_config.geometryChangeCrossFade();
}

void GeometryChange::manage(EffectWindow *window)
{
    m_windows.insert(window, WindowData{.createdTime = QDateTime::currentMSecsSinceEpoch()});
    connect(window, &EffectWindow::windowFrameGeometryChanged, this, &GeometryChange::onWindowFrameGeometryChanged);
    connect(window, &EffectWindow::windowMaximizedStateAboutToChange, this, [this, window] {
        const auto data = m_windows.find(window);
        if (data != m_windows.end()) {
            data->maximizedStateAboutToChange = true;
        }
    });
    connect(window, &EffectWindow::windowStartUserMovedResized, this, [this] {
        m_userResizing = true;
    });
    connect(window, &EffectWindow::windowFinishUserMovedResized, this, [this] {
        m_userResizing = false;
    });
}

void GeometryChange::restoreForceBlurState(EffectWindow *window)
{
    const auto data = m_windows.find(window);
    if (data == m_windows.end() || data->animationInstances == 0) {
        return;
    }
    if (--data->animationInstances == 0) {
        window->setData(WindowForceBlurRole, QVariant());
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
    const bool isUserMoveResize = window->isUserMove() || window->isUserResize() || m_userResizing;
    const bool maximizationChange = data->maximizedStateAboutToChange;
    data->maximizedStateAboutToChange = false;
    if (!window->isManaged()
        || !window->isVisible()
        || !window->isOnCurrentDesktop()
        || window->isMinimized()
        || !windowTypeSupportsAnimation
        || (isUserMoveResize && !maximizationChange)
        || isWindowClassExcluded(window->windowClass())) {
        return;
    }

    if (maximizationChange && effects->activeEffects().contains(QStringLiteral("maximize"))) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 windowAgeMs = now - data->createdTime;
    if (windowAgeMs < 0) {
        data->createdTime = now;
    } else if (windowAgeMs < 10) {
        return;
    }

    const RectF newGeometry = window->frameGeometry();
    const qreal xDelta = newGeometry.x() - oldGeometry.x();
    const qreal yDelta = newGeometry.y() - oldGeometry.y();
    const qreal widthDelta = newGeometry.width() - oldGeometry.width();
    const qreal heightDelta = newGeometry.height() - oldGeometry.height();
    const qreal widthRatio = oldGeometry.width() / newGeometry.width();
    const qreal heightRatio = oldGeometry.height() / newGeometry.height();

    data->animationInstances += m_crossFade ? 3 : 2;
    window->setData(WindowForceBlurRole, true);

    const QEasingCurve curve(QEasingCurve::OutExpo);
    m_effect.animate(window, AnimationEffect::Translation, 0, m_duration, FPx2(0, 0), curve, 0,
                     FPx2(-xDelta - widthDelta / 2, -yDelta - heightDelta / 2));
    m_effect.animate(window, AnimationEffect::Scale, 0, m_duration, FPx2(1, 1), curve, 0, FPx2(widthRatio, heightRatio));
    if (m_crossFade) {
        m_effect.animate(window, AnimationEffect::CrossFadePrevious, 0, m_duration, FPx2(1), curve, 0, FPx2(0));
    }
}
