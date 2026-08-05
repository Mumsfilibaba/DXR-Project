#include "Core/Memory/Memory.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "CoreApplication/Mac/MacCursor.h"
#include "CoreApplication/Mac/MacWindow.h"
#include "CoreApplication/Mac/CocoaWindow.h"
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
    , CurrentCursor(ECursor::Arrow)
    , bIsPositionInitialized(false)
    , bIsCursorInitialized(false)
{
}

FMacCursor::~FMacCursor()
{
}

void FMacCursor::SetCursor(ECursor Cursor)
{
    if (bIsCursorInitialized && CurrentCursor == Cursor)
    {
        return;
    }

    CurrentCursor        = Cursor;
    bIsCursorInitialized = true;

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();

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
    }, NSDefaultRunLoopMode, false);
}

void FMacCursor::SetPosition(int32 x, int32 y)
{
    const NSPoint CocoaPosition   = FMacApplication::ConvertEnginePointToCocoa(static_cast<CGFloat>(x), static_cast<CGFloat>(y));
    const CGRect  MainDisplayRect = CGDisplayBounds(CGMainDisplayID());

    CGWarpMouseCursorPosition(CGPointMake(CocoaPosition.x, MainDisplayRect.size.height - CocoaPosition.y));

    UpdateCursorPosition(IntVector2(x, y));
}

IntVector2 FMacCursor::GetPosition() const
{
    if (bIsPositionInitialized)
    {
        return CurrentPosition;
    }
    
    const NSPoint MouseLocation = FMacThreadManager::Get().MainThreadDispatchAndReturn(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();
        return [NSEvent mouseLocation];
    }, NSDefaultRunLoopMode);

    const NSPoint CursorPosition = FMacApplication::ConvertCocoaPointToEngine(MouseLocation.x, MouseLocation.y);
    return IntVector2(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
}

void FMacCursor::SetVisibility(bool bVisible)
{
    if (bIsVisible == bVisible)
    {
        return;
    }

    bIsVisible = bVisible;

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CHECK_COCOA_MAIN_THREAD();

        if (bVisible)
        {
            [NSCursor unhide];
        }
        else
        {
            [NSCursor hide];
        }
    }, NSDefaultRunLoopMode, false);
}

void FMacCursor::UpdateCursorPosition(const IntVector2& InPosition)
{
    CurrentPosition = InPosition;
    bIsPositionInitialized = true;
}
