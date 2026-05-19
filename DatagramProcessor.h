#ifndef DATAGRAMPROCESSOR_H
#define DATAGRAMPROCESSOR_H

#include "ConfigManager.h"
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QElapsedTimer>
#include <QRandomGenerator>


class DatagramProcessor : public QObject
{
    Q_OBJECT

  private:
    class PendingTransfer
    {
      private:
        const quint16                 m_totalFragmentCount {0};
        std::map<quint16, QByteArray> m_fragments;
        quint32                       m_totalSize          {0};
        QElapsedTimer                 m_idleTimer;

      public:
        PendingTransfer(quint16 totalFragmentCount);
        bool isComplete() const { return m_fragments.size() == m_totalFragmentCount; }
        bool contains(quint16 fragmentIdx) const { return m_fragments.count(fragmentIdx) > 0; }
        quint16 totalFragmentCount() const { return m_totalFragmentCount; }
        bool insertFragment(quint16 idx, const QByteArray& data);
        QByteArray reassemble() const;
        quint64 idleTime() const { return m_idleTimer.elapsed(); }
    };

    struct Header
    {
        static constexpr quint16 MAGIC = 0xB81E;
        quint16 m_magic          {MAGIC};
        quint16 m_fragmentCount  {0};
        quint16 m_fragmentIndex  {0};
        quint16 m_reserved       {0};
        quint32 m_transferId     {0};

        Header(quint16 fragmentCount, quint16 fragmentIndex, quint32 transferId)
          : m_fragmentCount(fragmentCount), m_fragmentIndex(fragmentIndex), m_transferId(transferId) {}
        Header(const QByteArray& data);
    };
    static_assert(sizeof(Header) == 12);
    friend QDataStream& operator<<(QDataStream&, const DatagramProcessor::Header&);
    friend QDataStream& operator>>(QDataStream&, DatagramProcessor::Header&);

  private:
    const ConfigManager& m_confMgr;

    static constexpr quint64 TRANSFER_TIMEOUT_MS = 10000;
    static constexpr quint32 MAX_DATAGRAM_SIZE = 1500; // TODO: make configurable
                                                                  // IP4 UDP                  pad   iv
    static constexpr quint32 MAX_PAYLOAD_SIZE  = MAX_DATAGRAM_SIZE - 20 - 8 - sizeof(Header) - 16 - 16;

    QUdpSocket m_socketTx, m_socketRx;
    quint32 m_transferIdCounter { QRandomGenerator::global()->generate() };
    using TransferKey = std::pair<QString, quint32>;            // Key: {sender address string, transfer id}
    std::map<TransferKey, PendingTransfer> m_pendingTransfers;  // Val: transfer session
    QElapsedTimer m_expiredTransfersRecheckTimer;

  private slots:
    void purgeExpiredTransfers();

  public:
    DatagramProcessor(const ConfigManager& confMgr, QObject *parent = nullptr);

  public slots:
    bool sendDatagram(const QByteArray& payload);

  signals:
    void datagramReceived(const QByteArray& payload, const QHostAddress& sender, quint16 senderPort);
};

QDataStream& operator<<(QDataStream&, const DatagramProcessor::Header&);
QDataStream& operator>>(QDataStream&, DatagramProcessor::Header&);

#endif // DATAGRAMPROCESSOR_H

