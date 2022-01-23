
#if PLATFORM_WINDOWS
#include "WindowsWindow.h"

#include "Core/Logging/Log.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformDebugMisc.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// WindowsWindow

CWindowsWindow::CWindowsWindow(CWindowsApplication* InApplication)
    : CPlatformWindow()
    , Application(InApplication)
    , Window(0)
    , bIsFullscreen(false)
    , StoredPlacement()
    , Style(0)
    , StyleEx(0)
{
}

CWindowsWindow::~CWindowsWindow()
{
    if (IsValid())
    {
        DestroyWindow(Window);
    }
}

TSharedRef<CWindowsWindow> CWindowsWindow::Make(CWindowsApplication* InApplication)
{
    return dbg_new CWindowsWindow(InApplication);
}

bool CWindowsWindow::Initialize(const CString& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle InStyle)
{
    // Determine the window style for WinAPI
    DWORD NewStyle = 0;
    DWORD NewStyleEx = WS_EX_APPWINDOW;
    if (InStyle.Style != 0)
    {
        NewStyle = WS_OVERLAPPED;
        if (InStyle.IsTitled())
        {
            NewStyle |= WS_CAPTION;
        }
        if (InStyle.IsClosable())
        {
            NewStyle |= WS_SYSMENU;
        }
        if (InStyle.IsMinimizable())
        {
            NewStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
        }
        if (InStyle.IsMaximizable())
        {
            NewStyle |= WS_SYSMENU | WS_MAXIMIZEBOX;
        }
        if (InStyle.IsResizeable())
        {
            NewStyle |= WS_THICKFRAME;
        }
    }
    else
    {
        NewStyle = WS_POPUP;
    }

    // Calculate real window size, since the width and height describe the client- area
    RECT ClientRect =
    {
        0,
        0,
        static_cast<LONG>(InWidth),
        static_cast<LONG>(InHeight)
    };

#if PLATFORM_WINDOWS_10_ANNIVERSARY
    AdjustWindowRectExForDpi(&ClientRect, NewStyle, false, NewStyleEx, USER_DEFAULT_SCREEN_DPI);
#else
    AdjustWindowRectEx(&ClientRect, NewStyle, false, NewStyleEx);
#endif

    int32 PositionX = x;
    int32 PositionY = y;
    int32 RealWidth = ClientRect.right - ClientRect.left;
    int32 RealHeight = ClientRect.bottom - ClientRect.top;

    HINSTANCE Instance = Application->GetInstance();
    LPCSTR WindowClassName = PlatformApplication::GetWindowClassName();

    Window = CreateWindowEx(NewStyleEx, WindowClassName, InTitle.CStr(), NewStyle, PositionX, PositionY, RealWidth, RealHeight, NULL, NULL, Instance, NULL);
    if (Window == 0)
    {
        LOG_ERROR("[CWindowsWindow]: FAILED to create window\n");
        return false;
    }
    else
    {
        // If the window has a sys-menu we check if the close-button should be active
        if (NewStyle & WS_SYSMENU)
        {
            if (!(InStyle.IsClosable()))
            {
                EnableMenuItem(GetSystemMenu(Window, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            }
        }

        // Save style for later
        StyleParams = InStyle;

        // Set this to userdata
        SetLastError(0);

        LONG_PTR Result = SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)this);

        DWORD LastError = GetLastError();
        if (Result == 0 && LastError != 0)
        {
            LOG_ERROR("[CWindowsWindow]: FAILED to Setup window-data\n");
            return false;
        }

        UpdateWindow(Window);

        SWindowShape NewWindowShape(RealWidth, RealHeight, PositionX, PositionY);
        SetWindowShape(NewWindowShape, true);

        return true;
    }
}

void CWindowsWindow::Show(bool bMaximized)
{
    Assert(Window != 0);

    if (IsValid())
    {
        if (bMaximized)
        {
            ShowWindow(Window, SW_SHOWMAXIMIZED);
        }
        else
        {
            ShowWindow(Window, SW_NORMAL);
        }
    }
}

