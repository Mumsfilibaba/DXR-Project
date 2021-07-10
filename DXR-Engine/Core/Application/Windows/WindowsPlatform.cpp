#include "WindowsPlatform.h"
#include "WindowsCursor.h"

#include "Core/Input/InputManager.h"

TArray<WindowsEvent> WindowsPlatform::Messages;

TRef<WindowsCursor> WindowsPlatform::CurrentCursor;

bool WindowsPlatform::IsTrackingMouse = false;

HINSTANCE WindowsPlatform::Instance = 0;

void WindowsPlatform::PreMainInit( HINSTANCE InInstance )
{
    Instance = InInstance;
}

bool WindowsPlatform::Init()
{
    if ( !RegisterWindowClass() )
    {
        return false;
    }

    if ( !InitCursors() )
    {
        return false;
    }

    return true;
}

bool WindowsPlatform::RegisterWindowClass()
{
    WNDCLASS WindowClass;
    Memory::Memzero( &WindowClass );

    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = WindowsPlatform::GetWindowClassName();
    WindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject( BLACK_BRUSH ));
    WindowClass.hCursor = LoadCursor( NULL, IDC_ARROW );
    WindowClass.lpfnWndProc = WindowsPlatform::MessageProc;

    ATOM ClassAtom = RegisterClass( &WindowClass );
    if ( ClassAtom == 0 )
    {
        LOG_ERROR( "[WindowsPlatform]: FAILED to register WindowClass\n" );
        return false;
    }

    return true;
}

bool WindowsPlatform::Release()
{
    return true;
}

void WindowsPlatform::Tick()
{
    MSG Message;
    while ( PeekMessage( &Message, 0, 0, 0, PM_REMOVE ) )
    {
        TranslateMessage( &Message );
        DispatchMessage( &Message );

        if ( Message.message == WM_QUIT )
        {
            StoreMessage( Message.hwnd, Message.message, Message.wParam, Message.lParam );
        }
    }

    for ( const WindowsEvent& Event : Messages )
    {
        HandleStoredMessage( Event.Window, Event.Message, Event.wParam, Event.lParam );
    }

    Messages.Clear();
}

