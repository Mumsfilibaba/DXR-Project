#if defined(PLATFORM_WINDOWS)
#include "WindowsCursorDevice.h"
#include "WindowsWindow.h"
#include "Windows.h"

void CWindowsCursorDevice::SetCursor( ECursor Cursor )
{
    LPSTR CursorName = NULL;
    switch ( Cursor )
    {
        case ECursor::Arrow:
            CursorName = IDC_ARROW;
            break;
        case ECursor::TextInput:
            CursorName = IDC_IBEAM;
            break;
        case ECursor::ResizeAll:
            CursorName = IDC_SIZEALL;
            break;
        case ECursor::ResizeEW:
            CursorName = IDC_SIZEWE;
            break;
        case ECursor::ResizeNS:
            CursorName = IDC_SIZENS;
            break;
        case ECursor::ResizeNESW:
            CursorName = IDC_SIZENESW;
            break;
        case ECursor::ResizeNWSE:
            CursorName = IDC_SIZENWSE;
            break;
        case ECursor::Hand:
            CursorName = IDC_HAND;
            break;
        case ECursor::NotAllowed:
            CursorName = IDC_NO;
            break;
        default:
            CursorName = NULL;
            break;
    }

    HCURSOR CursorHandle = ::LoadCursor( NULL, CursorName );
    if ( CursorHandle )
    {
        ::SetCursor( CursorHandle );
    }

    // TODO: Log error
}

void CWindowsCursorDevice::SetCursorPosition( CCoreWindow* RelativeWindow, int32 x, int32 y ) const
{
    POINT CursorPos = { x, y };
    if ( RelativeWindow )
    {
        TSharedRef<CWindowsWindow> WinWindow = MakeSharedRef<CWindowsWindow>( RelativeWindow );

        HWND hRelative = WinWindow->GetHandle();
        if ( !ClientToScreen( hRelative, &CursorPos ) )
        {
            return;
        }
    }

    ::SetCursorPos( CursorPos.x, CursorPos.y );
}

void CWindowsCursorDevice::GetCursorPosition( CCoreWindow* RelativeWindow, int32& OutX, int32& OutY ) const
{
    POINT CursorPos = { };
    if ( !::GetCursorPos( &CursorPos ) )
    {
        return;
    }

    if ( RelativeWindow )
    {
        TSharedRef<CWindowsWindow> WinRelative = MakeSharedRef<CWindowsWindow>( RelativeWindow );

        HWND Relative = WinRelative->GetHandle();
        if ( !ScreenToClient( Relative, &CursorPos ) )
        {
            return;
        }
    }

    OutX = CursorPos.x;
    OutY = CursorPos.y;
}

void CWindowsCursorDevice::SetVisibility( bool Visible )
{
    // TODO: Investigate if we need to do more in order to keep track of the ShowCursor calls
    if ( Visible )
    {
        if ( !IsCursorVisible )
        {
            ShowCursor( true );
        }
    }
    else
    {
        if ( IsCursorVisible )
        {
            ShowCursor( false );
        }
    }
}

#endif