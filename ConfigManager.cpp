#include "ConfigManager.h"
#include <QSettings>


ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    m_ifaceName = settings.value("ifaceName").toString();
    m_bcastAddr = QHostAddress(settings.value("bcastAddr", m_bcastAddr.toString()).toString());
    m_port      = settings.value("port", m_port).toUInt();
}

ConfigManager::~ConfigManager()
{
    QSettings settings;
    settings.setValue("ifaceName", m_ifaceName);
    settings.setValue("bcastAddr", m_bcastAddr.toString());
    settings.setValue("port",      m_port);
}

void ConfigManager::beginSecChange()
{
    changed = false;
    seqChanging = true;
}

void ConfigManager::endSecChange()
{
    seqChanging = false;
    if(changed)
        emit configChanged();
}

template <typename T> bool ConfigManager::setter(T& dst, const T& src)
{
    if(dst == src)
        return false;
    dst = src;
    changed = true;
    if(!seqChanging)
        emit configChanged();
    return true;
}

bool ConfigManager::setIfaceName(const QString& name)
{
    return setter(m_ifaceName, name);
}

bool ConfigManager::setBcastAddr(const QHostAddress& addr)
{
    return setter(m_bcastAddr, addr);
}

bool ConfigManager::setPort(quint16 port)
{
    return setter(m_port, port);
}

