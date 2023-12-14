#include "WindowsApplication.h"
#include "WindowsInputMapper.h"
#include "WindowsCursor.h"
#include "WindowsWindow.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

#define WINDOWS_SCAN_CODE_MASK   (0x01ff)
#define WINDOWS_KEY_REPEAT_MASK  (0x40000000)
#define WINDOWS_BACK_BUTTON_MASK (0x0001)

static TAutoConsoleVariable<bool> CVarIsProcessDPIAware(
    "Windows.IsProcessDPIAware", 
    "If set to true the process is set to be DPI aware, otherwise not", 
    true,
    EConsoleVariableFlags::Default);

COREAPPLICATION_API FWindowsApplication* WindowsApplication = nullptr;

TSharedPtr<FWindowsApplication> FWindowsApplication::CreateWindowsApplication()
{
    // Get the application instance
    HINSTANCE AppInstanceHandle = static_cast<HINSTANCE>(GetModuleHandleA(0)); 
    // TODO: Load icon resource here
    HICON Icon = ::LoadIcon(NULL, IDI_APPLICATION);

    TSharedPtr<FWindowsApplication> NewWindowsApplication = MakeShared<FWindowsApplication>(AppInstanceHandle, Icon);
    WindowsApplication = NewWindowsApplication.Get();
    return NewWindowsApplication;
}

FWindowsApplication::FWindowsApplication(HINSTANCE InInstanceHandle, HICON InIcon)
    : FGenericApplication(TSharedPtr<ICursor>(new FWindowsCursor()))
    , Windows()
    , Messages()
    , MessagesCS()
    , WindowsMessageListeners()
    , bIsTrackingMouse(false)
    , bHasDisplayInfoChanged(true)
    , InstanceHandle(InInstanceHandle)
    , Icon(InIcon)
{
    if (CVarIsProcessDPIAware.GetValue())
    {
#if PLATFORM_WINDOWS_10
        ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#elif PLATFORM_WINDOWS_VISTA
        ::SetProcessDPIAware();
#endif
    }

    const bool bResult = RegisterWindowClass();
    CHECK(bResult == true);

    // Init the key mapping Win32 KeyCodes -> EKeyboardKeyName
    FWindowsInputMapper::Initialize();

    // Run a check for connected devices
    XInputDevice.UpdateConnectionState();
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

    WindowClass.style         = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC;
    WindowClass.hInstance     = InstanceHandle;
    WindowClass.hIcon         = Icon;
    WindowClass.lpszClassName = FWindowsWindow::GetClassName();
    WindowClass.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
    WindowClass.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    WindowClass.lpfnWndProc   = FWindowsApplication::StaticMessageProc;

    ATOM ClassAtom = ::RegisterClass(&WindowClass);
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

    const BOOL bResult = ::RegisterRawInputDevices(Devices, DeviceCount, sizeof(RAWINPUTDEVICE));
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

    const BOOL bResult = ::RegisterRawInputDevices(Devices, DeviceCount, sizeof(RAWINPUTDEVICE));
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

TSharedRef<FGenericWindow> FWindowsApplication::CreateWindow()
{
    TSharedRef<FWindowsWindow> NewWindow = new FWindowsWindow(this);
 
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

        for (const TSharedRef<FWindowsWindow>& Window : ClosedWindows)
        {
            MessageHandler->OnWindowClosed(Window);
        }

        ClosedWindows.Clear();
    }
}

void FWindowsApplication::UpdateGamepadDevices()
{
    XInputDevice.UpdateDeviceState();
}

FInputDevice* FWindowsApplication::GetInputDeviceInterface()
{
    return &XInputDevice;
}

bool FWindowsApplication::EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window)
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

void FWindowsApplication::SetCapture(const TSharedRef<FGenericWindow>& Window)
{
    TSharedRef<FWindowsWindow> WindowsWindow = StaticCastSharedRef<FWindowsWindow>(Window);
    if (WindowsWindow && WindowsWindow->IsValid())
    {
        HWND CaptureWindow = WindowsWindow->GetWindowHandle();
        ::SetCapture(CaptureWindow);
    }
    else
    {
        ::ReleaseCapture();
    }
}

