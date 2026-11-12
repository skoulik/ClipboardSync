#include "NetworkNotifier.h"

#ifdef Q_OS_WIN

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <QNetworkInterface>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>

namespace
{
    class ResumeEventFilter : public QAbstractNativeEventFilter
    {
        NetworkNotifier& m_notifier;

      public:
        ResumeEventFilter(NetworkNotifier& notifier) : m_notifier(notifier) {}

        bool nativeEventFilter(const QByteArray&, void* message, qintptr*) override
        {
            const MSG* msg = static_cast<const MSG*>(message);
            if(msg->message == WM_POWERBROADCAST && msg->wParam == PBT_APMRESUMEAUTOMATIC)
                m_notifier.schedule();
            return false;
        }
    };

    struct Impl
    {
        ResumeEventFilter filter;
        HANDLE addressNotification { nullptr };
    };

    void WINAPI onAddressChange(void* context, MIB_UNICASTIPADDRESS_ROW* row, MIB_NOTIFICATION_TYPE type)
    {
        if(!row || type == MibDeleteInstance)
            return;

        MIB_UNICASTIPADDRESS_ROW entry = *row;
        if(GetUnicastIpAddressEntry(&entry) != NO_ERROR || entry.DadState != IpDadStatePreferred)
            return;

        auto notifier = static_cast<NetworkNotifier*>(context);
        const NET_IFINDEX index = entry.InterfaceIndex;
        QMetaObject::invokeMethod(notifier, [notifier, index]
        {
            notifier->addressAdded(QNetworkInterface::interfaceNameFromIndex(index));
        }, Qt::QueuedConnection);
    }
}

NetworkNotifier::NetworkNotifier(const ConfigManager& confMgr, QObject* parent) : QObject(parent), m_confMgr(confMgr)
{
    m_settleTimer.setSingleShot(true);
    connect(&m_settleTimer, &QTimer::timeout, this, &NetworkNotifier::networkChanged);

    auto impl = new Impl { ResumeEventFilter(*this) };
    m_impl = impl;
    QCoreApplication::instance()->installNativeEventFilter(&impl->filter);
    NotifyUnicastIpAddressChange(AF_UNSPEC, onAddressChange, this, FALSE, &impl->addressNotification);
}

NetworkNotifier::~NetworkNotifier()
{
    auto impl = static_cast<Impl*>(m_impl);
    CancelMibChangeNotify2(impl->addressNotification);
    QCoreApplication::instance()->removeNativeEventFilter(&impl->filter);
    delete impl;
}

#else

NetworkNotifier::NetworkNotifier(const ConfigManager& confMgr, QObject* parent) : QObject(parent), m_confMgr(confMgr) {}
NetworkNotifier::~NetworkNotifier() {}

#endif
