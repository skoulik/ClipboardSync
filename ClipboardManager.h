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
    size_t m_lastDataHash   { 0 };
    QClipboard* m_clipboard { QApplication::clipboard() };
  #ifdef Q_OS_MAC
    QTimer m_pollTimer;
  #endif

    static QByteArray serialize(const QMimeData* mime);
    static std::unique_ptr<QMimeData> deserialize(const QByteArray& serializedData);

  public:
    ClipboardManager(QObject* parent = nullptr);

  public slots:
    void setData(const QByteArray& serializedData);

  signals:
    void dataChanged(const QByteArray& serializedData);
};

#endif // CLIPBOARDMANAGER_H

