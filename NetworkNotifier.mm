#include "NetworkNotifier.h"
#import <AppKit/AppKit.h>
#include <SystemConfiguration/SystemConfiguration.h>

namespace
{
    struct Impl
    {
        id wakeObserver;
        SCDynamicStoreRef store;
        CFRunLoopSourceRef source;
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
}

NetworkNotifier::~NetworkNotifier()
{
    auto impl = static_cast<Impl*>(m_impl);
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:impl->wakeObserver];
    [impl->wakeObserver release];
    CFRunLoopSourceInvalidate(impl->source);
    CFRelease(impl->source);
    CFRelease(impl->store);
    delete impl;
}
