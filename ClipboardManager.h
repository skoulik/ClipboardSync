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
    uint lastDataHash     { 0 };
    QClipboard* clipboard { QApplication::clipboard() };
  #ifdef Q_OS_MAC
    QTimer pollTimer;
  #endif

  public:
    ClipboardManager(QObject* parent = nullptr);

  public slots:
    void setData(const QByteArray& data);

  signals:
    void dataChanged(const QByteArray& data);
};

#endif // CLIPBOARDMANAGER_H

