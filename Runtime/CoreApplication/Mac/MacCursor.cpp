#include "MacCursor.h"
#include "MacWindow.h"
#include "CocoaWindow.h"
#include "Core/Memory/Memory.h"

#include <AppKit/AppKit.h>

// These cursors are available but not documented
// See: https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_osx.mm
@interface NSCursor()

+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;

@end

FMacCursor::FMacCursor()
    : FGenericCursor()
    , CurrentPosition()
    , bIsPositionInitialized(false)
{
}

FMacCursor::~FMacCursor()
{
}

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
    
    if (!SelectedCursor)
    {
        SelectedCursor = [NSCursor arrowCursor];
    }
    
    [SelectedCursor set];
}

void FMacCursor::SetPosition(int32 x, int32 y)
{
    CGAssociateMouseAndMouseCursorPosition(false);
    
    const CGRect DisplayBounds = CGDisplayBounds(CGMainDisplayID());
    CGPoint NewPosition = CGPointMake(x, y);
    NewPosition = CGPointMake(NewPosition.x, DisplayBounds.size.height - NewPosition.y);
    CGWarpMouseCursorPosition(NewPosition);
    
    CGAssociateMouseAndMouseCursorPosition(true);

    UpdateCursorPosition(FIntVector2(NewPosition.x, NewPosition.y));
}

FIntVector2 FMacCursor::GetPosition() const
{
    if (bIsPositionInitialized)
    {
        return CurrentPosition;
    }
    
    const NSPoint MouseLocation  = [NSEvent mouseLocation];
    const NSPoint CursorPosition = FMacApplication::ConvertCocoaPointToEngine(MouseLocation.x, MouseLocation.y);
    return FIntVector2(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
}

void FMacCursor::SetVisibility(bool bVisible)
{
    if (bVisible)
    {
        if (!bIsVisible)
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

void FMacCursor::UpdateCursorPosition(const FIntVector2& InPosition)
{
    CurrentPosition = InPosition;
    bIsPositionInitialized = true;
}