void FWindowsApplication::SetActiveWindow(const TSharedRef<FGenericWindow>& Window)
{
    TSharedRef<FWindowsWindow> WindowsWindow = StaticCastSharedRef<FWindowsWindow>(Window);
    if (WindowsWindow && WindowsWindow->IsValid())
    {
        HWND ActiveWindow = WindowsWindow->GetWindowHandle();
        ::SetActiveWindow(ActiveWindow);
    }
}

TSharedRef<FGenericWindow> FWindowsApplication::GetCapture() const
{
    HWND CaptureWindow = ::GetCapture();
    return GetWindowsWindowFromHWND(CaptureWindow);
}

TSharedRef<FGenericWindow> FWindowsApplication::GetActiveWindow() const
{
    HWND ActiveWindow = ::GetActiveWindow();
    return GetWindowsWindowFromHWND(ActiveWindow);
}

TSharedRef<FGenericWindow> FWindowsApplication::GetForegroundWindow() const
{
    HWND ForegroundWindow = ::GetForegroundWindow();
    return GetWindowsWindowFromHWND(ForegroundWindow);
}

void FWindowsApplication::GetDisplayInfo(FDisplayInfo& OutDisplayInfo) const
{
    if (bHasDisplayInfoChanged)
    {
        ::EnumDisplayMonitors(nullptr, nullptr, FWindowsApplication::EnumerateMonitorsProc, reinterpret_cast<LPARAM>(this));
        DisplayInfo.MonitorInfos.Shrink();
        bHasDisplayInfoChanged = false;
    }

    OutDisplayInfo = DisplayInfo;
}

void FWindowsApplication::SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
{
    FGenericApplication::SetMessageHandler(InMessageHandler);
    XInputDevice.SetMessageHandler(InMessageHandler);
}

TSharedRef<FGenericWindow> FWindowsApplication::GetWindowUnderCursor() const
{
    POINT CursorPos;
    ::GetCursorPos(&CursorPos);

    HWND Handle = ::WindowFromPoint(CursorPos);
    return GetWindowsWindowFromHWND(Handle);
}

