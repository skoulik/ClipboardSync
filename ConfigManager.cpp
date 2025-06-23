#include "ConfigManager.h"
#include "CryptoEngine.h"
#include <QSettings>


ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    m_ifaceName = settings.value("ifaceName").toString();
    m_bcastAddr = QHostAddress(settings.value("bcastAddr", m_bcastAddr.toString()).toString());
    m_port      = settings.value("port", m_port).toUInt();
    m_passHash  = settings.value("pass", m_passHash).toByteArray();
}

ConfigManager::~ConfigManager()
{
    QSettings settings;
    settings.setValue("ifaceName", m_ifaceName);
    settings.setValue("bcastAddr", m_bcastAddr.toString());
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

bool ConfigManager::setBcastAddr(const QHostAddress& addr)
{
    return setter(m_bcastAddr, addr);
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

