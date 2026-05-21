#include "DatagramProcessor.h"
#include "CryptoEngine.h"
#include <QNetworkInterface>
#include <QMessageBox>
#include <QCoreApplication>
#include <QApplication>


DatagramProcessor::PendingTransfer::PendingTransfer(quint16 totalFragmentCount)
  : m_totalFragmentCount(totalFragmentCount)
{
    m_idleTimer.start();
}

bool DatagramProcessor::PendingTransfer::insertFragment(quint16 idx, const QByteArray& data)
{
    if(idx >= m_totalFragmentCount) return false;
    m_fragments.emplace(idx, data);
    m_totalSize += data.size();
    m_idleTimer.restart();
    return true;
}

QByteArray DatagramProcessor::PendingTransfer::reassemble() const
{
    if(m_fragments.size() < m_totalFragmentCount) return {};
    QByteArray result;
    result.reserve(m_totalSize);
    for(const auto& fragment : m_fragments)
        result.append(fragment.second);
    return result;
}

DatagramProcessor::Header::Header(const QByteArray& data)
{   
    if(data.size() < sizeof(*this))
        return;

    QDataStream ds(data);
    ds.setByteOrder(QDataStream::BigEndian);

    ds >> *this;
}

QDataStream& operator<<(QDataStream& ds, const DatagramProcessor::Header& h)
{
    ds << h.m_magic << h.m_fragmentCount << h.m_fragmentIndex << h.m_reserved << h.m_transferId;
    return ds;
}

QDataStream& operator>>(QDataStream& ds, DatagramProcessor::Header& h)
{
    ds >> h.m_magic >> h.m_fragmentCount >> h.m_fragmentIndex >> h.m_reserved >> h.m_transferId;
    return ds;
}

DatagramProcessor::DatagramProcessor(const ConfigManager& confMgr, QObject *parent)
    : QObject(parent), m_confMgr(confMgr)
{
    m_expiredTransfersRecheckTimer.start();

    connect(&confMgr, &ConfigManager::configChanged, this, [this]
    {
        m_socketRx.close();
        switch(m_confMgr.mode())
        {
            case ConfigManager::Mode::Broadcast:
            case ConfigManager::Mode::Unicast:
            {
                if(!m_socketRx.bind(QHostAddress::Any, m_confMgr.port(), QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
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
                if(!m_socketRx.bind(bindAddr, m_confMgr.port(), QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Failed to bind UDP Rx socket.");

                if(!m_socketRx.joinMulticastGroup(group, iface))
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Failed to join multicast group.");

                m_socketRx.setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);

                m_socketTx.close();
                if(!m_socketTx.bind(bindAddr, 0))
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(), "Failed to bind UDP Tx socket.");
                m_socketTx.setMulticastInterface(iface);
                m_socketTx.setSocketOption(QAbstractSocket::MulticastTtlOption, 3); // TODO: make configurable
            }
            break;
        }
    });

    connect(&m_socketRx, &QUdpSocket::readyRead, this, [this]
    {
        while(m_socketRx.hasPendingDatagrams())
        {
            if(signalsBlocked())
            {
                m_socketRx.readDatagram(nullptr, 0); // discard
                continue;
            }

            QByteArray data;
            data.resize(static_cast<int>(m_socketRx.pendingDatagramSize()));
            QHostAddress sender;
            quint16 senderPort;
            m_socketRx.readDatagram(data.data(), data.size(), &sender, &senderPort);

            const Header hdr(data);
            if(hdr.m_magic != Header::MAGIC || hdr.m_fragmentCount < 1)
                continue;

            QByteArray& payload = data.remove(0, sizeof(Header));

            if(!CryptoEngine::decrypt(payload, CryptoEngine::bindContext(m_confMgr.passHash(), QByteArray::fromRawData(reinterpret_cast<const char*>(&hdr), sizeof(hdr)))))
                continue;

            if(hdr.m_fragmentCount == 1)
            {
                 emit datagramReceived(payload, sender, senderPort);
                 continue;
            }

            TransferKey key = { sender.toString(), hdr.m_transferId };
            if(m_pendingTransfers.count(key) == 0)
                m_pendingTransfers.emplace(key, hdr.m_fragmentCount);
            
            PendingTransfer& transfer = m_pendingTransfers.at(key);
            
            if(hdr.m_fragmentIndex >= transfer.totalFragmentCount() || transfer.contains(hdr.m_fragmentIndex))
                continue;
            
            transfer.insertFragment(hdr.m_fragmentIndex, payload);
            if(transfer.isComplete())
            {
                emit datagramReceived(transfer.reassemble(), sender, senderPort);
                m_pendingTransfers.erase(key);
            }
        }
        purgeExpiredTransfers();
    });
}

bool DatagramProcessor::sendDatagram(const QByteArray& payload)
{
    const int fragmentCount = (payload.size() + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
    if(fragmentCount > std::numeric_limits<quint16>::max())
        return false;

    const quint32 transferId = m_transferIdCounter++;
    for(int i = 0; i < fragmentCount; ++i)
    {
        const int offset  = i * MAX_PAYLOAD_SIZE;
        const int length  = qMin(MAX_PAYLOAD_SIZE, static_cast<quint32>(payload.size() - offset));

        Header hdr(fragmentCount, i, transferId);

        QByteArray fragmentPayload = payload.mid(offset, length);
        CryptoEngine::encrypt(fragmentPayload, CryptoEngine::bindContext(m_confMgr.passHash(), QByteArray::fromRawData(reinterpret_cast<const char*>(&hdr), sizeof(hdr))));

        QByteArray fragment(sizeof(Header) + fragmentPayload.size(), Qt::Uninitialized);
        QDataStream ds(&fragment, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);

        ds << hdr;
        fragment.replace(sizeof(Header), fragmentPayload.size(), fragmentPayload);

        const qint64 sent = m_socketTx.writeDatagram(fragment, m_confMgr.addr(), m_confMgr.port());
        if(sent != fragment.size())
            return false;

        QApplication::processEvents(); // Do not block receiving side
    }
    return true;
}

void DatagramProcessor::purgeExpiredTransfers()
{
    if(m_expiredTransfersRecheckTimer.elapsed() < TRANSFER_TIMEOUT_MS)
        return;

    for(auto it = m_pendingTransfers.begin(); it != m_pendingTransfers.end(); )
    {
        if(it->second.idleTime() > TRANSFER_TIMEOUT_MS)
            it = m_pendingTransfers.erase(it);
        else
            ++it;
    }
    m_expiredTransfersRecheckTimer.restart();
}

