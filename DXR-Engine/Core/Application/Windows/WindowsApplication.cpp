#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Platform/PlatformApplicationMisc.h"

#include "WindowsApplication.h"
#include "WindowsKeyMapping.h"

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
    , Instance(InInstance)
    , Windows()
    , Messages()
    , WindowsMessageListeners()
    , IsTrackingMouse(false)
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

CWindowsWindow* CWindowsApplication::MakeWindow()
{
    TSharedRef<CWindowsWindow> NewWindow = new CWindowsWindow( this );
    Windows.Emplace(NewWindow);
    return NewWindow->Get();
}

bool CWindowsApplication::Init()
{
    if (!RegisterWindowClass())
    {
        return false;
    }

    CWindowsKeyMapping::Init();

    return true;
}

LRESULT CWindowsApplication::StaticMessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    if (GWindowsApplication)
    {
        return GWindowsApplication->MessageProc( Window, Message, wParam, lParam );
    }
    else
    {
        return DefWindowProc( Window, Message, wParam, lParam );
    }
}

LRESULT CWindowsApplication::MessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam )
{
    // Let the message listeners (a.k.a other modules) listen to native messages
    LRESULT ResultFromListeners = 0;
    for ( IWindowsMessageListener* MessageListener : WindowsMessageListeners )
    {
        Assert( MessageListener != nullptr );
        
        LRESULT TempResult = MessageListener->MessageProc( Window, Message, wParam, lParam );
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
    Messages.Emplace( Window, Message, wParam, lParam );
}

#endif