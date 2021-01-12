#include "WindowsWindow.h"
#include "WindowsApplication.h"

/*
* WindowsWindow
*/

WindowsWindow::WindowsWindow(WindowsApplication* InOwnerApplication)
	: GenericWindow()
	, OwnerApplication(InOwnerApplication)
	, hWindow(0)
	, Style(0)
	, StyleEx(0)
	, IsFullscreen(false)
{
}

WindowsWindow::~WindowsWindow()
{
	if (hWindow)
	{
		::DestroyWindow(hWindow);
	}
}

Bool WindowsWindow::Init(const WindowCreateInfo& InCreateInfo)
{
	// Determine the window style for WinAPI
	DWORD dwStyle = 0;
	if (InCreateInfo.Style != 0)
	{
		dwStyle = WS_OVERLAPPED;
		if (InCreateInfo.IsTitled())
		{
			dwStyle |= WS_CAPTION;
		}
		if (InCreateInfo.IsClosable())
		{
			dwStyle |= WS_SYSMENU;
		}
		if (InCreateInfo.IsMinimizable())
		{
			dwStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
		}
		if (InCreateInfo.IsMaximizable())
		{
			dwStyle |= WS_SYSMENU | WS_MAXIMIZEBOX;
		}
		if (InCreateInfo.IsResizeable())
		{
			dwStyle |= WS_THICKFRAME;
		}
	}
	else
	{
		dwStyle = WS_POPUP;
	}

	// Calculate real window size, since the width and height describe the clientarea
	RECT ClientRect = { 0, 0, static_cast<LONG>(InCreateInfo.Width), static_cast<LONG>(InCreateInfo.Height) };
	::AdjustWindowRect(&ClientRect, dwStyle, FALSE);

	INT nWidth	= ClientRect.right	- ClientRect.left;
	INT nHeight = ClientRect.bottom - ClientRect.top;

	HINSTANCE hInstance = OwnerApplication->GetInstance();
	hWindow = ::CreateWindowEx(0, "WinClass", InCreateInfo.Title.c_str(), dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, hInstance, NULL);
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
			if (!(InCreateInfo.IsClosable()))
			{
				::EnableMenuItem(::GetSystemMenu(hWindow, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			}
		}

		// Save style for later
		Style = dwStyle;
		CreateInfo = InCreateInfo;

		::UpdateWindow(hWindow);
		return true;
	}
}

void WindowsWindow::Show(Bool Maximized)
{
	VALIDATE(hWindow != 0);

	if (IsValid())
	{
		if (Maximized)
		{
			::ShowWindow(hWindow, SW_NORMAL);
		}
		else
		{
			::ShowWindow(hWindow, SW_SHOWMAXIMIZED);
		}
	}
}

void WindowsWindow::Close()
{
	VALIDATE(hWindow != 0);

	if (IsValid())
	{
		if (CreateInfo.IsClosable())
		{
			::CloseWindow(hWindow);
		}
	}
}

void WindowsWindow::Minimize()
{
	VALIDATE(hWindow != 0);
	
	if (CreateInfo.IsMinimizable())
	{
		if (IsValid())
		{
			::ShowWindow(hWindow, SW_MINIMIZE);
		}
	}
}

void WindowsWindow::Maximize()
{
	if (CreateInfo.IsMaximizable())
	{
		if (IsValid())
		{
			::ShowWindow(hWindow, SW_MAXIMIZE);
		}
	}
}

void WindowsWindow::Restore()
{
	VALIDATE(hWindow != 0);

	if (IsValid())
	{
		Bool result = ::IsIconic(hWindow);
		if (result)
		{
			::ShowWindow(hWindow, SW_RESTORE);
		}
	}
}

void WindowsWindow::ToggleFullscreen()
{
	VALIDATE(hWindow != 0);

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
			::ShowWindow(hWindow, SW_SHOWMAXIMIZED);
		}
		else
		{
			IsFullscreen = false;

			::SetWindowLong(hWindow, GWL_STYLE, Style);
			::SetWindowLong(hWindow, GWL_EXSTYLE, StyleEx);
			::ShowWindow(hWindow, SW_SHOWNORMAL);
			::SetWindowPlacement(hWindow, &StoredPlacement);
		}
	}
}

Bool WindowsWindow::IsValid() const
{
	return ::IsWindow(hWindow);
}

Bool WindowsWindow::IsActiveWindow() const
{
	HWND hActive = ::GetForegroundWindow();
	return (hActive == hWindow);
}

void WindowsWindow::SetTitle(const std::string& Title)
{
	VALIDATE(hWindow != 0);

	if (CreateInfo.IsTitled())
	{
		if (IsValid())
		{
			::SetWindowTextA(hWindow, Title.c_str());
		}
	}
}

void WindowsWindow::SetWindowShape(const WindowShape& Shape, Bool Move)
{
	VALIDATE(hWindow != 0);

	if (IsValid())
	{
		UINT Flags = SWP_NOZORDER | SWP_NOACTIVATE;
		if (!Move)
		{
			Flags |= SWP_NOMOVE;
		}

		::SetWindowPos(hWindow, NULL, Shape.Position.x, Shape.Position.y, Shape.Width, Shape.Height, Flags);
	}
}

void WindowsWindow::GetWindowShape(WindowShape& OutWindowShape) const
{
	VALIDATE(hWindow != 0);

	if (IsValid())
	{
		Int32 x = 0;
		Int32 y = 0;
		UInt32 Width = 0;
		UInt32 Height = 0;

		RECT Rect = { };
		if (::GetWindowRect(hWindow, &Rect) != 0)
		{
			x = static_cast<Int32>(Rect.left);
			y = static_cast<Int32>(Rect.top);
		}

		if (::GetClientRect(hWindow, &Rect) != 0)
		{
			Width = static_cast<UInt32>(Rect.right  - Rect.left);
			Height = static_cast<UInt32>(Rect.bottom - Rect.top);
		}

		OutWindowShape = WindowShape(Width, Height, x, y);
	}
}

UInt32 WindowsWindow::GetWidth() const
{
	if (IsValid())
	{
		RECT Rect = { };
		if (::GetClientRect(hWindow, &Rect) != 0)
		{
			const UInt32 Width = static_cast<UInt32>(Rect.right - Rect.left);
			return Width;
		}
	}

	return 0;
}

UInt32 WindowsWindow::GetHeight() const
{
	if (IsValid())
	{
		RECT Rect = { };
		if (::GetClientRect(hWindow, &Rect) != 0)
		{
			const UInt32 Height = static_cast<UInt32>(Rect.bottom - Rect.top);
			return Height;
		}
	}

	return 0;
}
