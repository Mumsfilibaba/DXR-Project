#if PLATFORM_WINDOWS
#include "WindowsApplication.h"

#include "Core/Threading/ScopedLock.h"
#include "Core/Input/Windows/WindowsKeyMapping.h"
#include "Core/Logging/Log.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

enum EWindowsMasks : uint32
{
    ScanCodeMask = 0x01ff,
    KeyRepeatMask = 0x40000000,
    BackButtonMask = 0x0001
};

/* A small wrapper for moved message */
struct SPointMessage
{
    FORCEINLINE SPointMessage( LPARAM InParam )
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

///////////////////////////////////////////////////////////////////////////////////////////////////

CWindowsApplication* CWindowsApplication::InstancePtr = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////////

TSharedPtr<CWindowsApplication> CWindowsApplication::Make()
{
    HINSTANCE Instance = static_cast<HINSTANCE>(GetModuleHandleA( 0 ));

    // TODO: Load icon here
    return TSharedPtr<CWindowsApplication>( dbg_new CWindowsApplication( Instance ) );
}

CWindowsApplication::CWindowsApplication( HINSTANCE InInstance )
    : CPlatformApplication( CWindowsCursor::Make() )
    , Windows()
    , Messages()
    , MessagesCriticalSection()
    , WindowsMessageListeners()
    , IsTrackingMouse( false )
    , Instance( InInstance )
{
    // Always the last instance created 
    InstancePtr = this;
}

CWindowsApplication::~CWindowsApplication()
{
    Windows.Clear();

    InstancePtr = nullptr;
}

bool CWindowsApplication::RegisterWindowClass()
{
    WNDCLASS WindowClass;
    CMemory::Memzero( &WindowClass );

    WindowClass.hInstance     = Instance;
    WindowClass.lpszClassName = CWindowsApplication::GetWindowClassName();
    WindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject( BLACK_BRUSH ));
    WindowClass.hCursor       = LoadCursor( NULL, IDC_ARROW );
    WindowClass.lpfnWndProc   = CWindowsApplication::StaticMessageProc;

    ATOM ClassAtom = RegisterClass( &WindowClass );
    if ( ClassAtom == 0 )
    {
        LOG_ERROR( "[CWindowsApplication]: FAILED to register WindowClass\n" );
        return false;
    }

    return true;
}

bool CWindowsApplication::RegisterRawInputDevices( HWND Window )
{
    constexpr uint32 DeviceCount = 1;
    
    RAWINPUTDEVICE Devices[DeviceCount];
    CMemory::Memzero( devices, sizeof(Devices) );

    // Mouse
    Devices[0].dwFlags	   = 0;
    Devices[0].hwndTarget  = Window;
    Devices[0].usUsage	   = 0x02;
    Devices[0].usUsagePage = 0x01;

    bool Result = !!::RegisterRawInputDevices( Devices, DeviceCount, sizeof(RAWINPUTDEVICE) );
    if ( !Result )
    {
        LOG_ERROR("[CWindowsApplication] Failed to register Raw Input devices");
        return false;
    }
    else
    {
        LOG_DEBUG("[CWindowsApplication] Registered Raw Input devices");
        return true;
    }
}

bool CWindowsApplication::UnregisterRawInputDevices()
{
    constexpr uint32 DeviceCount = 1;

    RAWINPUTDEVICE Devices[DeviceCount];
    CMemory::Memzero( Devices, sizeof(Devices) );

    // Mouse
    Devices[0].dwFlags	   = RIDEV_REMOVE;
    Devices[0].hwndTarget  = 0;
    Devices[0].usUsage	   = 0x02;
    Devices[0].usUsagePage = 0x01;

    bool Result = !!::RegisterRawInputDevices( Devices, DeviceCount, sizeof(RAWINPUTDEVICE) );
    if ( !Result )
    {
        LOG_ERROR("[CWindowsApplication] Failed to unregister Raw Input devices");
        return false;
    }
    else
    {
        LOG_DEBUG("[CWindowsApplication] Unregistered Raw Input devices");
        return true;
    }
}

TSharedRef<CPlatformWindow> CWindowsApplication::MakeWindow()
{
    TSharedRef<CWindowsWindow> NewWindow = CWindowsWindow::Make( this );
    Windows.Emplace( NewWindow );
    return NewWindow;
}

