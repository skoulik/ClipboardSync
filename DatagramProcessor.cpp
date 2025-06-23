#include "DatagramProcessor.h"
#include "CryptoEngine.h"
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
            QByteArray data;
            data.resize(static_cast<int>(socketRx.pendingDatagramSize()));
            QHostAddress sender;
            quint16 senderPort;
            socketRx.readDatagram(data.data(), data.size(), &sender, &senderPort);
            if(CryptoEngine::decrypt(data, m_confMgr.passHash()))
                emit datagramReceived(data, sender, senderPort);
        }
    });
}

bool DatagramProcessor::sendDatagram(const QByteArray& data)
{
    QByteArray cypher = data;
    CryptoEngine::encrypt(cypher, m_confMgr.passHash());
    qint64 sent = socketTx.writeDatagram(cypher, m_confMgr.bcastAddr(), m_confMgr.port());
    return sent == cypher.size();
}

