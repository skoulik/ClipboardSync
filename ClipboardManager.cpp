#include "ClipboardManager.h"
#include <QMimeData>


ClipboardManager::ClipboardManager(QObject* parent) : QObject(parent)
{
    const QMimeData* mimeData = m_clipboard->mimeData();
    if(mimeData && mimeData->hasFormat("text/plain"))
        m_lastDataHash = qHash(mimeData->data("text/plain"));

  #ifdef Q_OS_MAC
    connect(&m_pollTimer, &QTimer::timeout, this,
  #else
    connect(m_clipboard, &QClipboard::dataChanged, this,
  #endif
        [this]
        {
            if(signalsBlocked()) return;

            const QMimeData* mimeData = m_clipboard->mimeData();
            if(mimeData && mimeData->hasFormat("text/plain"))
            {
                QByteArray data = mimeData->data("text/plain");
                uint newDataHash = qHash(data);
                if(newDataHash != m_lastDataHash)
                {
                    m_lastDataHash = newDataHash;
                    emit dataChanged(data);
                }
            }
        }
    );
  #ifdef Q_OS_MAC
    m_pollTimer.start(1000);
  #endif
}

void ClipboardManager::setData(const QByteArray& data, bool emitDataChanged)
{
    uint newDataHash = qHash(data);
    if(newDataHash == m_lastDataHash)
        return;
    if(!emitDataChanged)
        m_lastDataHash = newDataHash;

    auto mimeData = new QMimeData;
    mimeData->setData("text/plain", data);
    m_clipboard->setMimeData(mimeData);
}

