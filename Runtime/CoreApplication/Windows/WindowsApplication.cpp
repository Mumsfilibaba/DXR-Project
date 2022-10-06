#include "WindowsApplication.h"

#include "Core/Threading/ScopedLock.h"
#include "Core/Input/Windows/WindowsKeyMapping.h"
#include "Core/Misc/OutputDeviceLogger.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EWindowsMasks

enum EWindowsMasks : uint32
{
    ScanCodeMask   = 0x01ff,
    KeyRepeatMask  = 0x40000000,
    BackButtonMask = 0x0001
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FPointMessage

struct FPointMessage
{
    FORCEINLINE FPointMessage(LPARAM InParam)
        : Param(InParam)
    { }

    union
    {
         /** @brief: Used for resize messages */
        struct
        {
            uint16 Width;
            uint16 Height;
        };

         /** @brief: Used for move messages */
        struct
        {
            int16 x;
            int16 y;
        };

        LPARAM Param;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsApplication

FWindowsApplication* WindowsApplication = nullptr;

FWindowsApplication* FWindowsApplication::CreateWindowsApplication()
{
    HINSTANCE TempInstanceHandle = static_cast<HINSTANCE>(GetModuleHandleA(0));

    // TODO: Load icon here
    WindowsApplication = dbg_new FWindowsApplication(TempInstanceHandle);
    return WindowsApplication;
}

FWindowsApplication::FWindowsApplication(HINSTANCE InInstanceHandle)
    : FGenericApplication(TSharedPtr<ICursor>(dbg_new FWindowsCursor()))
    , Windows()
    , Messages()
    , MessagesCS()
    , WindowsMessageListeners()
    , bIsTrackingMouse(false)
    , InstanceHandle(InInstanceHandle)
{
    const bool bResult = RegisterWindowClass();
    CHECK(bResult == true);

#if (PLATFORM_WINDOWS_VISTA && ENABLE_DPI_AWARENESS)
    SetProcessDPIAware();
#endif

    FWindowsKeyMapping::Initialize();
 }

FWindowsApplication::~FWindowsApplication()
{
    Windows.Clear();

    if (WindowsApplication == this)
    {
        WindowsApplication = nullptr;
    }
}

bool FWindowsApplication::RegisterWindowClass()
{
    WNDCLASS WindowClass;
    FMemory::Memzero(&WindowClass);

    WindowClass.hInstance     = InstanceHandle;
    WindowClass.lpszClassName = FWindowsWindow::GetClassName();
    WindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    WindowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    WindowClass.lpfnWndProc   = FWindowsApplication::StaticMessageProc;

    ATOM ClassAtom = RegisterClass(&WindowClass);
    if (ClassAtom == 0)
    {
        LOG_ERROR("[FWindowsApplication]: FAILED to register WindowClass\n");
        return false;
    }

    return true;
}

bool FWindowsApplication::RegisterRawInputDevices(HWND Window)
{
    constexpr uint32 DeviceCount = 1;

    RAWINPUTDEVICE Devices[DeviceCount];
    FMemory::Memzero(Devices, DeviceCount);

    // Mouse
    Devices[0].dwFlags     = 0;
    Devices[0].hwndTarget  = Window;
    Devices[0].usUsage     = 0x02;
    Devices[0].usUsagePage = 0x01;

    const auto bResult = ::RegisterRawInputDevices(Devices, DeviceCount, sizeof(RAWINPUTDEVICE));
    if (!bResult)
    {
        LOG_ERROR("[FWindowsApplication] Failed to register Raw Input devices");
        return false;
    }
    else
    {
        LOG_INFO("[FWindowsApplication] Registered Raw Input devices");
        return true;
    }
}

bool FWindowsApplication::UnregisterRawInputDevices()
{
    constexpr uint32 DeviceCount = 1;

    RAWINPUTDEVICE Devices[DeviceCount];
    FMemory::Memzero(Devices, DeviceCount);

    // Mouse
    Devices[0].dwFlags     = RIDEV_REMOVE;
    Devices[0].hwndTarget  = 0;
    Devices[0].usUsage     = 0x02;
    Devices[0].usUsagePage = 0x01;

    const auto bResult = ::RegisterRawInputDevices(Devices, DeviceCount, sizeof(RAWINPUTDEVICE));
    if (!bResult)
    {
        LOG_ERROR("[FWindowsApplication] Failed to unregister Raw Input devices");
        return false;
    }
    else
    {
        LOG_INFO("[FWindowsApplication] Unregistered Raw Input devices");
        return true;
    }
}

FGenericWindowRef FWindowsApplication::CreateWindow()
{
    FWindowsWindowRef NewWindow = dbg_new FWindowsWindow(this);
 
    {
        TScopedLock Lock(WindowsCS);
        Windows.Emplace(NewWindow);
    }

    return NewWindow;
}

void FWindowsApplication::Tick(float)
{
    // Start by pumping all the messages 
    FPlatformApplicationMisc::PumpMessages(true);

    // TODO: Store the second TArray to save on allocations
    TArray<FWindowsMessage> ProcessableMessages;
    if (!Messages.IsEmpty())
    {
        TScopedLock<FCriticalSection> Lock(MessagesCS);
        ProcessableMessages.Append(Messages);

        Messages.Clear();
    }

    // Handle all the messages 
    for (const FWindowsMessage& Message : ProcessableMessages)
    {
        HandleStoredMessage(Message.Window, Message.Message, Message.wParam, Message.lParam, Message.MouseDeltaX, Message.MouseDeltaY);
    }

    if (!ClosedWindows.IsEmpty())
    {
        TScopedLock Lock(ClosedWindowsCS);

        for (const FWindowsWindowRef& Window : ClosedWindows)
        {
            HWND WindowHandle = Window->GetWindowHandle();
            
            MessageListener->HandleWindowClosed(Window);

            DestroyWindow(WindowHandle);
        }

        ClosedWindows.Clear();
    }
}

bool FWindowsApplication::EnableHighPrecisionMouseForWindow(const FGenericWindowRef& Window)
{
    TSharedRef<FWindowsWindow> WindowsWindow = StaticCastSharedRef<FWindowsWindow>(Window);
    if (WindowsWindow && WindowsWindow->IsValid())
    {
        return RegisterRawInputDevices(WindowsWindow->GetWindowHandle());
    }
    else
    {
        return false;
    }
}

void FWindowsApplication::SetCapture(const FGenericWindowRef& Window)
{
    TSharedRef<FWindowsWindow> WindowsWindow = StaticCastSharedRef<FWindowsWindow>(Window);
    if (WindowsWindow && WindowsWindow->IsValid())
    {
        HWND hCapture = WindowsWindow->GetWindowHandle();
        ::SetCapture(hCapture);
    }
    else
    {
        ReleaseCapture();
    }
}

void FWindowsApplication::SetActiveWindow(const FGenericWindowRef& Window)
{
    FWindowsWindowRef WindowsWindow = StaticCastSharedRef<FWindowsWindow>(Window);
    if (WindowsWindow && WindowsWindow->IsValid())
    {
        HWND hActiveWindow = WindowsWindow->GetWindowHandle();
        ::SetActiveWindow(hActiveWindow);
    }
}

FGenericWindowRef FWindowsApplication::GetCapture() const
{
    // TODO: Should we add a reference here
    HWND CaptureWindow = ::GetCapture();
    return GetWindowsWindowFromHWND(CaptureWindow);
}

FGenericWindowRef FWindowsApplication::GetActiveWindow() const
{
    // TODO: Should we add a reference here
    HWND ActiveWindow = ::GetActiveWindow();
    return GetWindowsWindowFromHWND(ActiveWindow);
}

FGenericWindowRef FWindowsApplication::GetWindowUnderCursor() const
{
    POINT CursorPos;
    if (!GetCursorPos(&CursorPos))
    {
        LOG_ERROR("Failed to retrieve the Cursor position");
        return nullptr;
    }

    HWND Handle = WindowFromPoint(CursorPos);
    return GetWindowsWindowFromHWND(Handle);
}

FWindowsWindowRef FWindowsApplication::GetWindowsWindowFromHWND(HWND InWindow) const
{
    if (IsWindow(InWindow))
    {
        TScopedLock Lock(WindowsCS);

        for (const FWindowsWindowRef& Window : Windows)
        {
            if (Window->GetWindowHandle() == InWindow)
            {
                return Window;
            }
        }
    }

    return nullptr;
}

void FWindowsApplication::AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener)
{
    TScopedLock Lock(WindowsMessageListenersCS);

    // We do not want to add a listener if it already exists
    if (!WindowsMessageListeners.Contains(NewWindowsMessageListener))
    {
        WindowsMessageListeners.Emplace(NewWindowsMessageListener);
    }
    else
    {
        LOG_WARNING("Tried to add a listener that already exists");
    }
}

void FWindowsApplication::RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& InWindowsMessageListener)
{
    TScopedLock Lock(WindowsMessageListenersCS);
    WindowsMessageListeners.Remove(InWindowsMessageListener);
}

bool FWindowsApplication::IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& InWindowsMessageListener) const
{
    TScopedLock Lock(WindowsMessageListenersCS);
    return WindowsMessageListeners.Contains(InWindowsMessageListener);
}

