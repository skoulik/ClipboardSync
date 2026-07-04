#ifndef WAKENOTIFIER_H
#define WAKENOTIFIER_H

#include <QObject>
#include <QTimer>


class WakeNotifier : public QObject
{
    Q_OBJECT
  private:
    static constexpr int WAKE_SETTLE_DELAY_MS = 5000;
    void* m_impl { nullptr };

  public:
    WakeNotifier(QObject* parent = nullptr);
    ~WakeNotifier();

    void scheduleWake() { QTimer::singleShot(WAKE_SETTLE_DELAY_MS, this, &WakeNotifier::systemWoke); }

  signals:
    void systemWoke();
};

#endif // WAKENOTIFIER_H
