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

bool WindowsWindow::Init(Uint16 Width, Uint16 Height)
{
	DWORD	dwStyle		= WS_OVERLAPPEDWINDOW;
	RECT	clientRect	= { 0, 0, LONG(Width), LONG(Height) };
	::AdjustWindowRect(&clientRect, dwStyle, FALSE);

	INT nWidth	= clientRect.right - clientRect.left;
	INT nHeight = clientRect.bottom - clientRect.top;

	HINSTANCE hInstance = WindowsApplication::Get()->GetInstance();
	hWindow = ::CreateWindowEx(0, "WinClass", "DXR", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, nWidth, nHeight, NULL, NULL, hInstance, NULL);
	if (hWindow == NULL)
	{
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