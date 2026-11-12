#include "ClipboardManager.h"
#include "ConfigDialog.h"
#include "ConfigManager.h"
#include "DatagramProcessor.h"
#include "NetworkNotifier.h"
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>


int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("skoulik");
    QCoreApplication::setOrganizationDomain("skoulik");
    QCoreApplication::setApplicationName("Clipboard Sync");

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    ConfigManager confMgr;
    DatagramProcessor dgProc(confMgr);
    ClipboardManager clbMgr(confMgr);
    NetworkNotifier netNotifier(confMgr);

    enum { icnIdle = 0, icnTransmitting, icnDisabled };
    QIcon icons[] = {QIcon(":/TrayIcon.png"), QIcon(":/TrayIconTransmitting.png"), QIcon(":/TrayIconDisabled.png")};
    auto idleIcon = [&]{ return icons[confMgr.disabled() ? icnDisabled : icnIdle]; };

    QSystemTrayIcon trayIcon;
    QMenu trayMenu;
    if(QSystemTrayIcon::isSystemTrayAvailable())
    {
        trayIcon.setIcon(idleIcon());
        trayIcon.setToolTip(app.applicationName());

        QObject::connect(trayMenu.addAction("Config..."), &QAction::triggered, &app, [&]
        {
            ConfigDialog(confMgr).exec();
        });

        auto disable = trayMenu.addAction("Disable");
        disable->setCheckable(true);
        QObject::connect(disable, &QAction::toggled, &app, [&](bool disabled)
        {
            confMgr.setDisabled(disabled);
        });

        QObject::connect(&confMgr, &ConfigManager::configChanged, &app, [&, disable]
        {
            disable->setChecked(confMgr.disabled());
            trayIcon.setIcon(idleIcon());
        });

        QObject::connect(trayMenu.addAction("Quit"), &QAction::triggered, &app, &QCoreApplication::quit);

        QObject::connect(&dgProc, &DatagramProcessor::transmitStateChanged, &app, [&](bool transmitting)
        {
            if(transmitting)
                trayIcon.setIcon(icons[icnTransmitting]);
            else
                QTimer::singleShot(100, [&]{ trayIcon.setIcon(idleIcon()); });
        });

        trayIcon.setContextMenu(&trayMenu);
        trayIcon.show();
    }
    else
    {
        QMessageBox::critical(nullptr, app.applicationName(), "System tray not available.");
    }

    QObject::connect(&clbMgr, &ClipboardManager::dataChanged, &dgProc, &DatagramProcessor::sendDatagram);
    QObject::connect(&dgProc, &DatagramProcessor::datagramReceived, &clbMgr, &ClipboardManager::setData);
    QObject::connect(&netNotifier, &NetworkNotifier::networkChanged, &confMgr, &ConfigManager::configChanged);

    emit confMgr.configChanged();
    return app.exec();
}

