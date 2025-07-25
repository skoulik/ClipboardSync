#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QHostAddress>

class ConfigManager : public QObject
{
    Q_OBJECT
  public:
    enum class Mode
    {
        Broadcast,
        Multicast,
        Unicast
    };

    ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();

    const QString& ifaceName() const { return m_ifaceName; }
    Mode mode() const { return m_mode; }
    const QHostAddress& addr() const { return m_addr; }
    quint16 port() const { return m_port; }
    QByteArray passHash() const { return m_passHash; }

    void beginSeqChange();
    void endSeqChange();
    bool setIfaceName(const QString& name);
    bool setMode(Mode mode);
    bool setAddr(const QHostAddress& addr);
    bool setPort(quint16 port);
    bool setPass(const QString& pass, quint16 salt);

  signals:
    void configChanged();

  private:
    QString m_ifaceName;
    Mode m_mode              { Mode::Broadcast };
    QHostAddress m_addr      { "255.255.255.255" };
    quint16 m_port           { 19211 };
    QByteArray m_passHash;
    bool seqChanging         { false };
    bool changed             { false };

    template <typename T> bool setter(T& dst, const T& src);
};

#endif // CONFIGMANAGER_H