void CWindowsWindow::Close()
{
    Assert(Window != 0);

    if (IsValid())
    {
        if (StyleParams.IsClosable())
        {
            CloseWindow(Window);
        }
    }
}

void CWindowsWindow::Minimize()
{
    Assert(Window != 0);

    if (StyleParams.IsMinimizable())
    {
        if (IsValid())
        {
            ShowWindow(Window, SW_MINIMIZE);
        }
    }
}

void CWindowsWindow::Maximize()
{
    if (StyleParams.IsMaximizable())
    {
        if (IsValid())
        {
            ShowWindow(Window, SW_MAXIMIZE);
        }
    }
}

void CWindowsWindow::Restore()
{
    Assert(Window != 0);

    if (IsValid())
    {
        bool bResult = IsIconic(Window);
        if (bResult)
        {
            ShowWindow(Window, SW_RESTORE);
        }
    }
}

void CWindowsWindow::ToggleFullscreen()
{
    Assert(Window != 0);

    if (IsValid())
    {
        if (!bIsFullscreen)
        {
            bIsFullscreen = true;

            GetWindowPlacement(Window, &StoredPlacement);

            if (Style == 0)
            {
                Style = GetWindowLong(Window, GWL_STYLE);
            }

            if (StyleEx == 0)
            {
                StyleEx = GetWindowLong(Window, GWL_EXSTYLE);
            }

            LONG NewStyle = Style;
            NewStyle &= ~WS_BORDER;
            NewStyle &= ~WS_DLGFRAME;
            NewStyle &= ~WS_THICKFRAME;

            LONG NewStyleEx = StyleEx;
            NewStyleEx &= ~WS_EX_WINDOWEDGE;

            SetWindowLong(Window, GWL_STYLE, NewStyle | WS_POPUP);
            SetWindowLong(Window, GWL_EXSTYLE, NewStyleEx | WS_EX_TOPMOST);
            ShowWindow(Window, SW_SHOWMAXIMIZED);
        }
        else
        {
            bIsFullscreen = false;

            SetWindowLong(Window, GWL_STYLE, Style);
            SetWindowLong(Window, GWL_EXSTYLE, StyleEx);
            ShowWindow(Window, SW_SHOWNORMAL);
            SetWindowPlacement(Window, &StoredPlacement);
        }
    }
}

bool CWindowsWindow::IsValid() const
{
    return IsWindow(Window) == TRUE;
}

bool CWindowsWindow::IsActiveWindow() const
{
    HWND hActive = GetForegroundWindow();
    return (hActive == Window);
}

void CWindowsWindow::SetTitle(const CString& Title)
{
    Assert(Window != 0);

    if (StyleParams.IsTitled())
    {
        if (IsValid())
        {
            SetWindowTextA(Window, Title.CStr());
        }
    }
}

void CWindowsWindow::GetTitle(CString& OutTitle)
{
    if (IsValid())
    {
        int32 Size = GetWindowTextLengthA(Window);
        OutTitle.Resize(Size);

        GetWindowTextA(Window, OutTitle.Data(), Size);
    }
}

