#include "WakeNotifier.h"
#import <AppKit/AppKit.h>


WakeNotifier::WakeNotifier(QObject* parent) : QObject(parent)
{
    id observer = [[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserverForName:NSWorkspaceDidWakeNotification object:nil queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification*) { scheduleWake(); }];
    m_impl = [observer retain];
}

WakeNotifier::~WakeNotifier()
{
    id observer = static_cast<id>(m_impl);
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:observer];
    [observer release];
}
