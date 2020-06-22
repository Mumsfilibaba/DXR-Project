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

void WindowsApplication::AddWindow(std::shared_ptr<WindowsWindow>& InWindow)
{
	Windows.emplace_back(InWindow);
}

bool WindowsApplication::RegisterWindowClass()
{
	WNDCLASS WindowClass = { };
	WindowClass.hInstance		= hInstance;
	WindowClass.lpszClassName	= "WinClass";
	WindowClass.hbrBackground	= static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
	WindowClass.hCursor			= ::LoadCursor(NULL, IDC_ARROW);
	WindowClass.lpfnWndProc		= WindowsApplication::MessageProc;

	ATOM ClassAtom = ::RegisterClass(&WindowClass);
	if (ClassAtom == 0)
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

std::shared_ptr<WindowsWindow> WindowsApplication::CreateWindow(Uint16 InWidth, Uint16 InHeight)
{
	std::shared_ptr<WindowsWindow> NewWindow = std::shared_ptr<WindowsWindow>(new WindowsWindow());
	if (NewWindow->Initialize(this, InWidth, InHeight))
	{
		AddWindow(NewWindow);
		return NewWindow;
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<WindowsWindow> WindowsApplication::GetWindowFromHWND(HWND hWindow) const
{
	for (const std::shared_ptr<WindowsWindow>& CurrentWindow : Windows)
	{
		if (CurrentWindow->GetHandle() == hWindow)
		{
			return CurrentWindow;
		}
	}

	return nullptr;
}

std::shared_ptr<WindowsWindow> WindowsApplication::GetActiveWindow() const
{
	HWND hActiveWindow = ::GetForegroundWindow();
	return GetWindowFromHWND(hActiveWindow);
}

std::shared_ptr<WindowsWindow> WindowsApplication::GetCapture() const
{
	HWND hCapture = ::GetCapture();
	return GetWindowFromHWND(hCapture);
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

void WindowsApplication::SetActiveWindow(std::shared_ptr<WindowsWindow>& InActiveWindow)
{
	HWND hActiveWindow = InActiveWindow->GetHandle();
	if (::IsWindow(hActiveWindow))
	{
		::SetActiveWindow(hActiveWindow);
	}
}

void WindowsApplication::SetCapture(std::shared_ptr<WindowsWindow>& InCaptureWindow)
{
	HWND hCapture = InCaptureWindow->GetHandle();
	if (::IsWindow(hCapture))
	{
		::SetCapture(hCapture);
	}
}

/*
* MessageProc
*/

LRESULT WindowsApplication::ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	constexpr Uint16 SCAN_CODE_MASK		= 0x01ff;
	constexpr Uint16 BACK_BUTTON_MASK	= 0x0001;

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

				MessageEventHandler->OnWindowResize(MessageWindow, Width, Height);
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

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDOWN:
		{
			EMouseButton Button = EMouseButton::MOUSE_BUTTON_UNKNOWN;
			if (uMessage == WM_LBUTTONDOWN)
			{
				Button = EMouseButton::MOUSE_BUTTON_LEFT;
			}
			else if (uMessage == WM_MBUTTONDOWN)
			{
				Button = EMouseButton::MOUSE_BUTTON_MIDDLE;
			}
			else if (uMessage == WM_RBUTTONDOWN)
			{
				Button = EMouseButton::MOUSE_BUTTON_RIGHT;
			}
			else if (GET_XBUTTON_WPARAM(wParam) == BACK_BUTTON_MASK)
			{
				Button = EMouseButton::MOUSE_BUTTON_BACK;
			}
			else
			{
				Button = EMouseButton::MOUSE_BUTTON_FORWARD;
			}

			MessageEventHandler->OnMouseButtonPressed(Button);
			break;
		}

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_XBUTTONUP:
		{
			EMouseButton Button = EMouseButton::MOUSE_BUTTON_UNKNOWN;
			if (uMessage == WM_LBUTTONUP)
			{
				Button = EMouseButton::MOUSE_BUTTON_LEFT;
			}
			else if (uMessage == WM_MBUTTONUP)
			{
				Button = EMouseButton::MOUSE_BUTTON_MIDDLE;
			}
			else if (uMessage == WM_RBUTTONUP)
			{
				Button = EMouseButton::MOUSE_BUTTON_RIGHT;
			}
			else if (GET_XBUTTON_WPARAM(wParam) == BACK_BUTTON_MASK)
			{
				Button = EMouseButton::MOUSE_BUTTON_BACK;
			}
			else
			{
				Button = EMouseButton::MOUSE_BUTTON_FORWARD;
			}

			MessageEventHandler->OnMouseButtonReleased(Button);
			break;
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
