#ifndef NETWORKNOTIFIER_H
#define NETWORKNOTIFIER_H

#include "ConfigManager.h"
#include <QObject>
#include <QTimer>


class NetworkNotifier : public QObject
{
    Q_OBJECT
  private:
    static constexpr int SETTLE_DELAY_MS = 5000;
    const ConfigManager& m_confMgr;
    QTimer m_settleTimer;
    void* m_impl { nullptr };

  public:
    NetworkNotifier(const ConfigManager& confMgr, QObject* parent = nullptr);
    ~NetworkNotifier();

    void schedule() { m_settleTimer.start(SETTLE_DELAY_MS); }
    void addressAdded(const QString& ifaceName) { if(ifaceName == m_confMgr.ifaceName()) schedule(); }

  signals:
    void networkChanged();
};

#endif // NETWORKNOTIFIER_H
