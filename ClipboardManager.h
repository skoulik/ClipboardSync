#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H

#include <QObject>
#include <QApplication>
#include <QClipboard>
#ifdef Q_OS_MAC
    #include <QTimer>
#endif


class ClipboardManager : public QObject
{
    Q_OBJECT
  private:   
    uint m_lastDataHash     { 0 };
    QClipboard* m_clipboard { QApplication::clipboard() };
  #ifdef Q_OS_MAC
    QTimer m_pollTimer;
  #endif

  public:
    ClipboardManager(QObject* parent = nullptr);

  public slots:
    void setData(const QByteArray& data, bool emitDataChanged = false);

  signals:
    void dataChanged(const QByteArray& data);
};

#endif // CLIPBOARDMANAGER_H