void FWindowsApplication::CloseWindow(const FWindowsWindowRef& Window)
{
    {
        TScopedLock Lock(ClosedWindowsCS);
        ClosedWindows.Emplace(Window);
    }

    {
        TScopedLock Lock(WindowsCS);
        Windows.Remove(Window);
    }
}

LRESULT FWindowsApplication::StaticMessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    return WindowsApplication ? WindowsApplication->MessageProc(Window, Message, wParam, lParam) : DefWindowProc(Window, Message, wParam, lParam);
}

void FWindowsApplication::HandleStoredMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY)
{
    FWindowsWindowRef MessageWindow = GetWindowsWindowFromHWND(Window);
    switch (Message)
    {
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        {
            if (MessageWindow)
            {
                const bool bHasFocus = (Message == WM_SETFOCUS);
                MessageListener->HandleWindowFocusChanged(MessageWindow, bHasFocus);
            }

            break;
        }

        case WM_MOUSELEAVE:
        {
            if (MessageWindow)
            {
                MessageListener->HandleWindowMouseLeft(MessageWindow);
            }

            bIsTrackingMouse = false;
            break;
        }

        case WM_SIZE:
        {
            if (MessageWindow)
            {
                const FPointMessage Size(lParam);
                MessageListener->HandleWindowResized(MessageWindow, Size.Width, Size.Height);
            }

            break;
        }

        case WM_MOVE:
        {
            if (MessageWindow)
            {
                const FPointMessage Size(lParam);
                MessageListener->HandleWindowMoved(MessageWindow, Size.x, Size.y);
            }

            break;
        }

        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            const uint32 ScanCode = static_cast<uint32>(HIWORD(lParam) & ScanCodeMask);
            const EKey Key = FWindowsKeyMapping::GetKeyCodeFromScanCode(ScanCode);

            MessageListener->HandleKeyReleased(Key, FPlatformApplicationMisc::GetModifierKeyState());
            break;
        }

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            const uint32 ScanCode = static_cast<uint32>(HIWORD(lParam) & ScanCodeMask);
            const EKey Key = FWindowsKeyMapping::GetKeyCodeFromScanCode(ScanCode);

            const bool bIsRepeat = !!(lParam & KeyRepeatMask);
            MessageListener->HandleKeyPressed(Key, bIsRepeat, FPlatformApplicationMisc::GetModifierKeyState());
            break;
        }

        case WM_SYSCHAR:
        case WM_CHAR:
        {
            const uint32 Character = static_cast<uint32>(wParam);
            MessageListener->HandleKeyChar(Character);
            break;
        }

        case WM_MOUSEMOVE:
        {
            const int32 x = GET_X_LPARAM(lParam);
            const int32 y = GET_Y_LPARAM(lParam);

            if (!bIsTrackingMouse)
            {
                TRACKMOUSEEVENT TrackEvent;
                FMemory::Memzero(&TrackEvent);

                TrackEvent.cbSize    = sizeof(TRACKMOUSEEVENT);
                TrackEvent.dwFlags   = TME_LEAVE;
                TrackEvent.hwndTrack = Window;
                TrackMouseEvent(&TrackEvent);

                MessageListener->HandleWindowMouseEntered(MessageWindow);

                bIsTrackingMouse = true;
            }

            MessageListener->HandleMouseMove(x, y);
            break;
        }

        case WM_INPUT:
        {
            MessageListener->HandleHighPrecisionMouseInput(MessageWindow, MouseDeltaX, MouseDeltaY);
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
            else if (GET_XBUTTON_WPARAM(wParam) == BackButtonMask)
            {
                Button = EMouseButton::MouseButton_Back;
            }
            else
            {
                Button = EMouseButton::MouseButton_Forward;
            }

            MessageListener->HandleMousePressed(Button, FPlatformApplicationMisc::GetModifierKeyState());
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
            else if (GET_XBUTTON_WPARAM(wParam) == BackButtonMask)
            {
                Button = EMouseButton::MouseButton_Back;
            }
            else
            {
                Button = EMouseButton::MouseButton_Forward;
            }

            MessageListener->HandleMouseReleased(Button, FPlatformApplicationMisc::GetModifierKeyState());
            break;
        }

        case WM_MOUSEWHEEL:
        {
            const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
            MessageListener->HandleMouseScrolled(0.0f, WheelDelta);
            break;
        }

        case WM_MOUSEHWHEEL:
        {
            const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
            MessageListener->HandleMouseScrolled(WheelDelta, 0.0f);
            break;
        }

        case WM_QUIT:
        {
            int32 ExitCode = static_cast<int32>(wParam);
            MessageListener->HandleApplicationExit(ExitCode);
            break;
        }

        default:
        {
            // Nothing for now
            break;
        }
    }
}

