#include "WindowsWindow.h"
#include "WindowsPlatform.h"

GenericWindow* GenericWindow::Create(const std::string& InTitle, uint32 InWidth, uint32 InHeight, WindowStyle InStyle)
{
    TRef<WindowsWindow> NewWindow = DBG_NEW WindowsWindow();
    if (!NewWindow->Init(InTitle, InWidth, InHeight, InStyle))
    {
        return false;
    }

    return NewWindow.ReleaseOwnership();
}

WindowsWindow::WindowsWindow()
    : GenericWindow()
    , Window(0)
    , Style(0)
    , StyleEx(0)
    , IsFullscreen(false)
{
}

WindowsWindow::~WindowsWindow()
{
    if (IsValid())
    {
        DestroyWindow(Window);
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
    Window = CreateWindowEx(0, WindowClassName, InTitle.c_str(), dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, Instance, NULL);
    if (Window == NULL)
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
                EnableMenuItem(GetSystemMenu(Window, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            }
        }

        // Save style for later
        Style = dwStyle;
        WndStyle = InStyle;

        // Set this to userdata
        SetLastError(0);

        LONG_PTR Result = SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)this);
        DWORD LastError = GetLastError();
        if (Result == 0 && LastError != 0)
        {
            LOG_ERROR("[WindowsWindow]: FAILED to Setup window-data\n");
            return false;
        }

        UpdateWindow(Window);
        return true;
    }
}

void WindowsWindow::Show(bool Maximized)
{
    Assert(Window != 0);

    if (IsValid())
    {
        if (Maximized)
        {
            ShowWindow(Window, SW_NORMAL);
        }
        else
        {
            ShowWindow(Window, SW_SHOWMAXIMIZED);
        }
    }
}

void WindowsWindow::Close()
{
    Assert(Window != 0);

    if (IsValid())
    {
        if (WndStyle.IsClosable())
        {
            CloseWindow(Window);
        }
    }
}

void WindowsWindow::Minimize()
{
    Assert(Window != 0);
    
    if (WndStyle.IsMinimizable())
    {
        if (IsValid())
        {
            ShowWindow(Window, SW_MINIMIZE);
        }
    }
}

void WindowsWindow::Maximize()
{
    if (WndStyle.IsMaximizable())
    {
        if (IsValid())
        {
            ShowWindow(Window, SW_MAXIMIZE);
        }
    }
}

void WindowsWindow::Restore()
{
    Assert(Window != 0);

    if (IsValid())
    {
        bool result = ::IsIconic(Window);
        if (result)
        {
            ::ShowWindow(Window, SW_RESTORE);
        }
    }
}

void WindowsWindow::ToggleFullscreen()
{
    Assert(Window != 0);

    if (IsValid())
    {
        if (!IsFullscreen)
        {
            IsFullscreen = true;

            ::GetWindowPlacement(Window, &StoredPlacement);
            if (Style == 0)
            {
                Style = ::GetWindowLong(Window, GWL_STYLE);
            }
            if (StyleEx == 0)
            {
                StyleEx = ::GetWindowLong(Window, GWL_EXSTYLE);
            }

            LONG newStyle = Style;
            newStyle &= ~WS_BORDER;
            newStyle &= ~WS_DLGFRAME;
            newStyle &= ~WS_THICKFRAME;

            LONG newStyleEx = StyleEx;
            newStyleEx &= ~WS_EX_WINDOWEDGE;

            SetWindowLong(Window, GWL_STYLE, newStyle | WS_POPUP);
            SetWindowLong(Window, GWL_EXSTYLE, newStyleEx | WS_EX_TOPMOST);
            ShowWindow(Window, SW_SHOWMAXIMIZED);
        }
        else
        {
            IsFullscreen = false;

            SetWindowLong(Window, GWL_STYLE, Style);
            SetWindowLong(Window, GWL_EXSTYLE, StyleEx);
            ShowWindow(Window, SW_SHOWNORMAL);
            SetWindowPlacement(Window, &StoredPlacement);
        }
    }
}

bool WindowsWindow::IsValid() const
{
    return IsWindow(Window) == TRUE;
}

bool WindowsWindow::IsActiveWindow() const
{
    HWND hActive = GetForegroundWindow();
    return (hActive == Window);
}

void WindowsWindow::SetTitle(const std::string& Title)
{
    Assert(Window != 0);

    if (WndStyle.IsTitled())
    {
        if (IsValid())
        {
            SetWindowTextA(Window, Title.c_str());
        }
    }
}

void WindowsWindow::GetTitle(std::string& OutTitle)
{
    if (IsValid())
    {
        int32 Size = GetWindowTextLengthA(Window);
        OutTitle.resize(Size);

        GetWindowTextA(Window, OutTitle.data(), Size);
    }
}

void WindowsWindow::SetWindowShape(const WindowShape& Shape, bool Move)
{
    Assert(Window != 0);

    if (IsValid())
    {
        UINT Flags = SWP_NOZORDER | SWP_NOACTIVATE;
        if (!Move)
        {
            Flags |= SWP_NOMOVE;
        }

        SetWindowPos(Window, NULL, Shape.Position.x, Shape.Position.y, Shape.Width, Shape.Height, Flags);
    }
}

void WindowsWindow::GetWindowShape(WindowShape& OutWindowShape) const
{
    Assert(Window != 0);

    if (IsValid())
    {
        int32 x = 0;
        int32 y = 0;
        uint32 Width = 0;
        uint32 Height = 0;

        RECT Rect = { };
        if (GetWindowRect(Window, &Rect) != 0)
        {
            x = static_cast<int32>(Rect.left);
            y = static_cast<int32>(Rect.top);
        }

        if (GetClientRect(Window, &Rect) != 0)
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
        if (GetClientRect(Window, &Rect) != 0)
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
        if (GetClientRect(Window, &Rect) != 0)
        {
            const uint32 Height = static_cast<uint32>(Rect.bottom - Rect.top);
            return Height;
        }
    }

    return 0;
}
