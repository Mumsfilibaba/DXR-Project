#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Windows/WindowsWindow.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include <dwmapi.h>

// The size a single caption button occupies at 100% scaling, from Microsoft's title bar design guidance.
// Only reached when DWM cannot answer, which it cannot before the window is first shown.
constexpr float CAPTION_BUTTON_WIDTH_DIPS  = 46.0f;
constexpr float CAPTION_BUTTON_HEIGHT_DIPS = 32.0f;

static FWindowsWindowStyle GetWindowsWindowStyle(EWindowStyleFlags Style)
{
	const EWindowStyleFlags DecorationMask =
		EWindowStyleFlags::Titled |
		EWindowStyleFlags::Closable |
		EWindowStyleFlags::Minimizable |
		EWindowStyleFlags::Maximizable |
		EWindowStyleFlags::Resizable;

	const bool bHasAnyDecoration = (Style & DecorationMask) != EWindowStyleFlags::None;

	DWORD NewStyle = 0;
	if (!bHasAnyDecoration)
	{
		NewStyle = WS_POPUP;
	}
	else
	{
		NewStyle = WS_OVERLAPPED;

        if ((Style & EWindowStyleFlags::Titled) != EWindowStyleFlags::None)
        {
			NewStyle |= WS_CAPTION;
        }

        if ((Style & EWindowStyleFlags::Closable) != EWindowStyleFlags::None)
        {
			NewStyle |= WS_SYSMENU;
        }

        if ((Style & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
        {
			NewStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
        }

        if ((Style & EWindowStyleFlags::Maximizable) != EWindowStyleFlags::None) 
        {
			NewStyle |= WS_SYSMENU | WS_MAXIMIZEBOX;
        }

        if ((Style & EWindowStyleFlags::Resizable) != EWindowStyleFlags::None)
        {
			NewStyle |= WS_THICKFRAME;
        }

        if ((Style & EWindowStyleFlags::CustomTitleBar) != EWindowStyleFlags::None)
        {
			NewStyle |= WS_CAPTION;
        }
	}

	DWORD NewStyleEx = ((Style & EWindowStyleFlags::NoTaskBarIcon) == EWindowStyleFlags::None) ? WS_EX_APPWINDOW : WS_EX_TOOLWINDOW;
    if ((Style & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None)
    {
		NewStyleEx |= WS_EX_TOPMOST;
    }

    if ((Style & EWindowStyleFlags::Opaque) == EWindowStyleFlags::None)
    {
		NewStyleEx |= WS_EX_NOREDIRECTIONBITMAP;
    }

	return FWindowsWindowStyle(NewStyle, NewStyleEx);
}

TSharedRef<FWindowsWindow> FWindowsWindow::Create(FWindowsApplication* InApplication)
{
    TSharedRef<FWindowsWindow> NewWindow = new FWindowsWindow(InApplication);
    return NewWindow;
}

FWindowsWindow::FWindowsWindow(FWindowsApplication* InApplication)
    : FRefCountedBase()
    , Application(InApplication)
    , Window(0)
    , Style()
    , StyleParams(EWindowStyleFlags::None)
    , bIsFullscreen(false)
    , bAcceptsInput(true)
    , StoredPlacement()
{
}

FWindowsWindow::~FWindowsWindow()
{
    Destroy();
}

bool FWindowsWindow::Initialize(const FPlatformWindowDesc& InDesc)
{
    const FWindowsWindowStyle NewStyle = GetWindowsWindowStyle(InDesc.Style);

    RECT ClientRect = { 0, 0, static_cast<LONG>(InDesc.Width), static_cast<LONG>(InDesc.Height) };
#if PLATFORM_WINDOWS_10_ANNIVERSARY
    ::AdjustWindowRectExForDpi(&ClientRect, NewStyle.Style, false, NewStyle.StyleEx, USER_DEFAULT_SCREEN_DPI);
#else
    ::AdjustWindowRectEx(&ClientRect, NewStyle.Style, false, NewStyle.StyleEx);
#endif

    int32 PositionX  = InDesc.Position.X;
    int32 PositionY  = InDesc.Position.Y;
    int32 RealWidth  = ClientRect.right - ClientRect.left;
    int32 RealHeight = ClientRect.bottom - ClientRect.top;

    HWND ParentWindow = nullptr;
    if (InDesc.ParentWindow)
    {
        ParentWindow = reinterpret_cast<HWND>(InDesc.ParentWindow->GetPlatformHandle());
    }

    const CHAR* Title     = *InDesc.Title;
    const CHAR* ClassName = FWindowsWindow::GetClassName();

    HINSTANCE Instance = Application->GetInstance();
    Window = ::CreateWindowExA(NewStyle.StyleEx, ClassName, Title, NewStyle.Style, PositionX, PositionY, RealWidth, RealHeight, ParentWindow, nullptr, Instance, nullptr);

    if (!Window)
    {
        DWORD ErrorCode = ::GetLastError();
        LOG_ERROR("[FWindowsWindow]: FAILED to create window. Error: %lu\n", ErrorCode);
        return false;
    }

    if (NewStyle.Style & WS_SYSMENU)
    {
        if ((InDesc.Style & EWindowStyleFlags::Closable) == EWindowStyleFlags::None)
        {
            ::EnableMenuItem(::GetSystemMenu(Window, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }
    }

#if PLATFORM_WINDOWS_11
    if ((InDesc.Style & EWindowStyleFlags::RoundedCorners) != EWindowStyleFlags::None)
    {
        DWM_WINDOW_CORNER_PREFERENCE CornerPreference = DWMWCP_ROUNDSMALL;
        ::DwmSetWindowAttribute(Window, DWMWA_WINDOW_CORNER_PREFERENCE, &CornerPreference, sizeof(CornerPreference));
    }
#endif

    StyleParams   = InDesc.Style;
    Style         = NewStyle;
    bAcceptsInput = InDesc.bAcceptsInput;

    ::SetLastError(0);

    LONG_PTR Result    = ::SetWindowLongPtrA(Window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    DWORD    LastError = ::GetLastError();

    if (Result == 0 && LastError != 0)
    {
        LOG_ERROR("[FWindowsWindow]: FAILED to setup window-data. Error: %lu\n", LastError);
        ::DestroyWindow(Window);
        Window = nullptr;
        return false;
    }

    if ((InDesc.Style & EWindowStyleFlags::CustomTitleBar) != EWindowStyleFlags::None)
    {
        ::SetWindowPos(Window, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ::UpdateWindow(Window);

    FWindowShape NewWindowShape(InDesc.Width, InDesc.Height, PositionX, PositionY);
    SetWindowShape(NewWindowShape, true);

    return true;
}

void FWindowsWindow::Destroy()
{
    if (IsValid())
    {
        ::DestroyWindow(Window);
        Window = 0;
    }
}

void FWindowsWindow::Show(bool bFocus)
{
	if (!IsValid())
	{
		return;
	}

	if (bFocus)
	{
		::ShowWindow(Window, SW_SHOWNORMAL);
		::BringWindowToTop(Window);
		::SetForegroundWindow(Window);
		::SetFocus(Window);
	}
	else
	{
		::ShowWindow(Window, SW_SHOWNOACTIVATE);
		::SetWindowPos(Window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
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
    CHECK(Window != nullptr);

    if (IsValid())
    {
        if (::IsIconic(Window))
        {
            ::ShowWindow(Window, SW_RESTORE);
        }
    }
}

void FWindowsWindow::ToggleFullscreen()
{
    CHECK(Window != nullptr);

    if (!IsValid())
    {
        return;
    }

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

void FWindowsWindow::SetWindowFocus()
{
    if (IsValid())
    {
        ::BringWindowToTop(Window);
        ::SetForegroundWindow(Window);
        ::SetFocus(Window);
    }
}

bool FWindowsWindow::IsValid() const
{
    return ::IsWindow(Window) == TRUE;
}

bool FWindowsWindow::IsActiveWindow() const
{
    HWND ForegroundWindow = ::GetForegroundWindow();
    return (ForegroundWindow == Window);
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

bool FWindowsWindow::IsChildWindow(const TSharedRef<IPlatformWindow>& ChildWindow) const
{
    TSharedRef<FWindowsWindow> WindowsChild = StaticCastSharedRef<FWindowsWindow>(ChildWindow);
    if (WindowsChild)
    {
        return ::IsChild(Window, WindowsChild->GetWindowHandle()) == TRUE;
    }
    else
    {
        return false;
    }
}

void FWindowsWindow::SetTitle(const String& Title)
{
    if (IsValid())
    {
        ::SetWindowTextA(Window, *Title);
    }
}

void FWindowsWindow::GetTitle(String& OutTitle) const
{
    if (!IsValid())
    {
        OutTitle = "";
        return;
    }

    const int32 Size = ::GetWindowTextLengthA(Window);
    if (Size <= 0)
    {
        OutTitle = "";
        return;
    }

    OutTitle.Resize(Size + 1);
    ::GetWindowTextA(Window, OutTitle.Data(), Size + 1);

    OutTitle[Size] = '\0';
    OutTitle.Resize(Size);
}

void FWindowsWindow::SetWindowPos(int32 x, int32 y)
{
    if (!IsValid())
    {
        return;
    }

    RECT BorderRect = { static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(x), static_cast<LONG>(y) };

#if PLATFORM_WINDOWS_10_ANNIVERSARY
    const uint32 WindowDPI = ::GetDpiForWindow(Window);
    ::AdjustWindowRectExForDpi(&BorderRect, Style.Style, false, Style.StyleEx, WindowDPI);
#else
    ::AdjustWindowRectEx(&BorderRect, Style.Style, false, Style.StyleEx);
#endif

    ::SetWindowPos(Window, nullptr, BorderRect.left, BorderRect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

void FWindowsWindow::SetWindowShape(const FWindowShape& Shape, bool bMove)
{
    if (!IsValid())
    {
        return;
    }

    RECT ClientRect = { 0, 0, static_cast<LONG>(Shape.Width), static_cast<LONG>(Shape.Height) };
#if PLATFORM_WINDOWS_10_ANNIVERSARY
    const uint32 WindowDPI = ::GetDpiForWindow(Window);
    ::AdjustWindowRectExForDpi(&ClientRect, Style.Style, false, Style.StyleEx, WindowDPI);
#else
    ::AdjustWindowRectEx(&ClientRect, Style.Style, false, Style.StyleEx);
#endif

    int32  PositionX = 0;
    int32  PositionY = 0;
    uint32 Flags     = SWP_NOZORDER | SWP_NOACTIVATE;

    if (bMove)
    {
        PositionX = Shape.Position.X + ClientRect.left;
        PositionY = Shape.Position.Y + ClientRect.top;
    }
    else
    {
        Flags |= SWP_NOMOVE;
    }

    if (bIsFullscreen)
    {
        Flags |= SWP_NOSENDCHANGING;
    }

    const int32 RealWidth  = ClientRect.right  - ClientRect.left;
    const int32 RealHeight = ClientRect.bottom - ClientRect.top;

    ::SetWindowPos(Window, nullptr, PositionX, PositionY, RealWidth, RealHeight, Flags);
}

void FWindowsWindow::GetWindowShape(FWindowShape& OutWindowShape) const
{
    if (!IsValid())
    {
        OutWindowShape = FWindowShape(0, 0, 0, 0);
        return;
    }

    POINT Position = { 0, 0 };
    ::ClientToScreen(Window, &Position);
    OutWindowShape.Position.X = static_cast<int32>(Position.x);
    OutWindowShape.Position.Y = static_cast<int32>(Position.y);

    RECT Rect = { 0, 0, 0, 0 };
    ::GetClientRect(Window, &Rect);
    OutWindowShape.Width  = static_cast<uint32>(Rect.right - Rect.left);
    OutWindowShape.Height = static_cast<uint32>(Rect.bottom - Rect.top);
}

uint32 FWindowsWindow::GetWidth() const
{
    if (!IsValid())
    {
        return 0;
    }

    RECT Rect;
    ::GetClientRect(Window, &Rect);
    return static_cast<uint32>(Rect.right - Rect.left);
}

uint32 FWindowsWindow::GetHeight() const
{
    if (!IsValid())
    {
        return 0;
    }

    RECT Rect;
    ::GetClientRect(Window, &Rect);
    return static_cast<uint32>(Rect.bottom - Rect.top);
}

void FWindowsWindow::GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const
{
    if (!IsValid())
    {
        OutWidth = 0;
        OutHeight = 0;
        return;
    }

    HMONITOR Monitor = ::MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO MonitorInfo;
    MonitorInfo.cbSize = sizeof(MONITORINFO);

    ::GetMonitorInfoA(Monitor, &MonitorInfo);
    OutWidth  = MonitorInfo.rcMonitor.right  - MonitorInfo.rcMonitor.left;
    OutHeight = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;
}

float FWindowsWindow::GetWindowDPIScale() const
{
    if (!IsValid())
    {
        return 0.0f;
    }

    HMONITOR Monitor = ::MonitorFromWindow(Window, MONITOR_DEFAULTTONEAREST);

    UINT DpiX = 96;
    UINT DpiY = 96;
    ::GetDpiForMonitor(Monitor, MDT_EFFECTIVE_DPI, &DpiX, &DpiY);

    CHECK(DpiX == DpiY);
    return static_cast<float>(DpiX) / 96.0f;
}

void FWindowsWindow::SetStyle(EWindowStyleFlags InStyle)
{
    if (!IsValid())
    {
        return;
    }

    FWindowShape CurrentShape;
    GetWindowShape(CurrentShape);

    FWindowsWindowStyle NewStyle = GetWindowsWindowStyle(InStyle);
    NewStyle.StyleEx = (NewStyle.StyleEx & ~WS_EX_NOREDIRECTIONBITMAP) | (Style.StyleEx & WS_EX_NOREDIRECTIONBITMAP);

    if ((::GetWindowLongA(Window, GWL_EXSTYLE) & WS_EX_LAYERED) != 0)
    {
        NewStyle.StyleEx |= WS_EX_LAYERED;
    }

    if (NewStyle != Style)
    {
        ::SetWindowLong(Window, GWL_STYLE, NewStyle.Style);
        ::SetWindowLong(Window, GWL_EXSTYLE, NewStyle.StyleEx);

        ::ShowWindow(Window, SW_SHOWNA);

        Style = NewStyle;
    }

    StyleParams = InStyle;

    const bool bTopMost = (InStyle & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None;
    ::SetWindowPos(Window, bTopMost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    SetWindowShape(CurrentShape, true);
}

FWindowTitleBarMetrics FWindowsWindow::GetTitleBarMetrics() const
{
    FWindowTitleBarMetrics Metrics;
    if (!IsValid() || (StyleParams & EWindowStyleFlags::CustomTitleBar) == EWindowStyleFlags::None)
    {
        return Metrics;
    }

    // Asking DWM for the footprint it would have used puts our buttons on exactly the native geometry at any
    // DPI. The window keeps WS_CAPTION and the box styles, so it still answers despite the suppressed frame.

    RECT ButtonBounds = { 0, 0, 0, 0 };
    if (SUCCEEDED(::DwmGetWindowAttribute(Window, DWMWA_CAPTION_BUTTON_BOUNDS, &ButtonBounds, sizeof(ButtonBounds))))
    {
        const float BoundsWidth  = static_cast<float>(ButtonBounds.right - ButtonBounds.left);
        const float BoundsHeight = static_cast<float>(ButtonBounds.bottom - ButtonBounds.top);

        if (BoundsWidth > 0.0f && BoundsHeight > 0.0f)
        {
            Metrics.Height             = BoundsHeight;
            Metrics.TrailingInset      = BoundsWidth;
            Metrics.CaptionButtonWidth = BoundsWidth / 3.0f;
            return Metrics;
        }
    }

    // DWM reports a degenerate rectangle until the window has been shown once, so the documented sizes stand in.
    const float DPIScale = GetWindowDPIScale();

    Metrics.Height             = CAPTION_BUTTON_HEIGHT_DIPS * DPIScale;
    Metrics.CaptionButtonWidth = CAPTION_BUTTON_WIDTH_DIPS * DPIScale;
    Metrics.TrailingInset      = Metrics.CaptionButtonWidth * 3.0f;

    return Metrics;
}

void FWindowsWindow::SetTitleBarRegions(const FWindowTitleBarRegions& InRegions)
{
    SCOPED_LOCK(TitleBarRegionsCS);
    TitleBarRegions = InRegions;
}

bool FWindowsWindow::HitTestTitleBar(const IntVector2& ClientPoint) const
{
    SCOPED_LOCK(TitleBarRegionsCS);

    if (!TitleBarRegions.CaptionRect.Contains(ClientPoint))
    {
        return false;
    }

    for (const FWindowRect& InteractiveRect : TitleBarRegions.InteractiveRects)
    {
        if (InteractiveRect.Contains(ClientPoint))
        {
            return false;
        }
    }

    return true;
}

bool FWindowsWindow::HitTestMaximizeButton(const IntVector2& ClientPoint) const
{
    SCOPED_LOCK(TitleBarRegionsCS);
    return TitleBarRegions.MaximizeButtonRect.Contains(ClientPoint);
}

void FWindowsWindow::SetWindowOpacity(float Alpha)
{
    if (!IsValid())
    {
        return;
    }

    DWORD CurrentStyle = ::GetWindowLongA(Window, GWL_EXSTYLE);

    if (Alpha < 1.0f)
    {
        CurrentStyle |= WS_EX_LAYERED;

        ::SetWindowLongA(Window, GWL_EXSTYLE, CurrentStyle);
        ::SetLayeredWindowAttributes(Window, 0, static_cast<BYTE>(255.0f * Alpha), LWA_ALPHA);
    }
    else
    {
        CurrentStyle &= ~WS_EX_LAYERED;
        ::SetWindowLongA(Window, GWL_EXSTYLE, CurrentStyle);
    }
}

void FWindowsWindow::SetPlatformHandle(void* InPlatformHandle)
{
    HWND InWindowHandle = reinterpret_cast<HWND>(InPlatformHandle);
    if (::IsWindow(InWindowHandle))
    {
        Window = InWindowHandle;

        Style.Style   = ::GetWindowLong(Window, GWL_STYLE);
        Style.StyleEx = ::GetWindowLong(Window, GWL_EXSTYLE);

        uint32 FullscreenWidth = 0;
        uint32 FullscreenHeight = 0;
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
