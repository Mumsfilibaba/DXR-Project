#pragma once
#include <AppKit/AppKit.h>

@interface FCocoaWindow : NSWindow<NSWindowDelegate>
{
    CGFloat CachedWidth;
    CGFloat CachedHeight;
}
@end

@interface FCocoaWindowView : NSView<NSTextInputClient>
{
    CGFloat ScaleX;
    CGFloat ScaleY;
    CGFloat FrameBufferWidth;
    CGFloat FrameBufferHeight;
}
@end