bool CWindowsApplication::Initialize()
{
    if ( !RegisterWindowClass() )
    {
        return false;
    }

#if PLATFORM_WINDOWS_VISTA && ENABLE_DPI_AWARENESS
    SetProcessDPIAware();
#endif

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

bool CWindowsApplication::SupportsRawMouse() const
{
    return true;
}

bool CWindowsApplication::EnableRawMouse( const TSharedRef<CPlatformWindow>& Window )
{
    if ( Window )
    {
        TSharedRef<CWindowsWindow> WindowsWindow = StaticCast<CWindowsWindow>( Window );
        return RegisterRawInputDevices( WindowsWindow->GetHandle() );
    }
    else
    {
        return false;
    }
}

void CWindowsApplication::SetCapture( const TSharedRef<CPlatformWindow>& Window )
{
    if ( Window )
    {
        TSharedRef<CWindowsWindow> WindowsWindow = StaticCast<CWindowsWindow>( Window );

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

void CWindowsApplication::SetActiveWindow( const TSharedRef<CPlatformWindow>& Window )
{
    TSharedRef<CWindowsWindow> WindowsWindow = StaticCast<CWindowsWindow>( Window );

    HWND hActiveWindow = WindowsWindow->GetHandle();
    if ( WindowsWindow->IsValid() )
    {
        ::SetActiveWindow( hActiveWindow );
    }
}

TSharedRef<CPlatformWindow> CWindowsApplication::GetCapture() const
{
    // TODO: Should we add a reference here
    HWND CaptureWindow = ::GetCapture();
    return GetWindowsWindowFromHWND( CaptureWindow );
}

TSharedRef<CPlatformWindow> CWindowsApplication::GetActiveWindow() const
{
    // TODO: Should we add a reference here
    HWND ActiveWindow = ::GetActiveWindow();
    return GetWindowsWindowFromHWND( ActiveWindow );
}

TSharedRef<CWindowsWindow> CWindowsApplication::GetWindowsWindowFromHWND( HWND InWindow ) const
{
    for ( const TSharedRef<CWindowsWindow>& Window : Windows )
    {
        if ( Window->GetHandle() == InWindow )
        {
            return Window;
        }
    }

    return nullptr;
}

void CWindowsApplication::AddWindowsMessageListener( const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener )
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

void CWindowsApplication::RemoveWindowsMessageListener( const TSharedPtr<IWindowsMessageListener>& InWindowsMessageListener )
{
    WindowsMessageListeners.Remove( InWindowsMessageListener );
}

bool CWindowsApplication::IsWindowsMessageListener( const TSharedPtr<IWindowsMessageListener>& InWindowsMessageListener ) const
{
    return WindowsMessageListeners.Contains( InWindowsMessageListener );
}

LRESULT CWindowsApplication::StaticMessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    if ( InstancePtr )
    {
        return InstancePtr->MessageProc( Window, Message, wParam, lParam );
    }
    else
    {
        return DefWindowProc( Window, Message, wParam, lParam );
    }
}

void CWindowsApplication::HandleStoredMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY )
{
    TSharedRef<CWindowsWindow> MessageWindow = GetWindowsWindowFromHWND( Window );
    switch ( Message )
    {
        case WM_CLOSE:
        {
            if ( MessageWindow )
            {
                MessageListener->HandleWindowClosed( MessageWindow );
                DestroyWindow( Window );
            }

            break;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        {
            if ( MessageWindow )
            {
                const bool HasFocus = (Message == WM_SETFOCUS);
                MessageListener->HandleWindowFocusChanged( MessageWindow, HasFocus );
            }

            break;
        }

        case WM_MOUSELEAVE:
        {
            if ( MessageWindow )
            {
                MessageListener->HandleWindowMouseLeft( MessageWindow );
            }

            IsTrackingMouse = false;
            break;
        }

        case WM_SIZE:
        {
            if ( MessageWindow )
            {
                const SPointMessage Size( lParam );
                MessageListener->HandleWindowResized( MessageWindow, Size.Width, Size.Height );
            }

            break;
        }

        case WM_MOVE:
        {
            if ( MessageWindow )
            {
                const SPointMessage Size( lParam );
                MessageListener->HandleWindowMoved( MessageWindow, Size.x, Size.y );
            }

            break;
        }

        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            const uint32 ScanCode = static_cast<uint32>(HIWORD( lParam ) & ScanCodeMask);
            const EKey Key = CWindowsKeyMapping::GetKeyCodeFromScanCode( ScanCode );

            MessageListener->HandleKeyReleased( Key, PlatformApplicationMisc::GetModifierKeyState() );
            break;
        }

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            const uint32 ScanCode = static_cast<uint32>(HIWORD( lParam ) & ScanCodeMask);
            const EKey Key = CWindowsKeyMapping::GetKeyCodeFromScanCode( ScanCode );

            const bool IsRepeat = !!(lParam & KeyRepeatMask);
            MessageListener->HandleKeyPressed( Key, IsRepeat, PlatformApplicationMisc::GetModifierKeyState() );
            break;
        }

        case WM_SYSCHAR:
        case WM_CHAR:
        {
            const uint32 Character = static_cast<uint32>(wParam);
            MessageListener->HandleKeyTyped( Character );
            break;
        }

        case WM_MOUSEMOVE:
        {
            const int32 x = GET_X_LPARAM( lParam );
            const int32 y = GET_Y_LPARAM( lParam );

            if ( !IsTrackingMouse )
            {
                TRACKMOUSEEVENT TrackEvent;
                CMemory::Memzero( &TrackEvent );

                TrackEvent.cbSize = sizeof( TRACKMOUSEEVENT );
                TrackEvent.dwFlags = TME_LEAVE;
                TrackEvent.hwndTrack = Window;
                TrackMouseEvent( &TrackEvent );

                MessageListener->HandleWindowMouseEntered( MessageWindow );

                IsTrackingMouse = true;
            }

            MessageListener->HandleMouseMove( x, y );
            break;
        }

        case WM_INPUT:
        {
            MessageListener->HandleRawMouseMove( MessageWindow, MouseDeltaX, MouseDeltaY );
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

            MessageListener->HandleMousePressed( Button, PlatformApplicationMisc::GetModifierKeyState() );
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

            MessageListener->HandleMouseReleased( Button, PlatformApplicationMisc::GetModifierKeyState() );
            break;
        }

        case WM_MOUSEWHEEL:
        {
            const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM( wParam )) / static_cast<float>(WHEEL_DELTA);
            MessageListener->HandleMouseScrolled( 0.0f, WheelDelta );
            break;
        }

        case WM_MOUSEHWHEEL:
        {
            const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM( wParam )) / static_cast<float>(WHEEL_DELTA);
            MessageListener->HandleMouseScrolled( WheelDelta, 0.0f );
            break;
        }

        case WM_QUIT:
        {
            int32 ExitCode = (int32)wParam;
            MessageListener->HandleApplicationExit( ExitCode );
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
    for ( TSharedPtr<IWindowsMessageListener> NativeMessageListener : WindowsMessageListeners )
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
        case WM_INPUT:
        {
            return ProcessRawInput( Window, Message, wParam, lParam );
        }

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
            StoreMessage( Window, Message, wParam, lParam, 0, 0 );
            return ResultFromListeners; // Either zero or something else 
        }
    }

    return DefWindowProc( Window, Message, wParam, lParam );
}