void CWindowsWindow::MoveTo(int32 x, int32 y)
{
    RECT BorderRect = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&BorderRect, Style, false, StyleEx);

    x += BorderRect.left;
    y += BorderRect.top;

    SetWindowPos(Window, nullptr, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

void CWindowsWindow::SetWindowShape(const SWindowShape& Shape, bool bMove)
{
    Assert(Window != 0);

    if (IsValid())
    {
        uint32 Flags = SWP_NOZORDER | SWP_NOACTIVATE;
        if (!bMove)
        {
            Flags |= SWP_NOMOVE;
        }

        if (bIsFullscreen)
        {
            // Enables the window to be set to a higher window-size than what the resolution allows
            Flags |= SWP_NOSENDCHANGING;
        }

        // Calculate real window size, since the width and height describe the client- area
        RECT ClientRect =
        {
            0,
            0,
            static_cast<LONG>(Shape.Width),
            static_cast<LONG>(Shape.Height)
        };

#if PLATFORM_WINDOWS_10_ANNIVERSARY
        uint32 WindowDPI = GetDpiForWindow(Window);
        AdjustWindowRectExForDpi(&ClientRect, Style, false, StyleEx, WindowDPI);
#else
        AdjustWindowRectEx(&ClientRect, Style, false, StyleEx);
#endif

        int32 PositionX = Shape.Position.x;
        int32 PositionY = Shape.Position.y;
        int32 RealWidth = ClientRect.right  - ClientRect.left;
        int32 RealHeight = ClientRect.bottom - ClientRect.top;

        SetWindowPos(Window, NULL, PositionX, PositionY, RealWidth, RealHeight, Flags);
    }
}

void CWindowsWindow::GetWindowShape(SWindowShape& OutWindowShape) const
{
    Assert(Window != 0);

    if (IsValid())
    {
        int32  PositionX = 0;
        int32  PositionY = 0;
        uint32 Width = 0;
        uint32 Height = 0;

        RECT Rect = { };
        if (GetWindowRect(Window, &Rect) != 0)
        {
            PositionX = static_cast<int32>(Rect.left);
            PositionY = static_cast<int32>(Rect.top);
        }

        if (GetClientRect(Window, &Rect) != 0)
        {
            Width = static_cast<uint32>(Rect.right - Rect.left);
            Height = static_cast<uint32>(Rect.bottom - Rect.top);
        }

        OutWindowShape = SWindowShape(Width, Height, PositionX, PositionY);
    }
}

void CWindowsWindow::GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const
{
    HMONITOR Monitor = MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);

    SetLastError(0);

    MONITORINFO MonitorInfo;
    MonitorInfo.cbSize = sizeof(MONITORINFO);

    if (GetMonitorInfoA(Monitor, &MonitorInfo))
    {
        OutWidth  = MonitorInfo.rcMonitor.right  - MonitorInfo.rcMonitor.left;
        OutHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;
    }
    else
    {
        CString Error;
        PlatformDebugMisc::GetLastErrorString(Error);

        LOG_ERROR("[CWindowsWindow]: Failed to retrive monitorinfo. Reason: " + Error);
    }
}

uint32 CWindowsWindow::GetWidth() const
{
    if (IsValid())
    {
        RECT Rect = { };
        if (GetClientRect(Window, &Rect) != 0)
        {
            const uint32 Width = static_cast<uint32>(Rect.right - Rect.left);
            return Width;
        }
    }

    return 0;
}

uint32 CWindowsWindow::GetHeight() const
{
    if (IsValid())
    {
        RECT Rect = { };
        if (GetClientRect(Window, &Rect) != 0)
        {
            const uint32 Height = static_cast<uint32>(Rect.bottom - Rect.top);
            return Height;
        }
    }

    return 0;
}

void CWindowsWindow::SetPlatformhandle(PlatformWindowHandle InPlatformHandle)
{
    if (IsWindow(InPlatformHandle))
    {
        Window = InPlatformHandle;

        Style   = GetWindowLong(Window, GWL_STYLE);
        StyleEx = GetWindowLong(Window, GWL_EXSTYLE);

        // Check if the window with high probability is in fullscreen mode
        uint32 FullscreenWidth;
        uint32 FullscreenHeight;
        GetFullscreenInfo(FullscreenWidth, FullscreenHeight);

        SWindowShape WindowShape;
        GetWindowShape(WindowShape);

        const LONG BorderlessStyleMask   = (~WS_BORDER | ~WS_DLGFRAME | ~WS_THICKFRAME);
        const LONG BorderlessStyleExMask = ~WS_EX_WINDOWEDGE;

        const bool bHasFullscreenSize  = (FullscreenWidth == WindowShape.Width) && (FullscreenHeight == WindowShape.Height);
        const bool bHasFullscreenStyle = ((Style & BorderlessStyleMask) == 0) && ((Style & BorderlessStyleExMask) == 0); 
        bIsFullscreen = bHasFullscreenSize && bHasFullscreenStyle; 
    }
    else
    {
        LOG_ERROR("[CWindowsWindow]: Tried to set an invalid WindowHandle")
    }
}

#endif
