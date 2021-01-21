#include "WindowsApplication.h"
#include "WindowsWindow.h"
#include "WindowsCursor.h"

#include "Application/Input.h"

#include "Application/Generic/GenericApplication.h"

/*
* Globals
*/

WindowsApplication* GlobalWindowsApplication = nullptr;

/*
* WindowsApplication
*/

GenericApplication* WindowsApplication::Make()
{
	HINSTANCE hInstance = static_cast<HINSTANCE>(::GetModuleHandle(nullptr));

	GlobalWindowsApplication = DBG_NEW WindowsApplication(hInstance);
	return GlobalWindowsApplication;
}

WindowsApplication::WindowsApplication(HINSTANCE InInstanceHandle)
	: InstanceHandle(InInstanceHandle)
	, CurrentCursor()
	, Windows()
{
}

WindowsApplication::~WindowsApplication()
{
	Windows.Clear();
	GlobalWindowsApplication = nullptr;
}

Bool WindowsApplication::Init()
{
	WNDCLASS WindowClass;
	Memory::Memzero(&WindowClass);
	
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

	return true;
}

void WindowsApplication::Tick()
{
	constexpr UInt16 SCAN_CODE_MASK		= 0x01ff;
	constexpr UInt32 KEY_REPEAT_MASK	= 0x40000000;
	constexpr UInt16 BACK_BUTTON_MASK	= 0x0001;

	for (const WindowsEvent& Event : Events)
	{
		HWND	Hwnd	= Event.Hwnd;
		UInt32	Message	= Event.Message;
		WPARAM	wParam	= Event.wParam;
		WPARAM	lParam	= Event.lParam;

		TSharedRef<WindowsWindow> MessageWindow = MakeSharedRef<WindowsWindow>(GetWindowFromHWND(Hwnd));
		switch (Message)
		{
			case WM_DESTROY:
			{
				::PostQuitMessage(0);
				break;
			}

			case WM_SIZE:
			{
				if (MessageWindow)
				{
					const UInt16 Width	= LOWORD(lParam);
					const UInt16 Height	= HIWORD(lParam);

					EventHandler->OnWindowResized(MessageWindow, Width, Height);

					LOG_INFO("Window Resize(" + std::to_string(Width) + ", " + std::to_string(Height) + ")");
				}

				break;
			}

			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				const UInt32 ScanCode	= static_cast<UInt32>(HIWORD(lParam) & SCAN_CODE_MASK);
				const EKey Key			= Input::ConvertFromScanCode(ScanCode);
				EventHandler->OnKeyReleased(Key, GetModifierKeyState());
				break;
			}

			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				const Bool IsRepeat		= !!(lParam & KEY_REPEAT_MASK);
				const UInt32 ScanCode	= static_cast<UInt32>(HIWORD(lParam) & SCAN_CODE_MASK);
				const EKey Key			= Input::ConvertFromScanCode(ScanCode);
				EventHandler->OnKeyPressed(Key, IsRepeat, GetModifierKeyState());
				break;
			}

			case WM_SYSCHAR:
			case WM_CHAR:
			{
				const UInt32 Character = static_cast<UInt32>(wParam);
				EventHandler->OnCharacterInput(Character);
				break;
			}

			case WM_MOUSEMOVE:
			{
				const Int32 x = GET_X_LPARAM(lParam);
				const Int32 y = GET_Y_LPARAM(lParam);

				EventHandler->OnMouseMove(x, y);
				break;
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
				if (Message == WM_LBUTTONDOWN || Message == WM_LBUTTONDBLCLK)
				{
					Button = EMouseButton::MouseButton_Left;
				}
				else if (Message == WM_MBUTTONDOWN || Message == WM_MBUTTONDBLCLK)
				{
					Button = EMouseButton::MouseButton_Middle;
				}
				else if (Message == WM_RBUTTONDOWN || Message == WM_RBUTTONDBLCLK)
				{
					Button = EMouseButton::MouseButton_Right;
				}
				else if (GET_XBUTTON_WPARAM(wParam) == BACK_BUTTON_MASK)
				{
					Button = EMouseButton::MouseButton_Back;
				}
				else
				{
					Button = EMouseButton::MouseButton_Forward;
				}

				EventHandler->OnMouseButtonPressed(Button, GetModifierKeyState());
				break;
			}

			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			case WM_XBUTTONUP:
			{
				EMouseButton Button = EMouseButton::MOUSE_BUTTON_UNKNOWN;
				if (Message == WM_LBUTTONUP)
				{
					Button = EMouseButton::MouseButton_Left;
				}
				else if (Message == WM_MBUTTONUP)
				{
					Button = EMouseButton::MouseButton_Middle;
				}
				else if (Message == WM_RBUTTONUP)
				{
					Button = EMouseButton::MouseButton_Right;
				}
				else if (GET_XBUTTON_WPARAM(wParam) == BACK_BUTTON_MASK)
				{
					Button = EMouseButton::MouseButton_Back;
				}
				else
				{
					Button = EMouseButton::MouseButton_Forward;
				}

				EventHandler->OnMouseButtonReleased(Button, GetModifierKeyState());
				break;
			}

			case WM_MOUSEWHEEL:
			{
				const Float WheelDelta = static_cast<Float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<Float>(WHEEL_DELTA);
				EventHandler->OnMouseScrolled(0.0f, WheelDelta);
				break;
			}

			case WM_MOUSEHWHEEL:
			{
				const Float WheelDelta = static_cast<Float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<Float>(WHEEL_DELTA);
				EventHandler->OnMouseScrolled(WheelDelta, 0.0f);
				break;
			}

			default:
			{
				// Nothing for now
				break;
			}
		}
	}

	Events.Clear();
}

