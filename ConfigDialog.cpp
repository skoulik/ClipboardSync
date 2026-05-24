#include "ConfigDialog.h"
#include <QAction>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHostAddress>
#include <QActionGroup>
#include <QMetaType>
#include <QCoreApplication>

Q_DECLARE_METATYPE(QHostAddress);

ConfigDialog::ConfigDialog(ConfigManager& confMgr, QWidget *parent)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
    setWindowTitle(QCoreApplication::applicationName());
    m_ifaceBtn.setText("Select Interface");
    m_ifaceBtn.setPopupMode(QToolButton::InstantPopup);
    m_ifaceBtn.setMenu(&m_ifaceMenu);

    m_selectedIface = QNetworkInterface::interfaceFromName(confMgr.ifaceName());
    if(m_selectedIface.isValid())
        m_ifaceBtn.setText(m_selectedIface.humanReadableName());

    auto getIfaceAddr = [](QNetworkInterface& iface, ConfigManager::Mode mode) -> QHostAddress
    {
        switch(mode)
        {
            case ConfigManager::Mode::Broadcast:
            {
                for(const QNetworkAddressEntry &entry : iface.addressEntries())
                {
                    if(entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.broadcast().isNull())
                        return entry.broadcast();
                }
            }
            break;

            case ConfigManager::Mode::Multicast:
            {
                if(!iface.flags().testFlag(QNetworkInterface::CanMulticast))
                    return {};
                return QHostAddress("239.255.1.1");
            }
            break;

            case ConfigManager::Mode::Unicast:
            {
                QHostAddress v4, v6;
                for(const QNetworkAddressEntry &entry : iface.addressEntries())
                {
                    if(entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                    {
                        if(v4.isNull()) v4 = entry.ip();
                    }
                    else if(entry.ip().protocol() == QAbstractSocket::IPv6Protocol)
                    {
                        if(v6.isNull()) v6 = entry.ip();
                    }
                }
                return !v4.isNull() ? v4 : v6;
            }
            break;
        }

        return QHostAddress("255.255.255.255");
    };

    m_modeCb.addItems({"Broadcast", "Multicast", "Unicast"});
    m_modeCb.setCurrentIndex(int(confMgr.mode()));

    connect(&m_modeCb, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, &getIfaceAddr](int idx)
    {
        m_dstAddrEdit.setText(getIfaceAddr(m_selectedIface, ConfigManager::Mode(idx)).toString());
    });

    m_dstAddrEdit.setText(confMgr.addr().toString());

    m_portSb.setRange(0, 65535);
    m_portSb.setValue(confMgr.port());

    m_mtuSb.setRange(128, 65535);
    m_mtuSb.setValue(confMgr.mtu());

    connect(&m_ifaceMenu, &QMenu::aboutToShow, this, [this]()
    {
        m_ifaceMenu.clear();
        for(const QNetworkInterface &iface : QNetworkInterface::allInterfaces())
        {
            auto type = iface.type();
            if(type != QNetworkInterface::Unknown && type != QNetworkInterface::Ethernet &&
               type != QNetworkInterface::Wifi)
               continue;
      
            auto flags = iface.flags();
            if(flags.testFlag(QNetworkInterface::IsLoopBack))
                continue;
            if(!flags.testFlag(QNetworkInterface::IsUp) || !flags.testFlag(QNetworkInterface::IsRunning))
                continue;
            if(iface.addressEntries().isEmpty())
                continue;
            QString mac = iface.hardwareAddress();
            if(mac.isEmpty() || mac == "00:00:00:00:00:00")
                continue;

            QAction* action = m_ifaceMenu.addAction(iface.humanReadableName());
            action->setCheckable(true);
            action->setChecked(iface.name() == m_selectedIface.name());
            action->setData(QVariant::fromValue<QNetworkInterface>(iface));
        }
    });

    connect(&m_ifaceMenu, &QMenu::triggered, this, [this, &getIfaceAddr](QAction *action)
    {
        m_selectedIface = action->data().value<QNetworkInterface>();
        m_dstAddrEdit.setText(getIfaceAddr(m_selectedIface, ConfigManager::Mode(m_modeCb.currentIndex())).toString());
        m_ifaceBtn.setText(action->text());
    });

    connect(&m_buttonBox, &QDialogButtonBox::accepted, this, [this, &confMgr]
    {        
        auto addr = QHostAddress(m_dstAddrEdit.text());
        auto mode = ConfigManager::Mode(m_modeCb.currentIndex());
        if(addr.isNull() ||
           mode == ConfigManager::Mode::Broadcast && addr.protocol() != QAbstractSocket::IPv4Protocol ||
           mode == ConfigManager::Mode::Multicast && !addr.isMulticast())
        {
            m_dstAddrEdit.setFocus();
            return;
        }

        if(m_passEdit.text().isEmpty())
        {
            m_passEdit.setFocus();
            return;
        }

        confMgr.beginSeqChange();
          confMgr.setIfaceName(m_selectedIface.name());
          confMgr.setMode(mode);
          confMgr.setAddr(QHostAddress(m_dstAddrEdit.text()));
          confMgr.setPort(m_portSb.value());
          confMgr.setMtu(m_mtuSb.value());
          confMgr.setPass(m_passEdit.text(), m_portSb.value()); // The port number serves as the salt for the pass hash
        confMgr.endSeqChange();

        accept();
    });

    m_passEdit.setEchoMode(QLineEdit::Password);

    connect(&m_buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("Network Interface", &m_ifaceBtn);
    formLayout->addRow("Operating Mode", &m_modeCb);
    formLayout->addRow("Destination Address", &m_dstAddrEdit);
    formLayout->addRow("Port", &m_portSb);
    formLayout->addRow("Max datagram size", &m_mtuSb);
    formLayout->addRow("Password", &m_passEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(&m_buttonBox);
}

