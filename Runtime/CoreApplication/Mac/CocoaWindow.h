#pragma once
#include <AppKit/AppKit.h>

class FMacApplication;

@interface FCocoaWindow : NSWindow<NSWindowDelegate>
{
    CGFloat CachedWidth;
    CGFloat CachedHeight;
}

@end
