#include "WindowsApplication.h"
#include "WindowsWindow.h"
#include "WindowsCursor.h"

#include "Application/Input.h"

#include "Application/Generic/GenericApplication.h"

/*
* Static
*/
WindowsApplication* GlobalWindowsApplication = nullptr;

GenericApplication* WindowsApplication::Make(HINSTANCE hInstance)
{
	GlobalWindowsApplication = new WindowsApplication(hInstance);
	GlobalPlatformApplication = GlobalWindowsApplication;
	return GlobalWindowsApplication;
}

/*
* Members
*/
WindowsApplication::WindowsApplication(HINSTANCE InInstanceHandle)
	: InstanceHandle(InInstanceHandle)
{
}

WindowsApplication::~WindowsApplication()
{
}

bool WindowsApplication::Initialize()
{
	if (!RegisterWindowClass())
	{
		return false;
	}

	return true;
}

void WindowsApplication::AddWindow(TSharedPtr<WindowsWindow>& Window)
{
	Windows.EmplaceBack(Window);
}

bool WindowsApplication::RegisterWindowClass()
{
	WNDCLASS WindowClass = { };
	WindowClass.hInstance		= InstanceHandle;
	WindowClass.lpszClassName	= "WinClass";
	WindowClass.hbrBackground	= static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
	WindowClass.hCursor			= ::LoadCursor(NULL, IDC_ARROW);
	WindowClass.lpfnWndProc		= WindowsApplication::MessageProc;

	ATOM ClassAtom = ::RegisterClass(&WindowClass);
	if (ClassAtom == 0)
	{
		LOG_ERROR("[WindowsApplication]: FAILED to register WindowClass\n");
		return false;
	}
	else
	{
		return true;
	}
}

TSharedPtr<GenericWindow> WindowsApplication::MakeWindow()
{
	TSharedPtr<WindowsWindow> Window = MakeShared<WindowsWindow>(this);
	if (Window)
	{
		AddWindow(Window);
		return Window;
	}
	else
	{
		return TSharedPtr<GenericWindow>();
	}
}

TSharedPtr<GenericCursor> WindowsApplication::MakeCursor()
{
	TSharedPtr<WindowsCursor> Cursor = MakeShared<WindowsCursor>(this);
	if (!Cursor)
	{
		return TSharedPtr<GenericCursor>();
	}
	else
	{
		return Cursor;
	}
}

ModifierKeyState WindowsApplication::GetModifierKeyState() const
{
	Uint32 ModifierMask = 0;
	if (::GetKeyState(VK_CONTROL) & 0x8000)
	{
		ModifierMask |= EModifierFlag::MODIFIER_FLAG_CTRL;
	}
	if (::GetKeyState(VK_MENU) & 0x8000)
	{
		ModifierMask |= EModifierFlag::MODIFIER_FLAG_ALT;
	}
	if (::GetKeyState(VK_SHIFT) & 0x8000)
	{
		ModifierMask |= EModifierFlag::MODIFIER_FLAG_SHIFT;
	}
	if (::GetKeyState(VK_CAPITAL) & 1)
	{
		ModifierMask |= EModifierFlag::MODIFIER_FLAG_CAPS_LOCK;
	}
	if ((::GetKeyState(VK_LWIN) | ::GetKeyState(VK_RWIN)) & 0x8000)
	{
		ModifierMask |= EModifierFlag::MODIFIER_FLAG_SUPER;
	}
	if (::GetKeyState(VK_NUMLOCK) & 1)
	{
		ModifierMask |= EModifierFlag::MODIFIER_FLAG_NUM_LOCK;
	}

	return ModifierKeyState(ModifierMask);
}

TSharedPtr<WindowsWindow> WindowsApplication::GetWindowFromHWND(HWND Window) const
{
	for (const TSharedPtr<WindowsWindow>& CurrentWindow : Windows)
	{
		if (CurrentWindow->GetHandle() == Window)
		{
			return CurrentWindow;
		}
	}

	return TSharedPtr<WindowsWindow>(nullptr);
}

TSharedPtr<GenericWindow> WindowsApplication::GetActiveWindow() const
{
	HWND hActiveWindow = ::GetForegroundWindow();
	return GetWindowFromHWND(hActiveWindow);
}

TSharedPtr<GenericWindow> WindowsApplication::GetCapture() const
{
	HWND hCapture = ::GetCapture();
	return GetWindowFromHWND(hCapture);
}

TSharedPtr<GenericCursor> WindowsApplication::GetCursor() const
{
	return CurrentCursor;
}

void WindowsApplication::GetCursorPos(TSharedPtr<GenericWindow> RelativeWindow, Int32& OutX, Int32& OutY) const
{
	TSharedPtr<WindowsWindow> WinRelative = StaticCast<WindowsWindow>(RelativeWindow);
	HWND hRelative = WinRelative->GetHandle();

	POINT CursorPos = { };
	if (::GetCursorPos(&CursorPos))
	{
		if (::ScreenToClient(hRelative, &CursorPos))
		{
			OutX = CursorPos.x;
			OutY = CursorPos.y;
		}
	}
}

