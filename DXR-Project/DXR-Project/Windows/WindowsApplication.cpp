#include "WindowsApplication.h"
#include "WindowsWindow.h"

std::unique_ptr<WindowsApplication> WindowsApplication::WinApplication = nullptr;

/*
* Static
*/

WindowsApplication* WindowsApplication::Create(HINSTANCE hInstance)
{
	WinApplication.reset(new WindowsApplication(hInstance));
	if (WinApplication->Create())
	{
		return WinApplication.get();
	}
	else
	{
		return nullptr;
	}
}

WindowsApplication* WindowsApplication::Get()
{
	return WinApplication.get();
}

/*
* Members
*/

WindowsApplication::WindowsApplication(HINSTANCE hInstance)
	: hInstance(hInstance)
{
}

bool WindowsApplication::Create()
{
	if (!RegisterWindowClass())
	{
		return false;
	}

	return true;
}

void WindowsApplication::AddWindow(WindowsWindow* NewWindow)
{
	Windows.emplace_back(NewWindow);
}

bool WindowsApplication::RegisterWindowClass()
{
	WNDCLASS wc = { };
	wc.hInstance		= hInstance;
	wc.lpszClassName	= "WinClass";
	wc.hbrBackground	= static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc		= WindowsApplication::MessageProc;

	ATOM classAtom = ::RegisterClass(&wc);
	if (classAtom == 0)
	{
		OutputDebugString("[WindowsApplication]: Failed to register WindowProc");
		return false;
	}
	else
	{
		return true;
	}
}

WindowsApplication::~WindowsApplication()
{
}

WindowsWindow* WindowsApplication::CreateWindow(Uint16 Width, Uint16 Height)
{
	WindowsWindow* NewWindow = new WindowsWindow();
	if (NewWindow->Create(Width, Height))
	{
		AddWindow(NewWindow);
		return NewWindow;
	}
	else
	{
		return nullptr;
	}
}

WindowsWindow* WindowsApplication::GetWindowFromHWND(HWND hWindow)
{
	for (WindowsWindow* Window : Windows)
	{
		if (Window->GetHandle() == hWindow)
		{
			return Window;
		}
	}

	return nullptr;
}

bool WindowsApplication::Tick()
{
	MSG msg = { };
	while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
		{
			return false;
		}
	}

	return true;
}

/*
* MessageProc
*/

LRESULT WindowsApplication::ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_DESTROY: 
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

LRESULT WindowsApplication::MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	return WinApplication->ApplicationProc(hWnd, uMessage, wParam, lParam);
}
