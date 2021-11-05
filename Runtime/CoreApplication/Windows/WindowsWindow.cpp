
#if defined(PLATFORM_WINDOWS)
#include "WindowsWindow.h"
#include "WindowsApplication.h"

CWindowsWindow::CWindowsWindow( CWindowsApplication* InApplication )
    : CPlatformWindow()
    , Application( InApplication )
    , Window( 0 )
    , Style( 0 )
    , StyleEx( 0 )
    , IsFullscreen( false )
{
}

CWindowsWindow::~CWindowsWindow()
{
    if ( IsValid() )
    {
        DestroyWindow( Window );
    }
}

bool CWindowsWindow::Init( const CString& InTitle, uint32 InWidth, uint32 InHeight, SWindowStyle InStyle )
{
    // Determine the window style for WinAPI
    DWORD dwStyle = 0;
    if ( InStyle.Style != 0 )
    {
        dwStyle = WS_OVERLAPPED;
        if ( InStyle.IsTitled() )
        {
            dwStyle |= WS_CAPTION;
        }
        if ( InStyle.IsClosable() )
        {
            dwStyle |= WS_SYSMENU;
        }
        if ( InStyle.IsMinimizable() )
        {
            dwStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
        }
        if ( InStyle.IsMaximizable() )
        {
            dwStyle |= WS_SYSMENU | WS_MAXIMIZEBOX;
        }
        if ( InStyle.IsResizeable() )
        {
            dwStyle |= WS_THICKFRAME;
        }
    }
    else
    {
        dwStyle = WS_POPUP;
    }

    // Calculate real window size, since the width and height describe the client- area
    RECT ClientRect = { 0, 0, static_cast<LONG>(InWidth), static_cast<LONG>(InHeight) };
    AdjustWindowRect( &ClientRect, dwStyle, FALSE );

    INT nWidth = ClientRect.right - ClientRect.left;
    INT nHeight = ClientRect.bottom - ClientRect.top;

    HINSTANCE Instance = Application->GetInstance();
    LPCSTR WindowClassName = CWindowsApplication::GetWindowClassName();
    Window = CreateWindowEx( 0, WindowClassName, InTitle.CStr(), dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, Instance, NULL );
    if ( Window == 0 )
    {
        LOG_ERROR( "[CWindowsWindow]: FAILED to create window\n" );
        return false;
    }
    else
    {
        // If the window has a sys-menu we check if the close-button should be active
        if ( dwStyle & WS_SYSMENU )
        {
            if ( !(InStyle.IsClosable()) )
            {
                EnableMenuItem( GetSystemMenu( Window, FALSE ), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );
            }
        }

        // Save style for later
        Style = dwStyle;
        StyleParams = InStyle;

        // Set this to userdata
        SetLastError( 0 );

        LONG_PTR Result = SetWindowLongPtrA( Window, GWLP_USERDATA, (LONG_PTR)this );
        DWORD LastError = GetLastError();
        if ( Result == 0 && LastError != 0 )
        {
            LOG_ERROR( "[CWindowsWindow]: FAILED to Setup window-data\n" );
            return false;
        }

        UpdateWindow( Window );
        return true;
    }
}

void CWindowsWindow::Show( bool Maximized )
{
    Assert( Window != 0 );

    if ( IsValid() )
    {
        if ( Maximized )
        {
            ShowWindow( Window, SW_SHOWMAXIMIZED );
        }
        else
        {
            ShowWindow( Window, SW_NORMAL );
        }
    }
}

void CWindowsWindow::Close()
{
    Assert( Window != 0 );

    if ( IsValid() )
    {
        if ( StyleParams.IsClosable() )
        {
            CloseWindow( Window );
        }
    }
}

void CWindowsWindow::Minimize()
{
    Assert( Window != 0 );

    if ( StyleParams.IsMinimizable() )
    {
        if ( IsValid() )
        {
            ShowWindow( Window, SW_MINIMIZE );
        }
    }
}

void CWindowsWindow::Maximize()
{
    if ( StyleParams.IsMaximizable() )
    {
        if ( IsValid() )
        {
            ShowWindow( Window, SW_MAXIMIZE );
        }
    }
}