void WindowsApplication::AddWindow(WindowsWindow* Window)
{
	Windows.EmplaceBack(MakeSharedRef<WindowsWindow>(Window));
}

GenericWindow* WindowsApplication::MakeWindow()
{
	TSharedRef<WindowsWindow> Window = DBG_NEW WindowsWindow(this);
	if (Window)
	{
		AddWindow(Window.Get());
		return Window.ReleaseOwnership();
	}
	else
	{
		return nullptr;
	}
}

GenericCursor* WindowsApplication::MakeCursor()
{
	TSharedRef<WindowsCursor> Cursor = DBG_NEW WindowsCursor(this);
	if (Cursor)
	{
		return Cursor.ReleaseOwnership();
	}
	else
	{
		return nullptr;
	}
}

Bool WindowsApplication::FlushSystemEventQueue()
{
	MSG Message;
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

ModifierKeyState WindowsApplication::GetModifierKeyState()
{
	UInt32 ModifierMask = 0;
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

WindowsWindow* WindowsApplication::GetWindowFromHWND(HWND Window) const
{
	for (const TSharedRef<WindowsWindow>& CurrentWindow : Windows)
	{
		if (CurrentWindow->GetHandle() == Window)
		{
			return CurrentWindow.Get();
		}
	}

	return nullptr;
}

GenericWindow* WindowsApplication::GetActiveWindow() const
{
	HWND hActiveWindow = ::GetForegroundWindow();
	return GetWindowFromHWND(hActiveWindow);
}

GenericWindow* WindowsApplication::GetCapture() const
{
	HWND hCapture = ::GetCapture();
	return GetWindowFromHWND(hCapture);
}

GenericCursor* WindowsApplication::GetCursor() const
{
	return CurrentCursor.Get();
}

void WindowsApplication::GetCursorPos(GenericWindow* RelativeWindow, Int32& OutX, Int32& OutY) const
{
	TSharedRef<WindowsWindow> WinRelative = MakeSharedRef<WindowsWindow>(RelativeWindow);
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

void WindowsApplication::SetCursor(GenericCursor* Cursor)
{
	if (Cursor)
	{
		TSharedRef<WindowsCursor> WinCursor = MakeSharedRef<WindowsCursor>(Cursor);
		CurrentCursor = WinCursor;

		HCURSOR hCursor = WinCursor->GetCursor();
		::SetCursor(hCursor);
	}
	else
	{
		::SetCursor(NULL);
	}
}

void WindowsApplication::SetActiveWindow(GenericWindow* Window)
{
	TSharedRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(Window);
	HWND hActiveWindow = WinWindow->GetHandle();
	if (::IsWindow(hActiveWindow))
	{
		::SetActiveWindow(hActiveWindow);
	}
}

void WindowsApplication::SetCapture(GenericWindow* CaptureWindow)
{
	if (CaptureWindow)
	{
		TSharedRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(CaptureWindow);
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

void WindowsApplication::SetCursorPos(GenericWindow* RelativeWindow, Int32 x, Int32 y)
{
	if (RelativeWindow)
	{
		TSharedRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(RelativeWindow);
		HWND hRelative = WinWindow->GetHandle();
	
		POINT CursorPos = { x, y };
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
	switch (uMessage)
	{
		case WM_DESTROY:
		case WM_SIZE:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSCHAR:
		case WM_CHAR:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_XBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		{
			Events.EmplaceBack(hWnd, uMessage, wParam, lParam);
			return 0;
		}
	}

	return ::DefWindowProc(hWnd, uMessage, wParam, lParam);
}

LRESULT WindowsApplication::MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	return GlobalWindowsApplication->ApplicationProc(hWnd, uMessage, wParam, lParam);
}
