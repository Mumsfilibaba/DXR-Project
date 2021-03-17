#include "WindowsApplication.h"
#include "WindowsWindow.h"
#include "WindowsCursor.h"

#include "Application/Input.h"

#include "Application/Generic/GenericApplication.h"

WindowsApplication* GlobalWindowsApplication = nullptr;

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

bool WindowsApplication::Init()
{
    WNDCLASS WindowClass;
    Memory::Memzero(&WindowClass);
    
    WindowClass.hInstance     = InstanceHandle;
    WindowClass.lpszClassName = "WinClass";
    WindowClass.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
    WindowClass.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
    WindowClass.lpfnWndProc   = WindowsApplication::MessageProc;

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
    constexpr uint16 SCAN_CODE_MASK   = 0x01ff;
    constexpr uint32 KEY_REPEAT_MASK  = 0x40000000;
    constexpr uint16 BACK_BUTTON_MASK = 0x0001;

    for (const WindowsEvent& Event : Events)
    {
        HWND   Hwnd    = Event.Hwnd;
        uint32 Message = Event.Message;
        WPARAM wParam  = Event.wParam;
        WPARAM lParam  = Event.lParam;

        TRef<WindowsWindow> MessageWindow = MakeSharedRef<WindowsWindow>(GetWindowFromHWND(Hwnd));
        switch (Message)
        {
            case WM_DESTROY:
            {
                if (MessageWindow)
                {
                    EventHandler->OnWindowClosed(MessageWindow);
                }

                ::PostQuitMessage(0);
                break;
            }

            case WM_SETFOCUS:
            case WM_KILLFOCUS:
            {
                if (MessageWindow)
                {
                    const bool HasFocus = (Message == WM_SETFOCUS);
                    EventHandler->OnWindowFocusChanged(MessageWindow, HasFocus);
                }

                break;
            }

            case WM_MOUSELEAVE:
            {
                if (MessageWindow)
                {
                    EventHandler->OnWindowMouseLeft(MessageWindow);
                }

                IsTrackingMouse = false;
                break;
            }

            case WM_SIZE:
            {
                if (MessageWindow)
                {
                    const uint16 Width  = LOWORD(lParam);
                    const uint16 Height = HIWORD(lParam);
                    EventHandler->OnWindowResized(MessageWindow, Width, Height);
                }

                break;
            }

            case WM_MOVE:
            {
                if (MessageWindow)
                {
                    const int16 x = (int16)LOWORD(lParam);
                    const int16 y = (int16)HIWORD(lParam);
                    EventHandler->OnWindowMoved(MessageWindow, x, y);
                }

                break;
            }

            case WM_SYSKEYUP:
            case WM_KEYUP:
            {
                const uint32 ScanCode = static_cast<uint32>(HIWORD(lParam) & SCAN_CODE_MASK);
                const EKey Key        = Input::ConvertFromScanCode(ScanCode);
                EventHandler->OnKeyReleased(Key, GetModifierKeyState());
                break;
            }

            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
                const bool IsRepeat   = !!(lParam & KEY_REPEAT_MASK);
                const uint32 ScanCode = static_cast<uint32>(HIWORD(lParam) & SCAN_CODE_MASK);
                const EKey Key        = Input::ConvertFromScanCode(ScanCode);
                EventHandler->OnKeyPressed(Key, IsRepeat, GetModifierKeyState());
                break;
            }

            case WM_SYSCHAR:
            case WM_CHAR:
            {
                const uint32 Character = static_cast<uint32>(wParam);
                EventHandler->OnCharacterInput(Character);
                break;
            }

            case WM_MOUSEMOVE:
            {
                const int32 x = GET_X_LPARAM(lParam);
                const int32 y = GET_Y_LPARAM(lParam);

                if (!IsTrackingMouse)
                {
                    IsTrackingMouse = true;

                    TRACKMOUSEEVENT TrackEvent;
                    Memory::Memzero(&TrackEvent);

                    TrackEvent.cbSize    = sizeof(TRACKMOUSEEVENT);
                    TrackEvent.dwFlags   = TME_LEAVE;
                    TrackEvent.hwndTrack = Hwnd;
                    TrackMouseEvent(&TrackEvent);

                    EventHandler->OnWindowMouseEntered(MessageWindow);
                }

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
                EMouseButton Button = EMouseButton::MouseButton_Unknown;
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
                EMouseButton Button = EMouseButton::MouseButton_Unknown;
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
                const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
                EventHandler->OnMouseScrolled(0.0f, WheelDelta);
                break;
            }

            case WM_MOUSEHWHEEL:
            {
                const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
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
    TRef<WindowsWindow> Window = DBG_NEW WindowsWindow(this);
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
    TRef<WindowsCursor> Cursor = DBG_NEW WindowsCursor(this);
    if (Cursor)
    {
        return Cursor.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

bool WindowsApplication::PeekMessageUntilNoMessage()
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
    uint32 ModifierMask = 0;
    if (::GetKeyState(VK_CONTROL) & 0x8000)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Ctrl;
    }
    if (::GetKeyState(VK_MENU) & 0x8000)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Alt;
    }
    if (::GetKeyState(VK_SHIFT) & 0x8000)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Shift;
    }
    if (::GetKeyState(VK_CAPITAL) & 1)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_CapsLock;
    }
    if ((::GetKeyState(VK_LWIN) | ::GetKeyState(VK_RWIN)) & 0x8000)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Super;
    }
    if (::GetKeyState(VK_NUMLOCK) & 1)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_NumLock;
    }

    return ModifierKeyState(ModifierMask);
}

WindowsWindow* WindowsApplication::GetWindowFromHWND(HWND Window) const
{
    for (const TRef<WindowsWindow>& CurrentWindow : Windows)
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

void WindowsApplication::GetCursorPos(GenericWindow* RelativeWindow, int32& OutX, int32& OutY) const
{
    TRef<WindowsWindow> WinRelative = MakeSharedRef<WindowsWindow>(RelativeWindow);
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
        TRef<WindowsCursor> WinCursor = MakeSharedRef<WindowsCursor>(Cursor);
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
    TRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(Window);
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
        TRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(CaptureWindow);
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

void WindowsApplication::SetCursorPos(GenericWindow* RelativeWindow, int32 x, int32 y)
{
    if (RelativeWindow)
    {
        TRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(RelativeWindow);
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
        case WM_MOVE:
        case WM_MOUSELEAVE:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
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
