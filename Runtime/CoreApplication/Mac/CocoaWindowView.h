#pragma once
#include <Appkit/Appkit.h>

@interface FCocoaWindowView : NSView<NSTextInputClient>
{
    CGFloat ScaleX;
    CGFloat ScaleY;
    CGFloat FrameBufferWidth;
    CGFloat FrameBufferHeight;    
}

@end