bool WindowsApplication::Tick()
{
	MSG Message = { };
	while (::PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&Message);
		::DispatchMessage(&Message);

		if (Message.message == WM_QUIT)
		{
			return false;
		}
	}

	return true;
}

void WindowsApplication::SetCursor(TSharedPtr<GenericCursor> Cursor)
{
	if (Cursor)
	{
		TSharedPtr<WindowsCursor> WinCursor = StaticCast<WindowsCursor>(Cursor);
		CurrentCursor = WinCursor;

		HCURSOR hCursor = WinCursor->GetCursor();
		::SetCursor(hCursor);
	}
	else
	{
		::SetCursor(NULL);
	}
}

void WindowsApplication::SetActiveWindow(TSharedPtr<GenericWindow> Window)
{
	TSharedPtr<WindowsWindow> WinWindow = StaticCast<WindowsWindow>(Window);
	HWND hActiveWindow = WinWindow->GetHandle();
	if (::IsWindow(hActiveWindow))
	{
		::SetActiveWindow(hActiveWindow);
	}
}

void WindowsApplication::SetCapture(TSharedPtr<GenericWindow> CaptureWindow)
{
	if (CaptureWindow)
	{
		TSharedPtr<WindowsWindow> WinWindow = StaticCast<WindowsWindow>(CaptureWindow);
		HWND hCapture = WinWindow->GetHandle();
		if (::IsWindow(hCapture))
		{
			::SetCapture(hCapture);
		}
	}
	else
	{
		::ReleaseCapture();
	}
}

void WindowsApplication::SetCursorPos(TSharedPtr<GenericWindow> RelativeWindow, Int32 X, Int32 Y)
{
	if (RelativeWindow)
	{
		TSharedPtr<WindowsWindow> WinWindow = StaticCast<WindowsWindow>(RelativeWindow);
		HWND hRelative = WinWindow->GetHandle();
	
		POINT CursorPos = { X, Y };
		if (::ClientToScreen(hRelative, &CursorPos))
		{
			::SetCursorPos(CursorPos.x, CursorPos.y);
		}
	}
}

/*
* MessageProc
*/

LRESULT WindowsApplication::ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	constexpr Uint16 SCAN_CODE_MASK		= 0x01ff;
	constexpr Uint16 BACK_BUTTON_MASK	= 0x0001;

	TSharedPtr<WindowsWindow> MessageWindow = GetWindowFromHWND(hWnd);
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

				EventHandler->OnWindowResized(MessageWindow, Width, Height);
			}

			return 0;
		}

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			const Uint32	ScanCode	= static_cast<Uint32>(HIWORD(lParam) & SCAN_CODE_MASK);
			const EKey		Key			= Input::ConvertFromScanCode(ScanCode);
			EventHandler->OnKeyReleased(Key, GetModifierKeyState());
			return 0;
		}

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			const Uint32	ScanCode	= static_cast<Uint32>(HIWORD(lParam) & SCAN_CODE_MASK);
			const EKey		Key			= Input::ConvertFromScanCode(ScanCode);
			EventHandler->OnKeyPressed(Key, GetModifierKeyState());
			return 0;
		}

		case WM_SYSCHAR:
		case WM_CHAR:
		{
			const Uint32 Character = static_cast<Uint32>(wParam);
			EventHandler->OnCharacterInput(Character);
			return 0;
		}

		case WM_MOUSEMOVE:
		{
			const Int32 x = GET_X_LPARAM(lParam);
			const Int32 y = GET_Y_LPARAM(lParam);

			EventHandler->OnMouseMove(x, y);
			return 0;
		}

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		{
			EMouseButton Button = EMouseButton::MOUSE_BUTTON_UNKNOWN;
			if (uMessage == WM_LBUTTONDOWN || uMessage == WM_LBUTTONDBLCLK)
			{
				Button = EMouseButton::MOUSE_BUTTON_LEFT;
			}
			else if (uMessage == WM_MBUTTONDOWN || uMessage == WM_MBUTTONDBLCLK)
			{
				Button = EMouseButton::MOUSE_BUTTON_MIDDLE;
			}
			else if (uMessage == WM_RBUTTONDOWN || uMessage == WM_RBUTTONDBLCLK)
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

			EventHandler->OnMouseButtonPressed(Button, GetModifierKeyState());
			return 0;
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

			EventHandler->OnMouseButtonReleased(Button, GetModifierKeyState());
			return 0;
		}

		case WM_MOUSEWHEEL:
		{
			const Float32 WheelDelta = static_cast<Float32>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<Float32>(WHEEL_DELTA);
			EventHandler->OnMouseScrolled(0.0f, WheelDelta);
			return 0;
		}

		case WM_MOUSEHWHEEL:
		{
			const Float32 WheelDelta = static_cast<Float32>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<Float32>(WHEEL_DELTA);
			EventHandler->OnMouseScrolled(WheelDelta, 0.0f);
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
