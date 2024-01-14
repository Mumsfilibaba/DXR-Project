#include "MacCursor.h"
#include "MacWindow.h"
#include "CocoaWindow.h"
#include "Core/Memory/Memory.h"

#include <AppKit/AppKit.h>

/**
 * These cursors are available but not documented
 * See: https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_osx.mm
 */
@interface NSCursor()
+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;
@end


void FMacCursor::SetCursor(ECursor Cursor)
{
    NSCursor* SelectedCursor = nullptr;
    switch(Cursor)
    {
    case ECursor::None:
    case ECursor::Arrow:
        SelectedCursor = [NSCursor arrowCursor];
        break;
            
    case ECursor::TextInput:
        SelectedCursor = [NSCursor IBeamCursor];
        break;
            
    case ECursor::ResizeAll:
        SelectedCursor = [NSCursor closedHandCursor];
        break;
            
    case ECursor::ResizeEW:
        SelectedCursor= [NSCursor respondsToSelector:@selector(_windowResizeEastWestCursor)] ? [NSCursor _windowResizeEastWestCursor] : [NSCursor resizeLeftRightCursor];
        break;
            
    case ECursor::ResizeNS:
        SelectedCursor = [NSCursor respondsToSelector:@selector(_windowResizeNorthSouthCursor)] ? [NSCursor _windowResizeNorthSouthCursor] : [NSCursor resizeUpDownCursor];
        break;
            
    case ECursor::ResizeNESW:
        SelectedCursor = [NSCursor respondsToSelector:@selector(_windowResizeNorthEastSouthWestCursor)] ? [NSCursor _windowResizeNorthEastSouthWestCursor] : [NSCursor closedHandCursor];
        break;
            
    case ECursor::ResizeNWSE:
        SelectedCursor = [NSCursor respondsToSelector:@selector(_windowResizeNorthWestSouthEastCursor)] ? [NSCursor _windowResizeNorthWestSouthEastCursor] : [NSCursor closedHandCursor];
        break;
            
    case ECursor::Hand:
        SelectedCursor = [NSCursor pointingHandCursor];
        break;
            
    case ECursor::NotAllowed:
        SelectedCursor = [NSCursor operationNotAllowedCursor];
        break;
    }
    
    if (SelectedCursor)
    {
        SelectedCursor = [NSCursor arrowCursor];
    }
    
    [SelectedCursor set];
}

void FMacCursor::SetPosition(int32 x, int32 y) const
{
    CGPoint NewPosition = CGPointMake(x, y);
    CGWarpMouseCursorPosition(CGPointMake(NewPosition.x, CGDisplayBounds(CGMainDisplayID()).size.height - NewPosition.y - 1));
    
    if (bIsVisible)
    {
        CGAssociateMouseAndMouseCursorPosition(true);
    }
}

FIntVector2 FMacCursor::GetPosition() const
{
    NSPoint CursorPosition = [NSEvent mouseLocation];
    return FIntVector2(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
}

void FMacCursor::SetVisibility(bool bVisible)
{
    if (bVisible)
    {
        if (! bIsVisible)
        {
            [NSCursor unhide];
            bIsVisible = true;
        }
    }
    else
    {
        if (bIsVisible)
        {
            [NSCursor hide];
            bIsVisible = false;
        }
    }
}
