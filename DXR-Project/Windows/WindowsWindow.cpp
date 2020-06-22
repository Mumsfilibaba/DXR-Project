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

bool WindowsWindow::Initialize(WindowsApplication* InOwnerApplication, Uint16 InWidth, Uint16 InHeight)
{
	RECT ClientRect = { 0, 0, static_cast<LONG>(InWidth), static_cast<LONG>(InHeight) };
	::AdjustWindowRect(&ClientRect, dwStyle, FALSE);

	INT nWidth	= ClientRect.right	- ClientRect.left;
	INT nHeight = ClientRect.bottom - ClientRect.top;

	HINSTANCE hInstance = InOwnerApplication->GetInstance();
	hWindow = ::CreateWindowEx(0, "WinClass", "DXR", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, hInstance, NULL);
	if (hWindow == NULL)
	{
		OwnerApplication = InOwnerApplication;

		::OutputDebugString("[WindowsWindow]: Failed to create window\n");
		return false;
	}
	else
	{
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
		OutWindowShape.x = Rect.left;
		OutWindowShape.y = Rect.top;
	}

	if (::GetClientRect(hWindow, &Rect) != 0)
	{
		OutWindowShape.Width	= Rect.right  - Rect.left;
		OutWindowShape.Height	= Rect.bottom - Rect.top;
	}
}
