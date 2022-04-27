#if PLATFORM_MACOS
#include "MacCursor.h"
#include "MacWindow.h"
#include "CocoaWindow.h"

#include "Core/Memory/Memory.h"

#include <AppKit/AppKit.h>

/*
These cursors are available but not documented
See: https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_osx.mm
 */
@interface NSCursor()
+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;
@end

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

TSharedPtr<CMacCursor> CMacCursor::Make()
{
	return TSharedPtr<CMacCursor>(dbg_new CMacCursor());
}

void CMacCursor::SetCursor(ECursor Cursor)
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

void CMacCursor::SetPosition(CGenericWindow* InRelativeWindow, int32 x, int32 y) const
{
    CGPoint NewPosition;
    if (InRelativeWindow)
    {
		CCocoaWindow* RelativeWindow = reinterpret_cast<CCocoaWindow*>(InRelativeWindow->GetPlatformHandle());
        const NSRect ContentRect = [RelativeWindow frame];
        const NSRect LocalRect   = NSMakeRect(x, ContentRect.size.height - y - 1, 0, 0);
        const NSRect GlobalRect  = [RelativeWindow convertRectToScreen:LocalRect];
        NewPosition = CGPointMake(GlobalRect.origin.x, GlobalRect.origin.y);
    }
    else
    {
        NewPosition = CGPointMake(x, y);
    }
    
    CGWarpMouseCursorPosition(CGPointMake(NewPosition.x, CGDisplayBounds(CGMainDisplayID()).size.height - NewPosition.y - 1));
    
    if (bIsVisible)
    {
        CGAssociateMouseAndMouseCursorPosition(true);
    }
}

void CMacCursor::GetPosition(CGenericWindow* InRelativeWindow, int32& OutX, int32& OutY) const
{
    NSPoint CursorPosition;
    if (InRelativeWindow)
    {
        NSWindow* RelativeWindow = reinterpret_cast<CCocoaWindow*>(InRelativeWindow->GetPlatformHandle());
        CursorPosition = [RelativeWindow mouseLocationOutsideOfEventStream];
    }
    else
    {
        CursorPosition = [NSEvent mouseLocation];
    }
    
    OutX = CursorPosition.x;
    OutY = CursorPosition.y;
}

void CMacCursor::SetVisibility(bool bVisible)
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

#endif
