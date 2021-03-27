#include "WindowsWindow.h"
#include "WindowsPlatform.h"

Window* Window::Create(const std::string& InTitle, uint32 InWidth, uint32 InHeight, WindowStyle InStyle)
{
    TRef<WindowsWindow> NewWindow = DBG_NEW WindowsWindow();
    if (!NewWindow->Init(InTitle, InWidth, InHeight, InStyle))
    {
        return false;
    }

    return NewWindow.ReleaseOwnership();
}

WindowsWindow::WindowsWindow()
    : Window()
    , hWindow(0)
    , Style(0)
    , StyleEx(0)
    , IsFullscreen(false)
{
}

WindowsWindow::~WindowsWindow()
{
    if (IsValid())
    {
        DestroyWindow(hWindow);
    }
}

bool WindowsWindow::Init(const std::string& InTitle, uint32 InWidth, uint32 InHeight, WindowStyle InStyle)
{
    // Determine the window style for WinAPI
    DWORD dwStyle = 0;
    if (InStyle.Style != 0)
    {
        dwStyle = WS_OVERLAPPED;
        if (InStyle.IsTitled())
        {
            dwStyle |= WS_CAPTION;
        }
        if (InStyle.IsClosable())
        {
            dwStyle |= WS_SYSMENU;
        }
        if (InStyle.IsMinimizable())
        {
            dwStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
        }
        if (InStyle.IsMaximizable())
        {
            dwStyle |= WS_SYSMENU | WS_MAXIMIZEBOX;
        }
        if (InStyle.IsResizeable())
        {
            dwStyle |= WS_THICKFRAME;
        }
    }
    else
    {
        dwStyle = WS_POPUP;
    }

    // Calculate real window size, since the width and height describe the clientarea
    RECT ClientRect = { 0, 0, static_cast<LONG>(InWidth), static_cast<LONG>(InHeight) };
    AdjustWindowRect(&ClientRect, dwStyle, FALSE);

    INT nWidth	= ClientRect.right	- ClientRect.left;
    INT nHeight = ClientRect.bottom - ClientRect.top;

    HINSTANCE Instance     = WindowsPlatform::GetInstance();
    LPCSTR WindowClassName = WindowsPlatform::GetWindowClassName();
    hWindow = CreateWindowEx(0, WindowClassName, InTitle.c_str(), dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, Instance, NULL);
    if (hWindow == NULL)
    {
        LOG_ERROR("[WindowsWindow]: FAILED to create window\n");
        return false;
    }
    else
    {
        // If the window has a sysmenu we check if the closebutton should be active
        if (dwStyle & WS_SYSMENU)
        {
            if (!(InStyle.IsClosable()))
            {
                EnableMenuItem(GetSystemMenu(hWindow, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            }
        }

        // Save style for later
        Style = dwStyle;
        WndStyle = InStyle;

        // Set this to userdata
        SetLastError(0);
        LONG_PTR Result = SetWindowLongPtrA(hWindow, GWLP_USERDATA, (LONG_PTR)this);
        DWORD LastError = GetLastError();
        if (Result == 0 && LastError != 0)
        {
            LOG_ERROR("[WindowsWindow]: FAILED to Setup window-data\n");
            return false;
        }

        UpdateWindow(hWindow);
        return true;
    }
}

void WindowsWindow::Show(bool Maximized)
{
    Assert(hWindow != 0);

    if (IsValid())
    {
        if (Maximized)
        {
            ShowWindow(hWindow, SW_NORMAL);
        }
        else
        {
            ShowWindow(hWindow, SW_SHOWMAXIMIZED);
        }
    }
}

void WindowsWindow::Close()
{
    Assert(hWindow != 0);

    if (IsValid())
    {
        if (WndStyle.IsClosable())
        {
            CloseWindow(hWindow);
        }
    }
}

void WindowsWindow::Minimize()
{
    Assert(hWindow != 0);
    
    if (WndStyle.IsMinimizable())
    {
        if (IsValid())
        {
            ShowWindow(hWindow, SW_MINIMIZE);
        }
    }
}

void WindowsWindow::Maximize()
{
    if (WndStyle.IsMaximizable())
    {
        if (IsValid())
        {
            ShowWindow(hWindow, SW_MAXIMIZE);
        }
    }
}

void WindowsWindow::Restore()
{
    Assert(hWindow != 0);

    if (IsValid())
    {
        bool result = ::IsIconic(hWindow);
        if (result)
        {
            ::ShowWindow(hWindow, SW_RESTORE);
        }
    }
}

void WindowsWindow::ToggleFullscreen()
{
    Assert(hWindow != 0);

    if (IsValid())
    {
        if (!IsFullscreen)
        {
            IsFullscreen = true;

            ::GetWindowPlacement(hWindow, &StoredPlacement);
            if (Style == 0)
            {
                Style = ::GetWindowLong(hWindow, GWL_STYLE);
            }
            if (StyleEx == 0)
            {
                StyleEx = ::GetWindowLong(hWindow, GWL_EXSTYLE);
            }

            LONG newStyle = Style;
            newStyle &= ~WS_BORDER;
            newStyle &= ~WS_DLGFRAME;
            newStyle &= ~WS_THICKFRAME;

            LONG newStyleEx = StyleEx;
            newStyleEx &= ~WS_EX_WINDOWEDGE;

            SetWindowLong(hWindow, GWL_STYLE, newStyle | WS_POPUP);
            SetWindowLong(hWindow, GWL_EXSTYLE, newStyleEx | WS_EX_TOPMOST);
            ShowWindow(hWindow, SW_SHOWMAXIMIZED);
        }
        else
        {
            IsFullscreen = false;

            SetWindowLong(hWindow, GWL_STYLE, Style);
            SetWindowLong(hWindow, GWL_EXSTYLE, StyleEx);
            ShowWindow(hWindow, SW_SHOWNORMAL);
            SetWindowPlacement(hWindow, &StoredPlacement);
        }
    }
}

bool WindowsWindow::IsValid() const
{
    return IsWindow(hWindow) == TRUE;
}

bool WindowsWindow::IsActiveWindow() const
{
    HWND hActive = GetForegroundWindow();
    return (hActive == hWindow);
}

void WindowsWindow::SetTitle(const std::string& Title)
{
    Assert(hWindow != 0);

    if (WndStyle.IsTitled())
    {
        if (IsValid())
        {
            SetWindowTextA(hWindow, Title.c_str());
        }
    }
}

void WindowsWindow::GetTitle(std::string& OutTitle)
{
    if (IsValid())
    {
        int32 Size = GetWindowTextLengthA(hWindow);
        OutTitle.resize(Size);

        GetWindowTextA(hWindow, OutTitle.data(), Size);
    }
}

void WindowsWindow::SetWindowShape(const WindowShape& Shape, bool Move)
{
    Assert(hWindow != 0);

    if (IsValid())
    {
        UINT Flags = SWP_NOZORDER | SWP_NOACTIVATE;
        if (!Move)
        {
            Flags |= SWP_NOMOVE;
        }

        SetWindowPos(hWindow, NULL, Shape.Position.x, Shape.Position.y, Shape.Width, Shape.Height, Flags);
    }
}

void WindowsWindow::GetWindowShape(WindowShape& OutWindowShape) const
{
    Assert(hWindow != 0);

    if (IsValid())
    {
        int32 x = 0;
        int32 y = 0;
        uint32 Width = 0;
        uint32 Height = 0;

        RECT Rect = { };
        if (GetWindowRect(hWindow, &Rect) != 0)
        {
            x = static_cast<int32>(Rect.left);
            y = static_cast<int32>(Rect.top);
        }

        if (GetClientRect(hWindow, &Rect) != 0)
        {
            Width  = static_cast<uint32>(Rect.right  - Rect.left);
            Height = static_cast<uint32>(Rect.bottom - Rect.top);
        }

        OutWindowShape = WindowShape(Width, Height, x, y);
    }
}

uint32 WindowsWindow::GetWidth() const
{
    if (IsValid())
    {
        RECT Rect = { };
        if (GetClientRect(hWindow, &Rect) != 0)
        {
            const uint32 Width = static_cast<uint32>(Rect.right - Rect.left);
            return Width;
        }
    }

    return 0;
}

uint32 WindowsWindow::GetHeight() const
{
    if (IsValid())
    {
        RECT Rect = { };
        if (GetClientRect(hWindow, &Rect) != 0)
        {
            const uint32 Height = static_cast<uint32>(Rect.bottom - Rect.top);
            return Height;
        }
    }

    return 0;
}
