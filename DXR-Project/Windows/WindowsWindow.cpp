#include "WindowsWindow.h"
#include "WindowsApplication.h"

WindowsWindow::WindowsWindow()
	: hWindow(0)
{
}

WindowsWindow::~WindowsWindow()
{
	if (hWindow)
	{
		::DestroyWindow(hWindow);
	}
}

bool WindowsWindow::Initialize(WindowsApplication* InOwnerApplication, const WindowProperties& Properties)
{
	// Determine the window style for WinAPI
	DWORD dwStyle = 0;
	if (Properties.Style != 0)
	{
		dwStyle = WS_OVERLAPPED;
		if (Properties.Style & WINDOW_STYLE_FLAG_TITLED)
		{
			dwStyle |= WS_CAPTION;
		}
		if (Properties.Style & WINDOW_STYLE_FLAG_CLOSABLE)
		{
			dwStyle |= WS_SYSMENU;
		}
		if (Properties.Style & WINDOW_STYLE_FLAG_MINIMIZABLE)
		{
			dwStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
		}
		if (Properties.Style & WINDOW_STYLE_FLAG_MAXIMIZABLE)
		{
			dwStyle |= WS_SYSMENU | WS_MAXIMIZEBOX;
		}
		if (Properties.Style & WINDOW_STYLE_FLAG_RESIZEABLE)
		{
			dwStyle |= WS_THICKFRAME;
		}
	}
	else
	{
		dwStyle = WS_POPUP;
	}

	// Calculate real window size, since the width and height describe the clientarea
	RECT ClientRect = { 0, 0, static_cast<LONG>(Properties.Width), static_cast<LONG>(Properties.Height) };
	::AdjustWindowRect(&ClientRect, dwStyle, FALSE);

	INT nWidth	= ClientRect.right	- ClientRect.left;
	INT nHeight = ClientRect.bottom - ClientRect.top;

	HINSTANCE hInstance = InOwnerApplication->GetInstance();
	hWindow = ::CreateWindowEx(0, "WinClass", Properties.Title.c_str(), dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, hInstance, NULL);
	if (hWindow == NULL)
	{

		::OutputDebugString("[WindowsWindow]: FAILED to create window\n");
		return false;
	}
	else
	{
		// If the window has a sysmenu we check if the closebutton should be active
		if (dwStyle & WS_SYSMENU)
		{
			if (!(Properties.Style & WINDOW_STYLE_FLAG_CLOSABLE))
			{
				::EnableMenuItem(::GetSystemMenu(hWindow, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			}
		}

		// Save owner
		OwnerApplication = InOwnerApplication;
		
		// Save style for later
		Style = dwStyle;

		::UpdateWindow(hWindow);
		return true;
	}
}

void WindowsWindow::Show()
{
	::ShowWindow(hWindow, SW_NORMAL);
}

void WindowsWindow::GetWindowShape(WindowShape& OutWindowShape)
{
	RECT Rect = { };
	if (::GetWindowRect(hWindow, &Rect) != 0)
	{
		OutWindowShape.X = static_cast<Uint16>(Rect.left);
		OutWindowShape.Y = static_cast<Uint16>(Rect.top);
	}

	if (::GetClientRect(hWindow, &Rect) != 0)
	{
		OutWindowShape.Width	= static_cast<Uint16>(Rect.right  - Rect.left);
		OutWindowShape.Height	= static_cast<Uint16>(Rect.bottom - Rect.top);
	}
}
