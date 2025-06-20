#include "DatagramProcessor.h"
#include <QNetworkInterface>
#include <QMessageBox>
#include <QCoreApplication>


DatagramProcessor::DatagramProcessor(const ConfigManager& confMgr, QObject *parent)
    : QObject(parent), m_confMgr(confMgr)
{
    QObject::connect(&confMgr, &ConfigManager::configChanged, this, [this]
    {
        socketRx.close();
        if(!socketRx.bind(QHostAddress::Any, m_confMgr.port(), QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
            QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Failed to bind UDP Rx socket.");   
    });

    QObject::connect(&socketRx, &QUdpSocket::readyRead, this, [this]
    {
        while(socketRx.hasPendingDatagrams())
        {
            QByteArray datagram;
            datagram.resize(static_cast<int>(socketRx.pendingDatagramSize()));
            QHostAddress sender;
            quint16 senderPort;
            socketRx.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            emit datagramReceived(datagram, sender, senderPort);
        }
    });
}

bool DatagramProcessor::sendDatagram(const QByteArray& data)
{
    qint64 sent = socketTx.writeDatagram(data, m_confMgr.bcastAddr(), m_confMgr.port());
    return sent == data.size();
}