void WindowsPlatform::HandleStoredMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    constexpr uint16 SCAN_CODE_MASK = 0x01ff;
    constexpr uint32 KEY_REPEAT_MASK = 0x40000000;
    constexpr uint16 BACK_BUTTON_MASK = 0x0001;

    TRef<WindowsWindow> MessageWindow = WindowHandle( Window ).GetWindow();
    switch ( Message )
    {
    case WM_CLOSE:
    {
        if ( MessageWindow )
        {
            Callbacks->OnWindowClosed( MessageWindow );
        }

        break;
    }

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    {
        if ( MessageWindow )
        {
            const bool HasFocus = (Message == WM_SETFOCUS);
            Callbacks->OnWindowFocusChanged( MessageWindow, HasFocus );
        }

        break;
    }

    case WM_MOUSELEAVE:
    {
        if ( MessageWindow )
        {
            Callbacks->OnWindowMouseLeft( MessageWindow );
        }

        IsTrackingMouse = false;
        break;
    }

    case WM_SIZE:
    {
        if ( MessageWindow )
        {
            const uint16 Width = LOWORD( lParam );
            const uint16 Height = HIWORD( lParam );
            Callbacks->OnWindowResized( MessageWindow, Width, Height );
        }

        break;
    }

    case WM_MOVE:
    {
        if ( MessageWindow )
        {
            const int16 x = (int16)LOWORD( lParam );
            const int16 y = (int16)HIWORD( lParam );
            Callbacks->OnWindowMoved( MessageWindow, x, y );
        }

        break;
    }

    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
        const uint32 ScanCode = static_cast<uint32>(HIWORD( lParam ) & SCAN_CODE_MASK);
        const EKey Key = InputManager::Get().ConvertFromScanCode( ScanCode );
        Callbacks->OnKeyReleased( Key, GetModifierKeyState() );
        break;
    }

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        const bool IsRepeat = !!(lParam & KEY_REPEAT_MASK);
        const uint32 ScanCode = static_cast<uint32>(HIWORD( lParam ) & SCAN_CODE_MASK);
        const EKey Key = InputManager::Get().ConvertFromScanCode( ScanCode );
        Callbacks->OnKeyPressed( Key, IsRepeat, GetModifierKeyState() );
        break;
    }

    case WM_SYSCHAR:
    case WM_CHAR:
    {
        const uint32 Character = static_cast<uint32>(wParam);
        Callbacks->OnKeyTyped( Character );
        break;
    }

    case WM_MOUSEMOVE:
    {
        const int32 x = GET_X_LPARAM( lParam );
        const int32 y = GET_Y_LPARAM( lParam );

        if ( !IsTrackingMouse )
        {
            TRACKMOUSEEVENT TrackEvent;
            Memory::Memzero( &TrackEvent );

            TrackEvent.cbSize = sizeof( TRACKMOUSEEVENT );
            TrackEvent.dwFlags = TME_LEAVE;
            TrackEvent.hwndTrack = Window;
            TrackMouseEvent( &TrackEvent );

            Callbacks->OnWindowMouseEntered( MessageWindow );

            IsTrackingMouse = true;
        }

        Callbacks->OnMouseMove( x, y );
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDBLCLK:
    {
        EMouseButton Button = EMouseButton::MouseButton_Unknown;
        if ( Message == WM_LBUTTONDOWN || Message == WM_LBUTTONDBLCLK )
        {
            Button = EMouseButton::MouseButton_Left;
        }
        else if ( Message == WM_MBUTTONDOWN || Message == WM_MBUTTONDBLCLK )
        {
            Button = EMouseButton::MouseButton_Middle;
        }
        else if ( Message == WM_RBUTTONDOWN || Message == WM_RBUTTONDBLCLK )
        {
            Button = EMouseButton::MouseButton_Right;
        }
        else if ( GET_XBUTTON_WPARAM( wParam ) == BACK_BUTTON_MASK )
        {
            Button = EMouseButton::MouseButton_Back;
        }
        else
        {
            Button = EMouseButton::MouseButton_Forward;
        }

        Callbacks->OnMousePressed( Button, GetModifierKeyState() );
        break;
    }

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_XBUTTONUP:
    {
        EMouseButton Button = EMouseButton::MouseButton_Unknown;
        if ( Message == WM_LBUTTONUP )
        {
            Button = EMouseButton::MouseButton_Left;
        }
        else if ( Message == WM_MBUTTONUP )
        {
            Button = EMouseButton::MouseButton_Middle;
        }
        else if ( Message == WM_RBUTTONUP )
        {
            Button = EMouseButton::MouseButton_Right;
        }
        else if ( GET_XBUTTON_WPARAM( wParam ) == BACK_BUTTON_MASK )
        {
            Button = EMouseButton::MouseButton_Back;
        }
        else
        {
            Button = EMouseButton::MouseButton_Forward;
        }

        Callbacks->OnMouseReleased( Button, GetModifierKeyState() );
        break;
    }

    case WM_MOUSEWHEEL:
    {
        const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM( wParam )) / static_cast<float>(WHEEL_DELTA);
        Callbacks->OnMouseScrolled( 0.0f, WheelDelta );
        break;
    }

    case WM_MOUSEHWHEEL:
    {
        const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM( wParam )) / static_cast<float>(WHEEL_DELTA);
        Callbacks->OnMouseScrolled( WheelDelta, 0.0f );
        break;
    }

    case WM_QUIT:
    {
        int32 ExitCode = (int32)wParam;
        Callbacks->OnApplicationExit( ExitCode );
        break;
    }

    default:
    {
        // Nothing for now
        break;
    }
    }
}

void WindowsPlatform::SetActiveWindow( GenericWindow* Window )
{
    TRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>( Window );
    HWND hActiveWindow = WinWindow->GetHandle();
    if ( WinWindow->IsValid() )
    {
        ::SetActiveWindow( hActiveWindow );
    }
}

void WindowsPlatform::SetCapture( GenericWindow* CaptureWindow )
{
    if ( CaptureWindow )
    {
        TRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>( CaptureWindow );
        HWND hCapture = WinWindow->GetHandle();
        if ( WinWindow->IsValid() )
        {
            ::SetCapture( hCapture );
        }
    }
    else
    {
        ReleaseCapture();
    }
}

GenericWindow* WindowsPlatform::GetActiveWindow()
{
    HWND hActiveWindow = GetForegroundWindow();
    return WindowHandle( hActiveWindow ).GetWindow();
}

GenericWindow* WindowsPlatform::GetCapture()
{
    HWND hCapture = ::GetCapture();
    return WindowHandle( hCapture ).GetWindow();
}

