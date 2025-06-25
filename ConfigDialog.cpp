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
    ifaceBtn.setText("Select Interface");
    ifaceBtn.setPopupMode(QToolButton::InstantPopup);
    ifaceBtn.setMenu(&ifaceMenu);

    selectedIface = QNetworkInterface::interfaceFromName(confMgr.ifaceName());
    if(selectedIface.isValid())
        ifaceBtn.setText(selectedIface.humanReadableName());

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

    modeCb.addItems({"Broadcast", "Multicast", "Unicast"});
    modeCb.setCurrentIndex(int(confMgr.mode()));

    QObject::connect(&modeCb, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, &getIfaceAddr](int idx)
    {
        dstAddrEdit.setText(getIfaceAddr(selectedIface, ConfigManager::Mode(idx)).toString());
    });

    dstAddrEdit.setText(confMgr.addr().toString());

    portSb.setRange(0, 65535);
    portSb.setValue(confMgr.port());

    QObject::connect(&ifaceMenu, &QMenu::aboutToShow, this, [this]()
    {
        ifaceMenu.clear();
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

            QAction* action = ifaceMenu.addAction(iface.humanReadableName());
            action->setCheckable(true);
            action->setChecked(iface.name() == selectedIface.name());
            action->setData(QVariant::fromValue<QNetworkInterface>(iface));
        }
    });

    QObject::connect(&ifaceMenu, &QMenu::triggered, this, [this, &getIfaceAddr](QAction *action)
    {
        selectedIface = action->data().value<QNetworkInterface>();
        dstAddrEdit.setText(getIfaceAddr(selectedIface, ConfigManager::Mode(modeCb.currentIndex())).toString());
        ifaceBtn.setText(action->text());
    });

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, this, [this, &confMgr]
    {        
        auto addr = QHostAddress(dstAddrEdit.text());
        auto mode = ConfigManager::Mode(modeCb.currentIndex());
        if(addr.isNull() ||
           mode == ConfigManager::Mode::Broadcast && addr.protocol() != QAbstractSocket::IPv4Protocol ||
           mode == ConfigManager::Mode::Multicast && !addr.isMulticast())
        {
            dstAddrEdit.setFocus();
            return;
        }

        if(passEdit.text().isEmpty())
        {
            passEdit.setFocus();
            return;
        }

        confMgr.beginSeqChange();
          confMgr.setIfaceName(selectedIface.name());
          confMgr.setMode(mode);
          confMgr.setAddr(QHostAddress(dstAddrEdit.text()));
          confMgr.setPort(portSb.value());
          confMgr.setPass(passEdit.text(), portSb.value()); // The port number serves as the salt for the pass hash
        confMgr.endSeqChange();

        accept();
    });
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);

    passEdit.setEchoMode(QLineEdit::Password);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("Network Interface", &ifaceBtn);
    formLayout->addRow("Operating Mode", &modeCb);
    formLayout->addRow("Destination Address", &dstAddrEdit);
    formLayout->addRow("Port", &portSb);
    formLayout->addRow("Password", &passEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(&buttonBox);
}

