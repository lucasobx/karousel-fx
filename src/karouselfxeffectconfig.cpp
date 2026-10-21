#include "karouselfxconfig.h"
#include "ui_karouselfxeffectconfig.h"

#include <KCModule>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDBusConnection>
#include <QDBusMessage>

class KarouselFxEffectConfig : public KCModule
{
    Q_OBJECT

public:
    KarouselFxEffectConfig(QObject *parent, const KPluginMetaData &data)
        : KCModule(parent, data)
        , m_config(KSharedConfig::openConfig(QStringLiteral("kwinrc")))
    {
        m_ui.setupUi(widget());
        addConfig(&m_config, widget());
    }

    void save() override
    {
        KCModule::save();

        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                              QStringLiteral("/Effects"),
                                                              QStringLiteral("org.kde.kwin.Effects"),
                                                              QStringLiteral("reconfigureEffect"));
        message << QStringLiteral(KAROUSELFX_PLUGIN_ID);
        QDBusConnection::sessionBus().call(message);
    }

private:
    Ui::KarouselFxEffectConfig m_ui;
    KarouselFxConfig m_config;
};

K_PLUGIN_CLASS(KarouselFxEffectConfig)

#include "karouselfxeffectconfig.moc"
