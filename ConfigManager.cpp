#include "ConfigManager.h"
#include "CryptoEngine.h"
#include <QSettings>


ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    m_ifaceName = settings.value("ifaceName").toString();
    m_mode      = Mode(settings.value("mode").toUInt());
    m_addr      = QHostAddress(settings.value("addr", m_addr.toString()).toString());
    m_port      = settings.value("port", m_port).toUInt();
    m_passHash  = settings.value("pass", m_passHash).toByteArray();
}

ConfigManager::~ConfigManager()
{
    QSettings settings;
    settings.setValue("ifaceName", m_ifaceName);
    settings.setValue("mode",      uint(m_mode));
    settings.setValue("addr",      m_addr.toString());
    settings.setValue("port",      m_port);
    settings.setValue("pass",      m_passHash);
}

void ConfigManager::beginSeqChange()
{
    changed = false;
    seqChanging = true;
}

void ConfigManager::endSeqChange()
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

bool ConfigManager::setMode(Mode mode)
{
    return setter(m_mode, mode);
}

bool ConfigManager::setAddr(const QHostAddress& addr)
{
    return setter(m_addr, addr);
}

bool ConfigManager::setPort(quint16 port)
{
    return setter(m_port, port);
}

bool ConfigManager::setPass(const QString& pass, quint16 slt)
{
    auto salt = CryptoEngine::deriveKey(QByteArray((const char *)&slt, sizeof(slt)), {});
    auto hash = CryptoEngine::deriveKey(pass, salt);
    return setter(m_passHash, hash);
}

