#include "WindowsWindow.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Platform/PlatformMisc.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

static FWindowsWindowStyle GetWindowsWindowStyle(FWindowStyle Style)
{
    // Determine the window style for WinAPI
    DWORD NewStyle = 0;
    if (Style.Style != EWindowStyleFlag::None)
    {
        NewStyle = WS_OVERLAPPED;
        if (Style.IsTitled())
        {
            NewStyle |= WS_CAPTION;
        }
        if (Style.IsClosable())
        {
            NewStyle |= WS_SYSMENU;
        }
        if (Style.IsMinimizable())
        {
            NewStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
        }
        if (Style.IsMaximizable())
        {
            NewStyle |= WS_SYSMENU | WS_MAXIMIZEBOX;
        }
        if (Style.IsResizeable())
        {
            NewStyle |= WS_THICKFRAME;
        }
    }
    else
    {
        NewStyle = WS_POPUP;
    }

    DWORD NewStyleEx;
    if (Style.HasTaskBarIcon())
    {
        NewStyleEx = WS_EX_APPWINDOW;
    }
    else
    {
        NewStyleEx = WS_EX_TOOLWINDOW;
    }

    if (Style.IsTopMost())
    {
        NewStyleEx |= WS_EX_TOPMOST;
    }

    return FWindowsWindowStyle(NewStyle, NewStyleEx);
}

const CHAR* FWindowsWindow::GetClassName()
{
    return "WindowClass";
}


FWindowsWindow::FWindowsWindow(FWindowsApplication* InApplication)
    : FGenericWindow()
    , Application(InApplication)
    , Window(0)
    , bIsFullscreen(false)
    , StoredPlacement()
    , Style()
{
}

FWindowsWindow::~FWindowsWindow()
{
    Destroy();
}

