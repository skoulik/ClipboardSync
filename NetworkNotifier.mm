#include "NetworkNotifier.h"
#import <AppKit/AppKit.h>
#include <Network/Network.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <QCoreApplication>
#include <QMessageBox>

namespace
{
    struct Impl
    {
        id wakeObserver;
        SCDynamicStoreRef store;
        CFRunLoopSourceRef source;
        nw_connection_t probe;
        bool denied { false };
        QMessageBox noAccessBox { QMessageBox::Warning, QCoreApplication::applicationName(),
            "Clipboard Sync has no access to the local network, so it cannot sync.\n\n"
            "Allow it in the prompt, or in System Settings > Privacy & Security > Local Network." };
    };

    void onStoreChange(SCDynamicStoreRef store, CFArrayRef changedKeys, void* context)
    {
        for(CFIndex i = 0; i < CFArrayGetCount(changedKeys); ++i)
        {
            auto key = static_cast<CFStringRef>(CFArrayGetValueAtIndex(changedKeys, i));
            CFPropertyListRef value = SCDynamicStoreCopyValue(store, key);
            if(!value)
                continue;
            CFRelease(value);
            static_cast<NetworkNotifier*>(context)->addressAdded(QString::fromCFString(key).section('/', 3, 3));
        }
    }
}

NetworkNotifier::NetworkNotifier(const ConfigManager& confMgr, QObject* parent) : QObject(parent), m_confMgr(confMgr)
{
    m_settleTimer.setSingleShot(true);
    connect(&m_settleTimer, &QTimer::timeout, this, &NetworkNotifier::networkChanged);

    auto impl = new Impl;
    m_impl = impl;
    impl->wakeObserver = [[[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserverForName:NSWorkspaceDidWakeNotification object:nil queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification*) { schedule(); }] retain];

    SCDynamicStoreContext context = { 0, this, nullptr, nullptr, nullptr };
    impl->store = SCDynamicStoreCreate(nullptr, CFSTR("ClipboardSync"), onStoreChange, &context);
    const void* pattern = CFSTR("State:/Network/Interface/[^/]+/IPv[46]");
    CFArrayRef patterns = CFArrayCreate(nullptr, &pattern, 1, &kCFTypeArrayCallBacks);
    SCDynamicStoreSetNotificationKeys(impl->store, nullptr, patterns);
    CFRelease(patterns);
    impl->source = SCDynamicStoreCreateRunLoopSource(nullptr, impl->store, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), impl->source, kCFRunLoopCommonModes);

    nw_endpoint_t endpoint = nw_endpoint_create_host(m_confMgr.addr().toString().toUtf8().constData(),
                                                     QByteArray::number(m_confMgr.port()).constData());
    nw_parameters_t parameters = nw_parameters_create_secure_udp(NW_PARAMETERS_DISABLE_PROTOCOL, NW_PARAMETERS_DEFAULT_CONFIGURATION);
    impl->probe = nw_connection_create(endpoint, parameters);
    nw_release(parameters);
    nw_release(endpoint);
    impl->noAccessBox.setWindowModality(Qt::NonModal);
    nw_connection_set_queue(impl->probe, dispatch_get_main_queue());
    nw_connection_set_state_changed_handler(impl->probe, ^(nw_connection_state_t state, nw_error_t)
    {
        if(state == nw_connection_state_waiting && !impl->denied)
        {
            nw_path_t path = nw_connection_copy_current_path(impl->probe);
            impl->denied = path && nw_path_get_unsatisfied_reason(path) == nw_path_unsatisfied_reason_local_network_denied;
            if(path)
                nw_release(path);
            if(impl->denied)
            {
                impl->noAccessBox.show();
                NSWindow* window = [reinterpret_cast<NSView*>(impl->noAccessBox.winId()) window];
                [window setLevel:NSFloatingWindowLevel];
                [window orderFrontRegardless];
            }
        }
        else if(state == nw_connection_state_ready && impl->denied)
        {
            impl->denied = false;
            impl->noAccessBox.hide();
            schedule();
        }
    });
    nw_connection_start(impl->probe);
}

NetworkNotifier::~NetworkNotifier()
{
    auto impl = static_cast<Impl*>(m_impl);
    nw_connection_cancel(impl->probe);
    nw_release(impl->probe);
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:impl->wakeObserver];
    [impl->wakeObserver release];
    CFRunLoopSourceInvalidate(impl->source);
    CFRelease(impl->source);
    CFRelease(impl->store);
    delete impl;
}
