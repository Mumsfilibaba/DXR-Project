#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Platform/PlatformApplicationMisc.h"

#include "Core/Threading/ScopedLock.h"

#include "WindowsApplication.h"
#include "WindowsKeyMapping.h"

enum EWindowsMasks : uint32
{
    ScanCodeMask   = 0x01ff,
    KeyRepeatMask  = 0x40000000,
    BackButtonMask = 0x0001
};

/* A small wrapper for moved message */
struct SSizeMessage
{
    FORCEINLINE SSizeMessage( LPARAM InParam )
        : Param( InParam )
    {
    }

    union
    {
        /* Used for resize messages */
        struct
        {
            uint16 Width;
            uint16 Height;
        };

        /* Used for move messages */
        struct
        {
            int16 x;
            int16 y;
        };

        LPARAM Param;
    };
};

/* Global instance of the windows- application */
CWindowsApplication* GWindowsApplication = nullptr;

/* Create the application and load icon */
CWindowsApplication* CWindowsApplication::Make()
{
    HINSTANCE Instance = (HINSTANCE)GetModuleHandleA( 0 );

    // TODO: Load icon here
    return new CWindowsApplication( Instance );
}

CWindowsApplication::CWindowsApplication( HINSTANCE InInstance )
    : CGenericApplication()
    , Instance( InInstance )
    , Windows()
    , Messages()
    , WindowsMessageListeners()
    , IsTrackingMouse( false )
    , Cursor()
    , Keyboard()
{
    // Always the last instance created 
    GWindowsApplication = this;
}

CWindowsApplication::~CWindowsApplication()
{
    Windows.Clear();

    // Should be all cases but this protect those times that it is not true
    if ( GWindowsApplication )
    {
        GWindowsApplication = nullptr;
    }
}

bool CWindowsApplication::RegisterWindowClass()
{
    WNDCLASS WindowClass;
    Memory::Memzero( &WindowClass );

    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = CWindowsApplication::GetWindowClassName();
    WindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject( BLACK_BRUSH ));
    WindowClass.hCursor = LoadCursor( NULL, IDC_ARROW );
    WindowClass.lpfnWndProc = CWindowsApplication::StaticMessageProc;

    ATOM ClassAtom = RegisterClass( &WindowClass );
    if ( ClassAtom == 0 )
    {
        LOG_ERROR( "[CWindowsApplication]: FAILED to register WindowClass\n" );
        return false;
    }

    return true;
}

CGenericWindow* CWindowsApplication::MakeWindow()
{
    TSharedRef<CWindowsWindow> NewWindow = new CWindowsWindow( this );
    Windows.Emplace( NewWindow );
    return NewWindow.Get();
}

bool CWindowsApplication::Init()
{
    if ( !RegisterWindowClass() )
    {
        return false;
    }

    CWindowsKeyMapping::Init();

    return true;
}

void CWindowsApplication::Tick( float )
{
    // Start by pumping all the messages 
    PlatformApplicationMisc::PumpMessages( true );

    // TODO: Store the second TArray to save on allocations
    TArray<SWindowsMessage> ProcessableMessages;
    {
        TScopedLock<CCriticalSection> Lock( MessagesCriticalSection );
        ProcessableMessages.Append( Messages );
        Messages.Clear();
    }

    // Handle all the messages 
    for ( const SWindowsMessage& Message : ProcessableMessages )
    {
        HandleStoredMessage( Message.Window, Message.Message, Message.wParam, Message.lParam );
    }
}

void CWindowsApplication::Release()
{
    delete this;
}

void CWindowsApplication::SetCapture( CGenericWindow* Window )
{
    if ( Window )
    {
        TSharedRef<CWindowsWindow> WindowsWindow = MakeSharedRef<CWindowsWindow>( Window );

        HWND hCapture = WindowsWindow->GetHandle();
        if ( WindowsWindow->IsValid() )
        {
            ::SetCapture( hCapture );
        }
    }
    else
    {
        ReleaseCapture();
    }
}

void CWindowsApplication::SetActiveWindow( CGenericWindow* Window )
{
    TSharedRef<CWindowsWindow> WindowsWindow = MakeSharedRef<CWindowsWindow>( Window );

    HWND hActiveWindow = WindowsWindow->GetHandle();
    if ( WindowsWindow->IsValid() )
    {
        ::SetActiveWindow( hActiveWindow );
    }
}

CGenericWindow* CWindowsApplication::GetCapture() const
{
    // TODO: Should we add a reference here
    HWND CaptureWindow = ::GetCapture();
    return GetWindowsWindowFromHWND( CaptureWindow );
}

CGenericWindow* CWindowsApplication::GetActiveWindow() const
{
    // TODO: Should we add a reference here
    HWND ActiveWindow = ::GetActiveWindow();
    return GetWindowsWindowFromHWND( ActiveWindow );
}

CWindowsWindow* CWindowsApplication::GetWindowsWindowFromHWND( HWND InWindow ) const
{
    for ( const TSharedRef<CWindowsWindow>& Window : Windows )
    {
        if ( Window->GetHandle() == InWindow )
        {
            return Window.Get();
        }
    }

    return nullptr;
}

void CWindowsApplication::AddWindowsMessageListener( IWindowsMessageListener* NewWindowsMessageListener )
{
    // We do not want to add a listener if it already exists
    if ( !IsWindowsMessageListener( NewWindowsMessageListener ) )
    {
        WindowsMessageListeners.Emplace( NewWindowsMessageListener );
    }
    else
    {
        LOG_WARNING( "Tried to add an listener that already exists" );
    }
}