LRESULT FWindowsApplication::MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    // Let the message listeners (a.k.a other modules) listen to native messages
    LRESULT ResultFromListeners = 0;
    for (TSharedPtr<IWindowsMessageListener> NativeMessageListener : WindowsMessageListeners)
    {
        CHECK(NativeMessageListener != nullptr);

        LRESULT TempResult = NativeMessageListener->MessageProc(Window, Message, wParam, lParam);
        if (TempResult)
        {
            // TODO: Maybe some more checking?
            ResultFromListeners = TempResult;
        }
    }

    switch (Message)
    {
        case WM_INPUT:
        {
            return ProcessRawInput(Window, Message, wParam, lParam);
        }

        case WM_CLOSE:
        {
            FWindowsWindowRef MessageWindow = GetWindowsWindowFromHWND(Window);
            CloseWindow(MessageWindow);
            return ResultFromListeners;
        }

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
            StoreMessage(Window, Message, wParam, lParam, 0, 0);
            return ResultFromListeners;
        }
    }

    return DefWindowProc(Window, Message, wParam, lParam);
}

void FWindowsApplication::StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY)
{
    TScopedLock<FCriticalSection> Lock(MessagesCS);
    Messages.Emplace(Window, Message, wParam, lParam, MouseDeltaX, MouseDeltaY);
}

LRESULT FWindowsApplication::ProcessRawInput(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    UINT Size = 0;
    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &Size, sizeof(RAWINPUTHEADER));

    // TODO: Measure performance impact
    TUniquePtr<uint8[]> Buffer = MakeUnique<uint8[]>(Size);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, Buffer.Get(), &Size, sizeof(RAWINPUTHEADER)) != Size)
    {
        LOG_ERROR("[FWindowsApplication] GetRawInputData did not return correct size");
        return 0;
    }

    RAWINPUT* RawInput = reinterpret_cast<RAWINPUT*>(Buffer.Get());
    if (RawInput->header.dwType == RIM_TYPEMOUSE)
    {
        int32 DeltaX = RawInput->data.mouse.lLastX;
        int32 DeltaY = RawInput->data.mouse.lLastY;

        if (DeltaX != 0 || DeltaY != 0)
        {
            StoreMessage(Window, Message, wParam, lParam, DeltaX, DeltaY);
        }

        return 0;
    }
    else
    {
        return DefRawInputProc(&RawInput, 1, sizeof(RAWINPUTHEADER));
    }
}
