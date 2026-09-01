#pragma once
#include <AppKit/AppKit.h>

@interface FCocoaWindow : NSWindow<NSWindowDelegate>
{
}

/** @brief YES for a borderless popup, which stays out of the key window chain the way a menu does. */
@property (nonatomic, assign) BOOL IsTransientPopup;

@end

@interface FCocoaWindowView : NSView
{
    NSTrackingArea* TrackingArea;
}
@end