bool FWindowsWindow::Initialize(const FString& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, FWindowStyle InStyle, FGenericWindow* InParentWindow)
{
    const FWindowsWindowStyle NewStyle = GetWindowsWindowStyle(InStyle);

    // Calculate real window size, since the width and height describe the client- area
    RECT ClientRect =
    {
        0,
        0,
        static_cast<LONG>(InWidth),
        static_cast<LONG>(InHeight)
    };

#if PLATFORM_WINDOWS_10_ANNIVERSARY
    ::AdjustWindowRectExForDpi(&ClientRect, NewStyle.Style, false, NewStyle.StyleEx, USER_DEFAULT_SCREEN_DPI);
#else
    ::AdjustWindowRectEx(&ClientRect, NewStyle.Style, false, NewStyle.StyleEx);
#endif

    int32 PositionX  = x;
    int32 PositionY  = y;
    int32 RealWidth  = ClientRect.right - ClientRect.left;
    int32 RealHeight = ClientRect.bottom - ClientRect.top;

    HWND ParentWindow = nullptr;
    if (ParentWindow)
    {
        ParentWindow = reinterpret_cast<HWND>(InParentWindow->GetPlatformHandle());
    }

    HINSTANCE Instance = Application->GetInstance();
    Window = ::CreateWindowEx(
        NewStyle.StyleEx,
        FWindowsWindow::GetClassName(),
        InTitle.GetCString(),
        NewStyle.Style,
        PositionX,
        PositionY,
        RealWidth,
        RealHeight,
        ParentWindow,
        nullptr,
        Instance,
        nullptr);
    if (Window == 0)
    {
        LOG_ERROR("[FWindowsWindow]: FAILED to create window\n");
        return false;
    }
    else
    {
        // If the window has a sys-menu we check if the close-button should be active
        if (NewStyle.Style & WS_SYSMENU)
        {
            if (!(InStyle.IsClosable()))
            {
                ::EnableMenuItem(::GetSystemMenu(Window, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            }
        }

        StyleParams = InStyle;
        Style       = NewStyle;

        SetLastError(0);
        LONG_PTR Result = SetWindowLongPtrA(Window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        DWORD LastError = GetLastError();
        if (Result == 0 && LastError != 0)
        {
            LOG_ERROR("[FWindowsWindow]: FAILED to Setup window-data\n");
            return false;
        }

        ::UpdateWindow(Window);

        FWindowShape NewWindowShape(RealWidth, RealHeight, PositionX, PositionY);
        SetWindowShape(NewWindowShape, true);
        return true;
    }
}

void FWindowsWindow::Show(bool bFocusOnActivate)
{
    if (IsValid())
    {
        if (bFocusOnActivate)
        {
            ::ShowWindow(Window, SW_SHOWNORMAL);
        }
        else
        {
            ::ShowWindow(Window, SW_SHOWNA);
        }
    }
}

void FWindowsWindow::Destroy()
{
    if (IsValid())
    {
        ::DestroyWindow(Window);
    }
}

void FWindowsWindow::Minimize()
{
    if (IsValid())
    {
        ::ShowWindow(Window, SW_MINIMIZE);
    }
}

void FWindowsWindow::Maximize()
{
    if (IsValid())
    {
        ::ShowWindow(Window, SW_MAXIMIZE);
    }
}

void FWindowsWindow::Restore()
{
    CHECK(Window != 0);

    if (IsValid())
    {
        bool bResult = ::IsIconic(Window);
        if (bResult)
        {
            ::ShowWindow(Window, SW_RESTORE);
        }
    }
}

void FWindowsWindow::ToggleFullscreen()
{
    CHECK(Window != 0);

    if (IsValid())
    {
        if (!bIsFullscreen)
        {
            bIsFullscreen = true;

            ::GetWindowPlacement(Window, &StoredPlacement);

            if (Style.Style == 0)
            {
                Style.Style = ::GetWindowLong(Window, GWL_STYLE);
            }

            if (Style.StyleEx == 0)
            {
                Style.StyleEx = ::GetWindowLong(Window, GWL_EXSTYLE);
            }

            LONG NewStyle = Style.Style;
            NewStyle &= ~WS_BORDER;
            NewStyle &= ~WS_DLGFRAME;
            NewStyle &= ~WS_THICKFRAME;

            LONG NewStyleEx = Style.StyleEx;
            NewStyleEx &= ~WS_EX_WINDOWEDGE;

            ::SetWindowLong(Window, GWL_STYLE, NewStyle | WS_POPUP);
            ::SetWindowLong(Window, GWL_EXSTYLE, NewStyleEx | WS_EX_TOPMOST);
            
            ::ShowWindow(Window, SW_SHOWMAXIMIZED);
        }
        else
        {
            bIsFullscreen = false;

            ::SetWindowLong(Window, GWL_STYLE, Style.Style);
            ::SetWindowLong(Window, GWL_EXSTYLE, Style.StyleEx);
            
            ::ShowWindow(Window, SW_SHOWNORMAL);

            ::SetWindowPlacement(Window, &StoredPlacement);
        }
    }
}

bool FWindowsWindow::IsValid() const
{
    return ::IsWindow(Window) == TRUE;
}

bool FWindowsWindow::IsMinimized() const
{
    if (IsValid())
    {
        return ::IsIconic(Window) == TRUE;
    }

    return false;
}

bool FWindowsWindow::IsMaximized() const
{
    if (IsValid())
    {
        return ::IsZoomed(Window) == TRUE;
    }

    return false;
}

bool FWindowsWindow::IsChildWindow(const TSharedRef<FGenericWindow>& ChildWindow) const
{
    TSharedRef<FWindowsWindow> WindowsChild = StaticCastSharedRef<FWindowsWindow>(ChildWindow);
    if (WindowsChild)
    {
        return ::IsChild(Window, WindowsChild->GetWindowHandle()) == TRUE;
    }

    return false;
}

void FWindowsWindow::SetWindowFocus()
{
    if (IsValid())
    {
        ::BringWindowToTop(Window);
        ::SetForegroundWindow(Window);
        ::SetFocus(Window);
    }
}

bool FWindowsWindow::IsActiveWindow() const
{
    HWND ForegroundWindow = ::GetForegroundWindow();
    return ForegroundWindow == Window;
}

void FWindowsWindow::SetTitle(const FString& Title)
{
    if (IsValid())
    {
        ::SetWindowTextA(Window, Title.GetCString());
    }
}

void FWindowsWindow::GetTitle(FString& OutTitle) const
{
    if (IsValid())
    {
        const int32 Size = ::GetWindowTextLengthA(Window);
        OutTitle.Resize(Size);
        ::GetWindowTextA(Window, OutTitle.Data(), Size);
    }
}

void FWindowsWindow::SetWindowPos(int32 x, int32 y)
{
    if (IsValid())
    {
        RECT BorderRect = { static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(x), static_cast<LONG>(y) };

#if PLATFORM_WINDOWS_10_ANNIVERSARY
        const uint32 WindowDPI = ::GetDpiForWindow(Window);
        ::AdjustWindowRectExForDpi(&BorderRect, Style.Style, false, Style.StyleEx, WindowDPI);
#else
        ::AdjustWindowRectEx(&BorderRect, Style.Style, false, Style.StyleEx);
#endif

        ::SetWindowPos(Window, nullptr, BorderRect.left, BorderRect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }
}

void FWindowsWindow::SetWindowOpacity(float Alpha)
{
    if (IsValid())
    {
        if (Alpha < 1.0f)
        {
            DWORD CurrentStyle = ::GetWindowLongA(Window, GWL_EXSTYLE) | WS_EX_LAYERED;
            ::SetWindowLongA(Window, GWL_EXSTYLE, CurrentStyle);
            ::SetLayeredWindowAttributes(Window, 0, static_cast<BYTE>(255.0f * Alpha), LWA_ALPHA);
        }
        else
        {
            DWORD CurrentStyle = ::GetWindowLongA(Window, GWL_EXSTYLE) & ~WS_EX_LAYERED;
            ::SetWindowLongA(Window, GWL_EXSTYLE, CurrentStyle);
        }
    }
}

void FWindowsWindow::SetWindowShape(const FWindowShape& Shape, bool bMove)
{
    if (IsValid())
    {
        int32 PositionX  = Shape.Position.x;
        int32 PositionY  = Shape.Position.y;

        uint32 Flags = SWP_NOZORDER | SWP_NOACTIVATE;
        if (!bMove)
        {
            PositionX = 0;
            PositionY = 0;
            Flags |= SWP_NOMOVE;
        }

        if (bIsFullscreen)
        {
            // Enables the window to be set to a higher window-size than what the resolution allows
            Flags |= SWP_NOSENDCHANGING;
        }

        // Calculate real window size, since the width and height describe the client- area
        RECT ClientRect = { 0, 0, static_cast<LONG>(Shape.Width), static_cast<LONG>(Shape.Height) };
#if PLATFORM_WINDOWS_10_ANNIVERSARY
        const uint32 WindowDPI = ::GetDpiForWindow(Window);
        ::AdjustWindowRectExForDpi(&ClientRect, Style.Style, false, Style.StyleEx, WindowDPI);
#else
        ::AdjustWindowRectEx(&ClientRect, Style.Style, false, Style.StyleEx);
#endif

        int32 RealWidth  = ClientRect.right  - ClientRect.left;
        int32 RealHeight = ClientRect.bottom - ClientRect.top;
        ::SetWindowPos(Window, nullptr, PositionX, PositionY, RealWidth, RealHeight, Flags);
    }
}

void FWindowsWindow::GetWindowShape(FWindowShape& OutWindowShape) const
{
    CHECK(Window != 0);

    if (IsValid())
    {
        POINT Position = { 0, 0 };
        ::ClientToScreen(Window, &Position);
        const int32 PositionX = static_cast<int32>(Position.x);
        const int32 PositionY = static_cast<int32>(Position.y);

        RECT Rect = { 0, 0, 0, 0 };
        ::GetClientRect(Window, &Rect);
        const uint32 Width  = static_cast<uint32>(Rect.right - Rect.left);
        const uint32 Height = static_cast<uint32>(Rect.bottom - Rect.top);

        OutWindowShape = FWindowShape(Width, Height, PositionX, PositionY);
    }
}

void FWindowsWindow::GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const
{
    if (IsValid())
    {
        HMONITOR Monitor = ::MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);

        MONITORINFO MonitorInfo;
        MonitorInfo.cbSize = sizeof(MONITORINFO);

        ::GetMonitorInfoA(Monitor, &MonitorInfo);
        OutWidth  = MonitorInfo.rcMonitor.right  - MonitorInfo.rcMonitor.left;
        OutHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;
    }
}

float FWindowsWindow::GetWindowDpiScale() const
{
    if (IsValid())
    {
        HMONITOR Monitor = ::MonitorFromWindow(Window, MONITOR_DEFAULTTONEAREST);

        UINT DpiX = 96;
        UINT DpiY = 96;
        ::GetDpiForMonitor(Monitor, MDT_EFFECTIVE_DPI, &DpiX, &DpiY);

        CHECK(DpiX == DpiY);
        return static_cast<float>(DpiX) / 96.0f;
    }

    return 0.0f;
}

uint32 FWindowsWindow::GetWidth() const
{
    if (IsValid())
    {
        RECT Rect;
        ::GetClientRect(Window, &Rect);
        const uint32 Width = static_cast<uint32>(Rect.right - Rect.left);
        return Width;
    }

    return 0;
}

uint32 FWindowsWindow::GetHeight() const
{
    if (IsValid())
    {
        RECT Rect;
        ::GetClientRect(Window, &Rect);
        const uint32 Height = static_cast<uint32>(Rect.bottom - Rect.top);
        return Height;
    }

    return 0;
}

void FWindowsWindow::SetPlatformHandle(void* InPlatformHandle)
{
    HWND InWindowHandle = reinterpret_cast<HWND>(InPlatformHandle);
    if (IsWindow(InWindowHandle))
    {
        Window = InWindowHandle;

        Style.Style   = ::GetWindowLong(Window, GWL_STYLE);
        Style.StyleEx = ::GetWindowLong(Window, GWL_EXSTYLE);

        // Check if the window with high probability is in fullscreen mode
        uint32 FullscreenWidth;
        uint32 FullscreenHeight;
        GetFullscreenInfo(FullscreenWidth, FullscreenHeight);

        FWindowShape WindowShape;
        GetWindowShape(WindowShape);

        const LONG BorderlessStyleMask   = (~WS_BORDER | ~WS_DLGFRAME | ~WS_THICKFRAME);
        const LONG BorderlessStyleExMask = ~WS_EX_WINDOWEDGE;

        const bool bHasFullscreenSize  = (FullscreenWidth == WindowShape.Width) && (FullscreenHeight == WindowShape.Height);
        const bool bHasFullscreenStyle = ((Style.Style & BorderlessStyleMask) == 0) && ((Style.Style & BorderlessStyleExMask) == 0);
        bIsFullscreen = (bHasFullscreenSize && bHasFullscreenStyle); 
    }
    else
    {
        LOG_ERROR("[FWindowsWindow]: Tried to set an invalid WindowHandle");
    }
}

void FWindowsWindow::SetStyle(FWindowStyle InStyle)
{
    if (IsValid())
    {
        const FWindowsWindowStyle NewStyle = GetWindowsWindowStyle(InStyle);
        if (NewStyle != Style)
        {
            ::SetWindowLong(Window, GWL_STYLE, NewStyle.Style);
            ::SetWindowLong(Window, GWL_EXSTYLE, NewStyle.StyleEx);
            ::ShowWindow(Window, SW_SHOWNA); // This is necessary when we alter the style
            Style = NewStyle;
        }
    }
}