void CWindowsApplication::StoreMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY )
{
    TScopedLock<CCriticalSection> Lock( MessagesCriticalSection );
    Messages.Emplace( Window, Message, wParam, lParam, MouseDeltaX, MouseDeltaY );
}

LRESULT CWindowsApplication::ProcessRawInput( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    UINT Size = 0;
    GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, NULL, &Size, sizeof(RAWINPUTHEADER) );
    
    // TODO: Measure performance impact
    TUniquePtr<Byte[]> Buffer = MakeUnique<Byte[]>( Size );

    if ( GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, Buffer.Get(), &Size, sizeof(RAWINPUTHEADER) ) != Size )
    {
        LOG_ERROR( "[CWindowsApplication] GetRawInputData did not return correct size" );
        return 0;
    }

    RAWINPUT* RawInput = reinterpret_cast<RAWINPUT*>(Buffer.Get());
    if (RawInput->header.dwType == RIM_TYPEMOUSE)
    {
        int32 DeltaX = RawInput->data.mouse.lLastX;
        int32 DeltaY = RawInput->data.mouse.lLastY;

        if ( DeltaX != 0 || DeltaY != 0 )
        {
            StoreMessage( Window, Message, wParam, lParam, DeltaX, DeltaY );
        }

        return 0;
    }
    else
    {
        return DefRawInputProc( &RawInput, 1, sizeof(RAWINPUTHEADER) );
    }
}

#endif