bool WindowsPlatform::InitCursors()
{
    if ( !(GenericCursor::Arrow = WindowsCursor::Create( IDC_ARROW )) )
    {
        return false;
    }
    if ( !(GenericCursor::TextInput = WindowsCursor::Create( IDC_IBEAM )) )
    {
        return false;
    }
    if ( !(GenericCursor::ResizeAll = WindowsCursor::Create( IDC_SIZEALL )) )
    {
        return false;
    }
    if ( !(GenericCursor::ResizeEW = WindowsCursor::Create( IDC_SIZEWE )) )
    {
        return false;
    }
    if ( !(GenericCursor::ResizeNS = WindowsCursor::Create( IDC_SIZENS )) )
    {
        return false;
    }
    if ( !(GenericCursor::ResizeNESW = WindowsCursor::Create( IDC_SIZENESW )) )
    {
        return false;
    }
    if ( !(GenericCursor::ResizeNWSE = WindowsCursor::Create( IDC_SIZENWSE )) )
    {
        return false;
    }
    if ( !(GenericCursor::Hand = WindowsCursor::Create( IDC_HAND )) )
    {
        return false;
    }
    if ( !(GenericCursor::NotAllowed = WindowsCursor::Create( IDC_NO )) )
    {
        return false;
    }

    return true;
}

void WindowsPlatform::SetCursor( GenericCursor* Cursor )
{
    if ( Cursor )
    {
        TRef<WindowsCursor> WinCursor = MakeSharedRef<WindowsCursor>( Cursor );
        CurrentCursor = WinCursor;

        HCURSOR Cursorhandle = WinCursor->GetHandle();
        ::SetCursor( Cursorhandle );
    }
    else
    {
        ::SetCursor( NULL );
    }
}

GenericCursor* WindowsPlatform::GetCursor()
{
    HCURSOR Cursor = ::GetCursor();
    if ( CurrentCursor )
    {
        WindowsCursor* WinCursor = static_cast<WindowsCursor*>(CurrentCursor.Get());
        if ( WinCursor->GetHandle() == Cursor )
        {
            CurrentCursor->AddRef();
            return CurrentCursor.Get();
        }
        else
        {
            CurrentCursor.Reset();
        }
    }

    return nullptr;
}

void WindowsPlatform::SetCursorPos( GenericWindow* RelativeWindow, int32 x, int32 y )
{
    if ( RelativeWindow )
    {
        TRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>( RelativeWindow );
        HWND hRelative = WinWindow->GetHandle();

        POINT CursorPos = { x, y };
        if ( ClientToScreen( hRelative, &CursorPos ) )
        {
            ::SetCursorPos( CursorPos.x, CursorPos.y );
        }
    }
}

void WindowsPlatform::GetCursorPos( GenericWindow* RelativeWindow, int32& OutX, int32& OutY )
{
    POINT CursorPos = { };
    if ( !::GetCursorPos( &CursorPos ) )
    {
        return;
    }

    TRef<WindowsWindow> WinRelative = MakeSharedRef<WindowsWindow>( RelativeWindow );
    if ( WinRelative )
    {
        HWND Relative = WinRelative->GetHandle();
        if ( ScreenToClient( Relative, &CursorPos ) )
        {
            OutX = CursorPos.x;
            OutY = CursorPos.y;
        }
    }
}

ModifierKeyState WindowsPlatform::GetModifierKeyState()
{
    uint32 ModifierMask = 0;
    if ( GetKeyState( VK_CONTROL ) & 0x8000 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Ctrl;
    }
    if ( GetKeyState( VK_MENU ) & 0x8000 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Alt;
    }
    if ( GetKeyState( VK_SHIFT ) & 0x8000 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Shift;
    }
    if ( GetKeyState( VK_CAPITAL ) & 1 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_CapsLock;
    }
    if ( (GetKeyState( VK_LWIN ) | GetKeyState( VK_RWIN )) & 0x8000 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Super;
    }
    if ( GetKeyState( VK_NUMLOCK ) & 1 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_NumLock;
    }

    return ModifierKeyState( ModifierMask );
}

LRESULT WindowsPlatform::MessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    switch ( Message )
    {
    case WM_CLOSE:
    case WM_MOVE:
    case WM_MOUSELEAVE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_SIZE:
    case WM_SYSKEYUP:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_SYSCHAR:
    case WM_CHAR:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_XBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    {
        StoreMessage( Window, Message, wParam, lParam );
        return 0;
    }
    }

    return DefWindowProc( Window, Message, wParam, lParam );
}

void WindowsPlatform::StoreMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    Messages.EmplaceBack( Window, Message, wParam, lParam );
}