void CWindowsWindow::Restore()
{
    Assert( Window != 0 );

    if ( IsValid() )
    {
        bool result = ::IsIconic( Window );
        if ( result )
        {
            ::ShowWindow( Window, SW_RESTORE );
        }
    }
}

void CWindowsWindow::ToggleFullscreen()
{
    Assert( Window != 0 );

    if ( IsValid() )
    {
        if ( !IsFullscreen )
        {
            IsFullscreen = true;

            ::GetWindowPlacement( Window, &StoredPlacement );
            if ( Style == 0 )
            {
                Style = ::GetWindowLong( Window, GWL_STYLE );
            }
            if ( StyleEx == 0 )
            {
                StyleEx = ::GetWindowLong( Window, GWL_EXSTYLE );
            }

            LONG newStyle = Style;
            newStyle &= ~WS_BORDER;
            newStyle &= ~WS_DLGFRAME;
            newStyle &= ~WS_THICKFRAME;

            LONG newStyleEx = StyleEx;
            newStyleEx &= ~WS_EX_WINDOWEDGE;

            SetWindowLong( Window, GWL_STYLE, newStyle | WS_POPUP );
            SetWindowLong( Window, GWL_EXSTYLE, newStyleEx | WS_EX_TOPMOST );
            ShowWindow( Window, SW_SHOWMAXIMIZED );
        }
        else
        {
            IsFullscreen = false;

            SetWindowLong( Window, GWL_STYLE, Style );
            SetWindowLong( Window, GWL_EXSTYLE, StyleEx );
            ShowWindow( Window, SW_SHOWNORMAL );
            SetWindowPlacement( Window, &StoredPlacement );
        }
    }
}

bool CWindowsWindow::IsValid() const
{
    return IsWindow( Window ) == TRUE;
}

bool CWindowsWindow::IsActiveWindow() const
{
    HWND hActive = GetForegroundWindow();
    return (hActive == Window);
}

void CWindowsWindow::SetTitle( const CString& Title )
{
    Assert( Window != 0 );

    if ( StyleParams.IsTitled() )
    {
        if ( IsValid() )
        {
            SetWindowTextA( Window, Title.CStr() );
        }
    }
}

void CWindowsWindow::GetTitle( CString& OutTitle )
{
    if ( IsValid() )
    {
        int32 Size = GetWindowTextLengthA( Window );
        OutTitle.Resize( Size );

        GetWindowTextA( Window, OutTitle.Data(), Size );
    }
}

void CWindowsWindow::SetWindowShape( const SWindowShape& Shape, bool Move )
{
    Assert( Window != 0 );

    if ( IsValid() )
    {
        UINT Flags = SWP_NOZORDER | SWP_NOACTIVATE;
        if ( !Move )
        {
            Flags |= SWP_NOMOVE;
        }

        SetWindowPos( Window, NULL, Shape.Position.x, Shape.Position.y, Shape.Width, Shape.Height, Flags );
    }
}

void CWindowsWindow::GetWindowShape( SWindowShape& OutWindowShape ) const
{
    Assert( Window != 0 );

    if ( IsValid() )
    {
        int32 x = 0;
        int32 y = 0;
        uint32 Width = 0;
        uint32 Height = 0;

        RECT Rect = { };
        if ( GetWindowRect( Window, &Rect ) != 0 )
        {
            x = static_cast<int32>(Rect.left);
            y = static_cast<int32>(Rect.top);
        }

        if ( GetClientRect( Window, &Rect ) != 0 )
        {
            Width = static_cast<uint32>(Rect.right - Rect.left);
            Height = static_cast<uint32>(Rect.bottom - Rect.top);
        }

        OutWindowShape = SWindowShape( Width, Height, x, y );
    }
}

uint32 CWindowsWindow::GetWidth() const
{
    if ( IsValid() )
    {
        RECT Rect = { };
        if ( GetClientRect( Window, &Rect ) != 0 )
        {
            const uint32 Width = static_cast<uint32>(Rect.right - Rect.left);
            return Width;
        }
    }

    return 0;
}

uint32 CWindowsWindow::GetHeight() const
{
    if ( IsValid() )
    {
        RECT Rect = { };
        if ( GetClientRect( Window, &Rect ) != 0 )
        {
            const uint32 Height = static_cast<uint32>(Rect.bottom - Rect.top);
            return Height;
        }
    }

    return 0;
}

#endif