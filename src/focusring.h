#pragma once

#include <KConfigWatcher>

#include <QColor>
#include <QObject>

#include <memory>

class KarouselFxConfig;

namespace KWin
{
class EffectWindow;
class OutlinedBorderItem;
class Window;
class WindowPaintData;
}

class FocusRing : public QObject
{
    Q_OBJECT

public:
    explicit FocusRing(const KarouselFxConfig &config);
    ~FocusRing() override;

    void reconfigure();
    void applyPaintTransform(KWin::EffectWindow *window, const KWin::WindowPaintData &data);
    void resetPaintTransform(KWin::EffectWindow *window = nullptr);

private:
    void setWindow(KWin::Window *window);
    void removeRing();
    void updateColor();
    void updateDecoration();
    void updateRing();

    const KarouselFxConfig &m_config;
    KConfigWatcher::Ptr m_kdeglobals;
    KWin::Window *m_window = nullptr;
    QMetaObject::Connection m_decorationConnection;
    QMetaObject::Connection m_surfaceConnection;
    std::unique_ptr<KWin::OutlinedBorderItem> m_ring;
    QColor m_color;
};
