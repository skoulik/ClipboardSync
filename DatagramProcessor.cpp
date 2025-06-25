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
        switch(m_confMgr.mode())
        {
            case ConfigManager::Mode::Broadcast:
            case ConfigManager::Mode::Unicast:
            {
                if(!socketRx.bind(QHostAddress::Any, m_confMgr.port(), QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Failed to bind UDP Rx socket.");   
            }
            break;

            case ConfigManager::Mode::Multicast:
            {
                QNetworkInterface iface = QNetworkInterface::interfaceFromName(m_confMgr.ifaceName());
                if(!iface.isValid())
                {
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Invalid network interface selected.");
                    break;
                }

                QHostAddress group = m_confMgr.addr();
                QHostAddress bindAddr = group.protocol() == QAbstractSocket::IPv4Protocol ? QHostAddress::AnyIPv4 : QHostAddress::AnyIPv6;
                if(!socketRx.bind(bindAddr, m_confMgr.port(), QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Failed to bind UDP Rx socket.");

                if(!socketRx.joinMulticastGroup(group, iface))
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Failed to join multicast group.");

                socketRx.setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);

                socketTx.close();
                if(!socketTx.bind(bindAddr, 0))
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Failed to bind UDP Tx socket.");
                socketTx.setMulticastInterface(iface);
                socketTx.setSocketOption(QAbstractSocket::MulticastTtlOption, 3);
            }
            break;
        }
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
    qint64 sent = socketTx.writeDatagram(cypher, m_confMgr.addr(), m_confMgr.port());
    return sent == cypher.size();
}

