#include "ClipboardManager.h"
#include "ConfigDialog.h"
#include "ConfigManager.h"
#include "DatagramProcessor.h"
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMessageBox>


int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("SKSoft");
    QCoreApplication::setOrganizationDomain("SKSoft");
    QCoreApplication::setApplicationName("Clipboard Sync");

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    ConfigManager confMgr;
    DatagramProcessor dgProc(confMgr);
    ClipboardManager clbMgr;

    QSignalBlocker dgProcBlker(&dgProc);
    QSignalBlocker clbMgrBlker(&clbMgr);
    for(auto o : {&dgProcBlker, &clbMgrBlker}) o->unblock();

    QSystemTrayIcon trayIcon;
    QMenu trayMenu;
    if(QSystemTrayIcon::isSystemTrayAvailable())
    {
        trayIcon.setIcon(QIcon(":/TrayIcon.png"));
        trayIcon.setToolTip(app.applicationName());

        QObject::connect(trayMenu.addAction("Config..."), &QAction::triggered, &app, [&confMgr]
        {
            ConfigDialog(confMgr).exec();
        });

        auto disable = trayMenu.addAction("Disable");
        disable->setCheckable(true);
        QObject::connect(disable, &QAction::toggled, &app, [&dgProcBlker, &clbMgrBlker](bool checked)
        {
            for(auto o : {&dgProcBlker, &clbMgrBlker}) if(checked) o->reblock(); else o->unblock();
        });

        QObject::connect(trayMenu.addAction("Quit"), &QAction::triggered, &app, &QCoreApplication::quit);

        trayIcon.setContextMenu(&trayMenu);
        trayIcon.show();
    }
    else
    {
        QMessageBox::critical(nullptr, app.applicationName(), "System tray not available.");
    }

    QObject::connect(&clbMgr, &ClipboardManager::dataChanged, &dgProc, &DatagramProcessor::sendDatagram);
    QObject::connect(&dgProc, &DatagramProcessor::datagramReceived, &clbMgr, [&clbMgr](const QByteArray& data){ clbMgr.setData(data); });

    emit confMgr.configChanged();
    return app.exec();
}