void CWindowsApplication::RemoveWindowsMessageListener( IWindowsMessageListener* InWindowsMessageListener )
{
    for ( uint32 Index = 0; Index < WindowsMessageListeners.Size(); Index++ )
    {
        if ( WindowsMessageListeners[Index] == InWindowsMessageListener )
        {
            WindowsMessageListeners.RemoveAt( Index );
        }
    }
}

bool CWindowsApplication::IsWindowsMessageListener( IWindowsMessageListener* InWindowsMessageListener ) const
{
    for ( IWindowsMessageListener* WindowsMessageListener : WindowsMessageListeners )
    {
        if ( WindowsMessageListener == InWindowsMessageListener )
        {
            return true;
        }
    }

    return false;
}

LRESULT CWindowsApplication::StaticMessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    if ( GWindowsApplication )
    {
        return GWindowsApplication->MessageProc( Window, Message, wParam, lParam );
    }
    else
    {
        return DefWindowProc( Window, Message, wParam, lParam );
    }
}

void CWindowsApplication::HandleStoredMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    TSharedRef<CWindowsWindow> MessageWindow = MakeSharedRef<CWindowsWindow>( GetWindowsWindowFromHWND( Window ) );
    switch ( Message )
    {
        case WM_CLOSE:
        {
            if ( MessageWindow )
            {
                MessageListener->OnWindowClosed( MessageWindow );
            }

            break;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        {
            if ( MessageWindow )
            {
                const bool HasFocus = (Message == WM_SETFOCUS);
                MessageListener->OnWindowFocusChanged( MessageWindow, HasFocus );
            }

            break;
        }

        case WM_MOUSELEAVE:
        {
            if ( MessageWindow )
            {
                MessageListener->OnWindowMouseLeft( MessageWindow );
            }

            IsTrackingMouse = false;
            break;
        }

        case WM_SIZE:
        {
            if ( MessageWindow )
            {
                const SSizeMessage Size( lParam );
                MessageListener->OnWindowResized( MessageWindow, Size.Width, Size.Height );
            }

            break;
        }

        case WM_MOVE:
        {
            if ( MessageWindow )
            {
                const SSizeMessage Size( lParam );
                MessageListener->OnWindowMoved( MessageWindow, Size.x, Size.y );
            }

            break;
        }

        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            const uint32 ScanCode = static_cast<uint32>(HIWORD( lParam ) & ScanCodeMask);
            const EKey Key = CWindowsKeyMapping::GetKeyCodeFromScanCode( ScanCode );
            MessageListener->OnKeyReleased( Key, PlatformApplicationMisc::GetModifierKeyState() );
            break;
        }

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            const bool IsRepeat = !!(lParam & KeyRepeatMask);
            const uint32 ScanCode = static_cast<uint32>(HIWORD( lParam ) & ScanCodeMask);
            const EKey Key = CWindowsKeyMapping::GetKeyCodeFromScanCode( ScanCode );
            MessageListener->OnKeyPressed( Key, IsRepeat, PlatformApplicationMisc::GetModifierKeyState() );
            break;
        }

        case WM_SYSCHAR:
        case WM_CHAR:
        {
            const uint32 Character = static_cast<uint32>(wParam);
            MessageListener->OnKeyTyped( Character );
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

                MessageListener->OnWindowMouseEntered( MessageWindow );

                IsTrackingMouse = true;
            }

            MessageListener->OnMouseMove( x, y );
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
            else if ( GET_XBUTTON_WPARAM( wParam ) == BackButtonMask )
            {
                Button = EMouseButton::MouseButton_Back;
            }
            else
            {
                Button = EMouseButton::MouseButton_Forward;
            }

            MessageListener->OnMousePressed( Button, PlatformApplicationMisc::GetModifierKeyState() );
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
            else if ( GET_XBUTTON_WPARAM( wParam ) == BackButtonMask )
            {
                Button = EMouseButton::MouseButton_Back;
            }
            else
            {
                Button = EMouseButton::MouseButton_Forward;
            }

            MessageListener->OnMouseReleased( Button, PlatformApplicationMisc::GetModifierKeyState() );
            break;
        }

        case WM_MOUSEWHEEL:
        {
            const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM( wParam )) / static_cast<float>(WHEEL_DELTA);
            MessageListener->OnMouseScrolled( 0.0f, WheelDelta );
            break;
        }

        case WM_MOUSEHWHEEL:
        {
            const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM( wParam )) / static_cast<float>(WHEEL_DELTA);
            MessageListener->OnMouseScrolled( WheelDelta, 0.0f );
            break;
        }

        case WM_QUIT:
        {
            int32 ExitCode = (int32)wParam;
            MessageListener->OnApplicationExit( ExitCode );
            break;
        }

        default:
        {
            // Nothing for now
            break;
        }
    }
}

LRESULT CWindowsApplication::MessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    // Let the message listeners (a.k.a other modules) listen to native messages
    LRESULT ResultFromListeners = 0;
    for ( IWindowsMessageListener* NativeMessageListener : WindowsMessageListeners )
    {
        Assert( NativeMessageListener != nullptr );

        LRESULT TempResult = NativeMessageListener->MessageProc( Window, Message, wParam, lParam );
        if ( TempResult )
        {
            // TODO: Maybe some more checking?
            ResultFromListeners = TempResult;
        }
    }

    // Store relevant messages 
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
            return ResultFromListeners; // Either zero or something else 
        }
    }

    return DefWindowProc( Window, Message, wParam, lParam );
}

void CWindowsApplication::StoreMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    TScopedLock<CCriticalSection> Lock( MessagesCriticalSection );
    Messages.Emplace( Window, Message, wParam, lParam );
}

#endif