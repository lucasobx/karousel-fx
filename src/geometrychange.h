#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>

#include <chrono>

class KarouselFxConfig;
class KarouselFxEffect;

namespace KWin
{
class EffectWindow;
class RectF;
}

class GeometryChange : public QObject
{
    Q_OBJECT

public:
    GeometryChange(KarouselFxEffect &effect, const KarouselFxConfig &config);
    ~GeometryChange() override;

    void reconfigure();
    void restoreForceBlurState(KWin::EffectWindow *window);

private:
    static constexpr std::chrono::milliseconds openingDuration{250};
    static constexpr qreal resizeThreshold = 10;
    static constexpr qreal moveThreshold = 1;

    struct WindowData
    {
        qint64 createdTime = 0;
        int animationInstances = 0;
        bool maximizeChangePending = false;
        bool fullScreen = false;
    };

    void manage(KWin::EffectWindow *window);
    bool isWindowClassExcluded(const QString &windowClass) const;
    void onWindowFrameGeometryChanged(KWin::EffectWindow *window, const KWin::RectF &oldGeometry);

    KarouselFxEffect &m_effect;
    const KarouselFxConfig &m_config;
    std::chrono::milliseconds m_duration{0};
    QStringList m_excludedWindowClasses;
    bool m_crossFade = false;
    std::chrono::milliseconds m_openingDuration{0};
    QHash<KWin::EffectWindow *, WindowData> m_windows;
};
