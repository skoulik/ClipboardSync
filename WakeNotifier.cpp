#include "WakeNotifier.h"

#ifdef Q_OS_WIN

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <windows.h>

namespace
{
    class ResumeEventFilter : public QAbstractNativeEventFilter
    {
        WakeNotifier& m_notifier;

      public:
        ResumeEventFilter(WakeNotifier& notifier) : m_notifier(notifier) {}

        bool nativeEventFilter(const QByteArray&, void* message, qintptr*) override
        {
            const MSG* msg = static_cast<const MSG*>(message);
            if(msg->message == WM_POWERBROADCAST && msg->wParam == PBT_APMRESUMEAUTOMATIC)
                m_notifier.scheduleWake();
            return false;
        }
    };
}

WakeNotifier::WakeNotifier(QObject* parent) : QObject(parent)
{
    auto filter = new ResumeEventFilter(*this);
    m_impl = filter;
    QCoreApplication::instance()->installNativeEventFilter(filter);
}

WakeNotifier::~WakeNotifier()
{
    auto filter = static_cast<ResumeEventFilter*>(m_impl);
    QCoreApplication::instance()->removeNativeEventFilter(filter);
    delete filter;
}

#else

WakeNotifier::WakeNotifier(QObject* parent) : QObject(parent) {}
WakeNotifier::~WakeNotifier() {}

#endif
