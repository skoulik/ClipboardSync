#ifndef DATAGRAMPROCESSOR_H
#define DATAGRAMPROCESSOR_H

#include "ConfigManager.h"
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>


class DatagramProcessor : public QObject
{
    Q_OBJECT

  private:
    QUdpSocket socketTx, socketRx;
    const ConfigManager& m_confMgr;

  public:
    DatagramProcessor(const ConfigManager& confMgr, QObject *parent = nullptr);

  public slots:
    bool sendDatagram(const QByteArray& data);

  signals:
    void datagramReceived(const QByteArray& data, const QHostAddress& sender, quint16 senderPort);
};

#endif // DATAGRAMPROCESSOR_H

