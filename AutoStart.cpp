#include "AutoStart.h"
#include <QCoreApplication>

#ifdef Q_OS_WIN

#include <QSettings>
#include <QDir>

namespace
{
    QSettings runKey()
    {
        return QSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                         QSettings::NativeFormat);
    }
}

bool AutoStart::enabled()
{
    return runKey().contains(QCoreApplication::applicationName());
}

void AutoStart::setEnabled(bool on)
{
    QSettings run = runKey();
    if(on)
        run.setValue(QCoreApplication::applicationName(),
                     '"' + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + '"');
    else
        run.remove(QCoreApplication::applicationName());
}

#elif defined(Q_OS_MACOS)

#include <QDir>
#include <QFile>

namespace
{
    QString plistPath()
    {
        return QDir::homePath() + "/Library/LaunchAgents/com.skoulik.clipboardsync.plist";
    }
}

bool AutoStart::enabled()
{
    return QFile::exists(plistPath());
}

void AutoStart::setEnabled(bool on)
{
    if(on)
    {
        QDir().mkpath(QDir::homePath() + "/Library/LaunchAgents");
        QFile file(plistPath());
        if(file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            file.write(QString(
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                "<plist version=\"1.0\">\n"
                "<dict>\n"
                "    <key>Label</key>\n"
                "    <string>com.skoulik.clipboardsync</string>\n"
                "    <key>ProgramArguments</key>\n"
                "    <array>\n"
                "        <string>%1</string>\n"
                "    </array>\n"
                "    <key>RunAtLoad</key>\n"
                "    <true/>\n"
                "</dict>\n"
                "</plist>\n").arg(QCoreApplication::applicationFilePath().toHtmlEscaped()).toUtf8());
        }
    }
    else
        QFile::remove(plistPath());
}

#else

bool AutoStart::enabled() { return false; }
void AutoStart::setEnabled(bool) {}

#endif
