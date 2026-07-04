#include "ClipboardManager.h"
#include <QDataStream>
#include <QIODevice>
#include <QMimeData>


QByteArray ClipboardManager::serialize(const QMimeData* mime)
{
    QByteArray result;
    QDataStream ds(&result, QIODevice::WriteOnly);
    ds.setVersion(QDataStream::Qt_6_0);
    for(const QString& format : mime->formats())
    {
        if(!format.startsWith("text/")) continue;
        ds << format << mime->data(format);
    }
    return result;
}

std::unique_ptr<QMimeData> ClipboardManager::deserialize(const QByteArray& serializedData)
{
    QDataStream ds(serializedData);
    ds.setVersion(QDataStream::Qt_6_0);
    auto mimeData = std::make_unique<QMimeData>();
    while(ds.status() == QDataStream::Ok)
    {
        QString format;
        ds >> format;
        if(ds.status() != QDataStream::Ok) break;
        QByteArray data;
        ds >> data;
        if(ds.status() != QDataStream::Ok) return nullptr;
        mimeData->setData(format, data);
    }
    return mimeData;
}

ClipboardManager::ClipboardManager(QObject* parent) : QObject(parent)
{
    const QMimeData* mimeData = m_clipboard->mimeData();
    if(mimeData)
        m_lastDataHash = qHash(serialize(mimeData));

  #ifdef Q_OS_MAC
    connect(&m_pollTimer, &QTimer::timeout, this,
  #else
    connect(m_clipboard, &QClipboard::dataChanged, this,
  #endif
        [this]
        {
            if(signalsBlocked()) return;

            const QMimeData* mimeData = m_clipboard->mimeData();
            if(!mimeData) return;

            QByteArray data = serialize(mimeData);
            size_t newDataHash = qHash(data);
            if(newDataHash != m_lastDataHash)
            {
                m_lastDataHash = newDataHash;
                emit dataChanged(data);
            }
        }
    );
  #ifdef Q_OS_MAC
    m_pollTimer.start(1000);
  #endif
}

void ClipboardManager::setData(const QByteArray& data)
{
    size_t newDataHash = qHash(data);
    if(newDataHash == m_lastDataHash)
        return;
    m_lastDataHash = newDataHash;

    auto mimeData = deserialize(data);
    if(mimeData)
        m_clipboard->setMimeData(mimeData.release());
}

