#include "WindowsApplication.h"
#include "WindowsWindow.h"

#include "Application/EventHandler.h"
#include "Application/InputManager.h"

#include <windowsx.h>

/*
* Static
*/

WindowsApplication* GlobalWindowsApplication = nullptr;

WindowsApplication* WindowsApplication::Create(HINSTANCE hInstance)
{
	GlobalWindowsApplication = new WindowsApplication(hInstance);
	if (GlobalWindowsApplication->Create())
	{
		return GlobalWindowsApplication;
	}
	else
	{
		return nullptr;
	}
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

void WindowsApplication::AddWindow(std::shared_ptr<WindowsWindow>& NewWindow)
{
	Windows.emplace_back(NewWindow);
}

bool WindowsApplication::RegisterWindowClass()
{
	WNDCLASS wc = { };
	wc.hInstance		= hInstance;
	wc.lpszClassName	= "WinClass";
	wc.hbrBackground	= static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
	wc.hCursor			= ::LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc		= WindowsApplication::MessageProc;

	ATOM classAtom = ::RegisterClass(&wc);
	if (classAtom == 0)
	{
		::OutputDebugString("[WindowsApplication]: Failed to register WindowClass\n");
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

std::shared_ptr<WindowsWindow> WindowsApplication::CreateWindow(Uint16 Width, Uint16 Height)
{
	std::shared_ptr<WindowsWindow> NewWindow = std::shared_ptr<WindowsWindow>(new WindowsWindow());
	if (NewWindow->Initialize(this, Width, Height))
	{
		AddWindow(NewWindow);
		return NewWindow;
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<WindowsWindow> WindowsApplication::GetWindowFromHWND(HWND hWindow)
{
	for (std::shared_ptr<WindowsWindow>& CurrentWindow : Windows)
	{
		if (CurrentWindow->GetHandle() == hWindow)
		{
			return CurrentWindow;
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

void WindowsApplication::SetEventHandler(std::shared_ptr<EventHandler> NewMessageHandler)
{
	MessageEventHandler = NewMessageHandler;
}

/*
* MessageProc
*/

LRESULT WindowsApplication::ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	constexpr Uint16 SCAN_CODE_MASK = 0x01ff;

	std::shared_ptr<WindowsWindow> MessageWindow = GetWindowFromHWND(hWnd);
	switch (uMessage)
	{
		case WM_DESTROY:
		{
			::PostQuitMessage(0);
			return 0;
		}

		case WM_SIZE:
		{
			if (MessageWindow)
			{
				const Uint16 Width	= LOWORD(lParam);
				const Uint16 Height = HIWORD(lParam);

				MessageEventHandler->OnWindowResize(MessageWindow.get(), Width, Height);
			}

			return 0;
		}

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			const Uint32	ScanCode	= static_cast<Uint32>(HIWORD(lParam) & SCAN_CODE_MASK);
			const EKey		Key			= InputManager::Get().ConvertFromKeyCode(ScanCode);
			MessageEventHandler->OnKeyUp(Key);
			return 0;
		}

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			const Uint32	ScanCode	= static_cast<Uint32>(HIWORD(lParam) & SCAN_CODE_MASK);
			const EKey		Key			= InputManager::Get().ConvertFromKeyCode(ScanCode);
			MessageEventHandler->OnKeyDown(Key);
			return 0;
		}

		case WM_MOUSEMOVE:
		{
			const Int32 x = GET_X_LPARAM(lParam);
			const Int32 y = GET_Y_LPARAM(lParam);

			MessageEventHandler->OnMouseMove(x, y);
			return 0;
		}

		default:
		{
			return ::DefWindowProc(hWnd, uMessage, wParam, lParam);
		}
	}

}

LRESULT WindowsApplication::MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	return GlobalWindowsApplication->ApplicationProc(hWnd, uMessage, wParam, lParam);
}
