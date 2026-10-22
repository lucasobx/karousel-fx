#pragma once

#include <core/rect.h>

#include <QHash>
#include <QObject>
#include <QSet>
#include <QSizeF>
#include <QStringList>
#include <QTimer>

#include <array>
#include <chrono>
#include <optional>

class KarouselFxConfig;
class KarouselFxEffect;

namespace KWin
{
class EffectWindow;
class RenderView;
class WindowPaintData;
}

class GeometryChange : public QObject
{
    Q_OBJECT

public:
    GeometryChange(KarouselFxEffect &effect, const KarouselFxConfig &config);
    ~GeometryChange() override;

    void reconfigure();
    bool isAnimating() const;
    bool isAnimating(KWin::EffectWindow *window) const;
    void advance(const KWin::RenderView *view);
    void applyTransform(KWin::EffectWindow *window, KWin::WindowPaintData &data) const;
    void scheduleRepaints() const;

private:
    static constexpr std::chrono::milliseconds openingDuration{250};
    static constexpr std::chrono::milliseconds transactionTimeout{300};
    static constexpr std::chrono::nanoseconds maximumFrameInterval{16'666'667};
    static constexpr qreal resizeThreshold = 10;
    static constexpr qreal moveThreshold = 1;
    static constexpr qreal angularFrequencyScale = 11.7;
    static constexpr qreal settleDistance = 0.5;
    static constexpr qreal settleVelocity = 50;
    static constexpr qreal repaintMargin = 32;

    struct WindowData
    {
        std::chrono::steady_clock::time_point createdTime;
        bool maximizeChangePending = false;
        bool fullScreen = false;
        std::optional<QSizeF> refusedSize{};
    };

    struct Motion
    {
        std::array<qreal, 4> rect;
        std::array<qreal, 4> velocity{};
        KWin::RectF painted;
        std::optional<std::array<qreal, 4>> heldTarget{};
    };

    void manage(KWin::EffectWindow *window);
    bool isWindowClassExcluded(const QString &windowClass) const;
    bool isAnimatable(const KWin::EffectWindow *window) const;
    void onWindowFrameGeometryChanged(KWin::EffectWindow *window, const KWin::RectF &oldGeometry);
    void onGeometryRequested(KWin::EffectWindow *window);
    void onCommitted(KWin::EffectWindow *window);
    void flushRequests();
    void release();
    void forget(KWin::EffectWindow *window);
    void addRepaint(KWin::EffectWindow *window, const Motion &motion) const;
    void finish(KWin::EffectWindow *window, const Motion &motion);
    void stop(KWin::EffectWindow *window);

    KarouselFxEffect &m_effect;
    const KarouselFxConfig &m_config;
    std::chrono::milliseconds m_duration{0};
    qreal m_angularFrequency = 0;
    QStringList m_excludedWindowClasses;
    bool m_crossFade = false;
    std::chrono::milliseconds m_openingDuration{0};
    std::optional<std::chrono::nanoseconds> m_lastPresentation;
    QHash<KWin::EffectWindow *, WindowData> m_windows;
    QHash<KWin::EffectWindow *, Motion> m_motions;
    QSet<KWin::EffectWindow *> m_requested;
    QHash<KWin::EffectWindow *, QSizeF> m_pending;
    QHash<KWin::EffectWindow *, QSizeF> m_late;
    QTimer m_deadline;
};
