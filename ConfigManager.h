#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QHostAddress>

class ConfigManager : public QObject
{
    Q_OBJECT
  public:
    ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();

    const QString& ifaceName() const { return m_ifaceName; }
    const QHostAddress& bcastAddr() const { return m_bcastAddr; }
    quint16 port() const { return m_port; }

    void beginSecChange();
    void endSecChange();
    bool setIfaceName(const QString& name);
    bool setBcastAddr(const QHostAddress& addr);
    bool setPort(quint16 port);

  signals:
    void configChanged();

  private:
    QString m_ifaceName;
    QHostAddress m_bcastAddr { "255.255.255.255" };
    quint16 m_port           { 19211 };
    bool seqChanging         { false };
    bool changed             { false };

    template <typename T> bool setter(T& dst, const T& src);
};

#endif // CONFIGMANAGER_H

