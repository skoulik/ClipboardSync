#include "ConfigDialog.h"
#include <QAction>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHostAddress>
#include <QNetworkInterface>
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

    selectedIfaceName = confMgr.ifaceName();
    QNetworkInterface selectedIface = QNetworkInterface::interfaceFromName(selectedIfaceName);
    if(selectedIface.isValid())
        ifaceBtn.setText(selectedIface.humanReadableName());

    dstAddrEdit.setText(confMgr.bcastAddr().toString());

    portSb.setRange(0, 65535);
    portSb.setValue(confMgr.port());

    QObject::connect(&ifaceMenu, &QMenu::aboutToShow, this, [this]()
    {
#if 0  
TODO
          for(const auto & iface : QNetworkInterface::allInterfaces())
          {
              auto type = iface.type();
        
              if(/*type != QNetworkInterface::Unknown && type != QNetworkInterface::Ethernet &&*/
                 type != QNetworkInterface::Wifi)
                 continue;
        
              auto flags = iface.flags();
              if(flags.testFlag(QNetworkInterface::IsLoopBack) || flags.testFlag(QNetworkInterface::IsPointToPoint))
                  continue;
              if(!flags.testFlag(QNetworkInterface::IsUp) || !flags.testFlag(QNetworkInterface::IsRunning) ||
                 !flags.testFlag(QNetworkInterface::CanMulticast))
                  continue;
          
              bool isIp4 = false;
              for(const auto &curAddr : iface.addressEntries())
              {
                  QHostAddress    hostAddr(curAddr.ip());
                  bool            linkLocalB(hostAddr.isLinkLocal());
                  bool            nullB(hostAddr.isNull());
              
                  if(!nullB && !linkLocalB && hostAddr.protocol() == QAbstractSocket::IPv4Protocol)
                  {
                      isIp4 = true;
                      break;
                  }
              }
              if(!isIp4)
                  continue;
        
              qWarning() << iface.name() << " " << iface.humanReadableName() << " " << iface.hardwareAddress();
        
              socketTx.setMulticastInterface(iface);
          }
#endif

        ifaceMenu.clear();
        for(const QNetworkInterface &iface : QNetworkInterface::allInterfaces())
        {
            QHostAddress bcast;
            for(const QNetworkAddressEntry &entry : iface.addressEntries())
            {
                if(entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.broadcast().isNull())
                {
                    bcast = entry.broadcast();
                    break;
                }
            }
            QAction* action = ifaceMenu.addAction(iface.humanReadableName());
            action->setCheckable(true);
            action->setChecked(iface.name() == selectedIfaceName);
            action->setData(QList<QVariant>{ QVariant::fromValue<QHostAddress>(bcast), iface.name() });
        }
    });

    QObject::connect(&ifaceMenu, &QMenu::triggered, this, [this](QAction *action)
    {
        auto data = action->data().value<QList<QVariant>>();
        QHostAddress bcast = data[0].value<QHostAddress>();
        selectedIfaceName = data[1].toString();
        dstAddrEdit.setText(bcast.isNull() ? "255.255.255.255" : bcast.toString());
        ifaceBtn.setText(action->text());
    });

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, this, [this, &confMgr]
    {        
        if(dstAddrEdit.text().isEmpty())
        {
            dstAddrEdit.setFocus();
            return;
        }

        if(passEdit.text().isEmpty())
        {
            passEdit.setFocus();
            return;
        }

        confMgr.beginSecChange();
          confMgr.setIfaceName(selectedIfaceName);
          confMgr.setBcastAddr(QHostAddress(dstAddrEdit.text()));
          confMgr.setPort(portSb.value());
          confMgr.setPass(passEdit.text(), portSb.value()); // The port number serves as the salt for the pass hash
        confMgr.endSecChange();

        accept();
    });
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);

    passEdit.setEchoMode(QLineEdit::Password);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("Interface", &ifaceBtn);
    formLayout->addRow("Broadcast address", &dstAddrEdit);
    formLayout->addRow("Port", &portSb);
    formLayout->addRow("Password", &passEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(&buttonBox);
}

