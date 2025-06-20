#include "ClipboardManager.h"
#include <QMimeData>


ClipboardManager::ClipboardManager(QObject* parent) : QObject(parent)
{
    const QMimeData* mimeData = clipboard->mimeData();
    lastDataHash = mimeData->hasFormat("text/plain") ? qHash(mimeData->data("text/plain")) : 0;

  #ifdef Q_OS_MAC
    QObject::connect(&pollTimer, &QTimer::timeout, this,
  #else
    QObject::connect(clipboard, &QClipboard::dataChanged, this,
  #endif
        [this]
        {
            const QMimeData* mimeData = clipboard->mimeData();
            if(mimeData->hasFormat("text/plain"))
            {
                QByteArray data = mimeData->data("text/plain");
                uint newDataHash = qHash(data);
                if(newDataHash != lastDataHash)
                {
                    lastDataHash = newDataHash;
                    emit dataChanged(data);
                }
            }
        }
    );
  #ifdef Q_OS_MAC
    pollTimer.start(1000);
  #endif
}

void ClipboardManager::setData(const QByteArray& data)
{
    auto mimeData = new QMimeData;
    mimeData->setData("text/plain", data);
    clipboard->setMimeData(mimeData);
}