TSharedRef<FWindowsWindow> FWindowsApplication::GetWindowsWindowFromHWND(HWND InWindow) const
{
    if (::IsWindow(InWindow))
    {
        TScopedLock Lock(WindowsCS);

        for (const TSharedRef<FWindowsWindow>& Window : Windows)
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

void FWindowsApplication::CloseWindow(const TSharedRef<FWindowsWindow>& Window)
{
    CHECK(Window != nullptr);

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
    return WindowsApplication ? WindowsApplication->MessageProc(Window, Message, wParam, lParam) : ::DefWindowProc(Window, Message, wParam, lParam);
}

BOOL FWindowsApplication::EnumerateMonitorsProc(HMONITOR Monitor, HDC, LPRECT, LPARAM Data)
{
    FWindowsApplication* InWindowsApplication = reinterpret_cast<FWindowsApplication*>(Data);
    CHECK(InWindowsApplication != nullptr);

    MONITORINFOEX MonitorInfo;

    FMemory::Memzero(&MonitorInfo);
    MonitorInfo.cbSize = sizeof(MONITORINFOEX);

    BOOL Result = ::GetMonitorInfoA(Monitor, &MonitorInfo);
    if (!Result)
    {
        return TRUE;
    }

    UINT DpiX = 96;
    UINT DpiY = 96;
    ::GetDpiForMonitor(Monitor, MDT_EFFECTIVE_DPI, &DpiX, &DpiY);
    CHECK(DpiX == DpiY);

    DEVICE_SCALE_FACTOR ScalingFactor;
    ::GetScaleFactorForMonitor(Monitor, &ScalingFactor);

    FMonitorInfo NewMonitorInfo;
    NewMonitorInfo.DeviceName         = FString(MonitorInfo.szDevice);
    NewMonitorInfo.MainPosition       = FIntVector2(MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top);
    NewMonitorInfo.MainSize           = FIntVector2(MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top);
    NewMonitorInfo.WorkPosition       = FIntVector2(MonitorInfo.rcWork.left, MonitorInfo.rcWork.top);
    NewMonitorInfo.WorkSize           = FIntVector2(MonitorInfo.rcWork.right - MonitorInfo.rcWork.left, MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top);
    NewMonitorInfo.bIsPrimary         = (MonitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    NewMonitorInfo.DisplayDPI         = DpiX;
    NewMonitorInfo.DisplayScaleFactor = static_cast<int32>(ScalingFactor);
    NewMonitorInfo.DisplayScaling     = static_cast<float>(DpiX) / 96.0f;

    if (NewMonitorInfo.bIsPrimary)
    {
        InWindowsApplication->DisplayInfo.PrimaryDisplayWidth  = NewMonitorInfo.MainSize.x;
        InWindowsApplication->DisplayInfo.PrimaryDisplayHeight = NewMonitorInfo.MainSize.y;
    }

    InWindowsApplication->DisplayInfo.MonitorInfos.Add(NewMonitorInfo);
    return TRUE;
}

void FWindowsApplication::HandleStoredMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY)
{
    TSharedRef<FWindowsWindow> MessageWindow = GetWindowsWindowFromHWND(Window);
    switch (Message)
    {
        case WM_SETFOCUS:
        {
            if (MessageWindow)
            {
                MessageHandler->OnWindowFocusGained(MessageWindow);
            }

            break;
        }

        case WM_KILLFOCUS:
        {
            if (MessageWindow)
            {
                MessageHandler->OnWindowFocusLost(MessageWindow);
            }

            break;
        }

        case WM_MOUSELEAVE:
        {
            if (MessageWindow)
            {
                MessageHandler->OnWindowMouseLeft(MessageWindow);
            }

            bIsTrackingMouse = false;
            break;
        }

        case WM_SIZE:
        {
            if (MessageWindow)
            {
                const uint16 Width  = LOWORD(lParam);
                const uint16 Height = HIWORD(lParam);
                MessageHandler->OnWindowResized(MessageWindow, Width, Height);
            }

            break;
        }

        case WM_MOVE:
        {
            if (MessageWindow)
            {
                const uint16 x = LOWORD(lParam);
                const uint16 y = HIWORD(lParam);
                MessageHandler->OnWindowMoved(MessageWindow, x, y);
            }

            break;
        }

        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            const uint32 ScanCode = static_cast<uint32>(HIWORD(lParam) & WINDOWS_SCAN_CODE_MASK);
            const EKeyboardKeyName::Type Key = FWindowsInputMapper::GetKeyCodeFromScanCode(ScanCode);
            MessageHandler->OnKeyUp(Key, FPlatformApplicationMisc::GetModifierKeyState());
            break;
        }

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            const uint32 ScanCode = static_cast<uint32>(HIWORD(lParam) & WINDOWS_SCAN_CODE_MASK);
            const EKeyboardKeyName::Type Key = FWindowsInputMapper::GetKeyCodeFromScanCode(ScanCode);
            const bool bIsRepeat = (lParam & WINDOWS_KEY_REPEAT_MASK) != 0;
            MessageHandler->OnKeyDown(Key, bIsRepeat, FPlatformApplicationMisc::GetModifierKeyState());
            break;
        }

        case WM_SYSCHAR:
        case WM_CHAR:
        {
            const uint32 Character = static_cast<uint32>(wParam);
            MessageHandler->OnKeyChar(Character);
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
                ::TrackMouseEvent(&TrackEvent);

                MessageHandler->OnWindowMouseEntered(MessageWindow);

                bIsTrackingMouse = true;
            }

            MessageHandler->OnMouseMove(x, y);
            break;
        }

        case WM_INPUT:
        {
            MessageHandler->OnHighPrecisionMouseInput(MessageWindow, MouseDeltaX, MouseDeltaY);
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        {
            EMouseButtonName::Type Button = EMouseButtonName::Unknown;
            if (Message == WM_LBUTTONDOWN)
            {
                Button = EMouseButtonName::Left;
            }
            else if (Message == WM_MBUTTONDOWN)
            {
                Button = EMouseButtonName::Middle;
            }
            else if (Message == WM_RBUTTONDOWN)
            {
                Button = EMouseButtonName::Right;
            }
            else if (Message == WM_XBUTTONDOWN)
            {
                if (GET_XBUTTON_WPARAM(wParam) == WINDOWS_BACK_BUTTON_MASK)
                {
                    Button = EMouseButtonName::Thumb1;
                }
                else
                {
                    Button = EMouseButtonName::Thumb2;
                }
            }

            const int32 x = GET_X_LPARAM(lParam);
            const int32 y = GET_Y_LPARAM(lParam);
            MessageHandler->OnMouseButtonDown(MessageWindow, Button, FPlatformApplicationMisc::GetModifierKeyState(), x, y);
            break;
        }

        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        {
            EMouseButtonName::Type Button = EMouseButtonName::Unknown;
            if (Message == WM_LBUTTONDBLCLK)
            {
                Button = EMouseButtonName::Left;
            }
            else if (Message == WM_MBUTTONDBLCLK)
            {
                Button = EMouseButtonName::Middle;
            }
            else if (Message == WM_RBUTTONDBLCLK)
            {
                Button = EMouseButtonName::Right;
            }
            else if (Message == WM_XBUTTONDBLCLK)
            {
                if (GET_XBUTTON_WPARAM(wParam) == WINDOWS_BACK_BUTTON_MASK)
                {
                    Button = EMouseButtonName::Thumb1;
                }
                else
                {
                    Button = EMouseButtonName::Thumb2;
                }
            }

            const int32 x = GET_X_LPARAM(lParam);
            const int32 y = GET_Y_LPARAM(lParam);
            MessageHandler->OnMouseButtonDoubleClick(MessageWindow, Button, FPlatformApplicationMisc::GetModifierKeyState(), x, y);
            break;
        }

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        {
            EMouseButtonName::Type Button = EMouseButtonName::Unknown;
            if (Message == WM_LBUTTONUP)
            {
                Button = EMouseButtonName::Left;
            }
            else if (Message == WM_MBUTTONUP)
            {
                Button = EMouseButtonName::Middle;
            }
            else if (Message == WM_RBUTTONUP)
            {
                Button = EMouseButtonName::Right;
            }
            else if (Message == WM_XBUTTONUP)
            {
                if (GET_XBUTTON_WPARAM(wParam) == WINDOWS_BACK_BUTTON_MASK)
                {
                    Button = EMouseButtonName::Thumb1;
                }
                else
                {
                    Button = EMouseButtonName::Thumb2;
                }
            }

            const int32 x = GET_X_LPARAM(lParam);
            const int32 y = GET_Y_LPARAM(lParam);
            MessageHandler->OnMouseButtonUp(Button, FPlatformApplicationMisc::GetModifierKeyState(), x, y);
            break;
        }

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            const int32 x = GET_X_LPARAM(lParam);
            const int32 y = GET_Y_LPARAM(lParam);

            const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);

            const bool bIsVertical = (Message == WM_MOUSEWHEEL);
            MessageHandler->OnMouseScrolled(WheelDelta, bIsVertical, x, y);
            break;
        }

        case WM_DEVICECHANGE:
        {
            if (static_cast<UINT>(wParam) == DBT_DEVNODES_CHANGED)
            {
                XInputDevice.UpdateConnectionState();
            }

            break;
        }

        case WM_DISPLAYCHANGE:
        {
            bHasDisplayInfoChanged = true;
            MessageHandler->OnMonitorChange();
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
            // TODO: This does not currently feel like the best way of handling closing of windows
            TSharedRef<FWindowsWindow> MessageWindow = GetWindowsWindowFromHWND(Window);
            if (MessageWindow)
            {
                MessageWindow->Destroy();
            }

            return ResultFromListeners;
        }

        case WM_DESTROY:
        {
            TSharedRef<FWindowsWindow> MessageWindow = GetWindowsWindowFromHWND(Window);
            if (MessageWindow)
            {
                CloseWindow(MessageWindow);
            }

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
        case WM_DEVICECHANGE:
        case WM_DISPLAYCHANGE:
        {
            StoreMessage(Window, Message, wParam, lParam, 0, 0);
            return ResultFromListeners;
        }
    }

    return ::DefWindowProc(Window, Message, wParam, lParam);
}

void FWindowsApplication::StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY)
{
    TScopedLock<FCriticalSection> Lock(MessagesCS);
    Messages.Emplace(Window, Message, wParam, lParam, MouseDeltaX, MouseDeltaY);
}

LRESULT FWindowsApplication::ProcessRawInput(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    UINT Size = 0;
    ::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, 0, &Size, sizeof(RAWINPUTHEADER));

    TUniquePtr<uint8[]> Buffer = MakeUnique<uint8[]>(Size);
    if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, Buffer.Get(), &Size, sizeof(RAWINPUTHEADER)) != Size)
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
        return ::DefRawInputProc(&RawInput, 1, sizeof(RAWINPUTHEADER));
    }
}
