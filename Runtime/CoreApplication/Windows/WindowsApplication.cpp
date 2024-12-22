#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Windows/WindowsApplication.h"
#include "CoreApplication/Windows/WindowsInputMapper.h"
#include "CoreApplication/Windows/WindowsCursor.h"
#include "CoreApplication/Windows/WindowsWindow.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

static TAutoConsoleVariable<bool> CVarIsProcessDPIAware(
    "Windows.IsProcessDPIAware", 
    "If set to true the process is set to be DPI aware, otherwise not", 
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarEnableDeferredMessages(
    "Windows.EnableDeferredMessages",
    "If set to true the application defers all windows messages to be processed at a single point in time",
    true,
    EConsoleVariableFlags::Default);

COREAPPLICATION_API FWindowsApplication* GWindowsApplication = nullptr;

TSharedPtr<FGenericApplication> FWindowsApplication::Create()
{
    // Get the application instance (HINSTANCE)
    HINSTANCE AppInstanceHandle = static_cast<HINSTANCE>(::GetModuleHandleA(0));
    
    // TODO: Replace with actual icon resource
    HICON Icon = ::LoadIcon(0, IDI_APPLICATION);

    TSharedPtr<FWindowsApplication> NewWindowsApplication = MakeSharedPtr<FWindowsApplication>(AppInstanceHandle, Icon);
    GWindowsApplication = NewWindowsApplication .Get();
    return NewWindowsApplication;
}

FWindowsApplication::FWindowsApplication(HINSTANCE InInstanceHandle, HICON InIcon)
    : FGenericApplication(TSharedPtr<ICursor>(new FWindowsCursor()))
    , Icon(InIcon)
    , InstanceHandle(InInstanceHandle)
    , XInputDevice()
    , bIsTrackingMouse(false)
    , bDeferredMessagesEnabled(false)
    , Messages()
    , MessagesCS()
    , WindowsMessageListeners()
    , WindowsMessageListenersCS()
    , Windows()
    , WindowsCS()
    , ClosedWindows()
    , ClosedWindowsCS()
{
    // If DPI awareness is enabled via console variable, set process DPI awareness
    if (CVarIsProcessDPIAware.GetValue())
    {
    #if PLATFORM_WINDOWS_10
        ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    #elif PLATFORM_WINDOWS_VISTA
        ::SetProcessDPIAware();
    #endif
    }

    const bool bResult = RegisterWindowClass();
    CHECK(bResult == true);

    // Initialize the Win32 -> EKeyboardKeyName mapping
    FWindowsInputMapper::Initialize();

    // Update XInput device connection state once at startup
    XInputDevice.UpdateConnectionState();

    // Cache the cvar value for deferred messages
    bDeferredMessagesEnabled = CVarEnableDeferredMessages.GetValue();
}

FWindowsApplication::~FWindowsApplication()
{
    // Clean up tracked windows
    {
        TScopedLock Lock(WindowsCS);
        Windows.Clear();
    }

    // Unset global pointer if this instance is the global app
    if (GWindowsApplication == this)
    {
        GWindowsApplication = nullptr;
    }
}

bool FWindowsApplication::RegisterWindowClass()
{
    WNDCLASSA WindowClass;
    FMemory::Memzero(&WindowClass);

    WindowClass.style         = CS_DBLCLKS | CS_HREDRAW | CS_OWNDC;
    WindowClass.hInstance     = InstanceHandle;
    WindowClass.hIcon         = Icon;
    WindowClass.lpszClassName = FWindowsWindow::GetClassName();
    WindowClass.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
    WindowClass.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    WindowClass.lpfnWndProc   = &FWindowsApplication::WindowProc;

    ATOM ClassAtom = ::RegisterClassA(&WindowClass);
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
    FMemory::Memzero(Devices, sizeof(Devices));

    // Register Mouse as a raw input device
    Devices[0].dwFlags     = 0;
    Devices[0].hwndTarget  = Window;
    Devices[0].usUsage     = 0x02; // Mouse usage
    Devices[0].usUsagePage = 0x01; // Generic desktop controls

    const BOOL bResult = ::RegisterRawInputDevices(Devices, DeviceCount, sizeof(RAWINPUTDEVICE));
    if (!bResult)
    {
        LOG_ERROR("[FWindowsApplication] Failed to register Raw Input devices");
        return false;
    }

    LOG_INFO("[FWindowsApplication] Registered Raw Input devices");
    return true;
}

bool FWindowsApplication::UnregisterRawInputDevices()
{
    constexpr uint32 DeviceCount = 1;
    RAWINPUTDEVICE Devices[DeviceCount];
    FMemory::Memzero(Devices, sizeof(Devices));

    // Unregister Mouse
    Devices[0].dwFlags     = RIDEV_REMOVE;
    Devices[0].hwndTarget  = nullptr;
    Devices[0].usUsage     = 0x02; // Mouse usage
    Devices[0].usUsagePage = 0x01; // Generic desktop controls

    const BOOL bResult = ::RegisterRawInputDevices(Devices, DeviceCount, sizeof(RAWINPUTDEVICE));
    if (!bResult)
    {
        LOG_ERROR("[FWindowsApplication] Failed to unregister Raw Input devices");
        return false;
    }

    LOG_INFO("[FWindowsApplication] Unregistered Raw Input devices");
    return true;
}

TSharedRef<FGenericWindow> FWindowsApplication::CreateWindow()
{
    TSharedRef<FWindowsWindow> NewWindow = FWindowsWindow::Create(this);
    {
        TScopedLock Lock(WindowsCS);
        Windows.Emplace(NewWindow);
    }
    return NewWindow;
}

void FWindowsApplication::Tick(float)
{
    // Clear closed windows each tick, if any
    if (!ClosedWindows.IsEmpty())
    {
        TScopedLock Lock(ClosedWindowsCS);
        ClosedWindows.Clear();
    }
}

void FWindowsApplication::ProcessEvents()
{
    MSG Message;

    while (::PeekMessage(&Message, nullptr, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&Message);
        ::DispatchMessage(&Message);
    }
}

void FWindowsApplication::ProcessDeferredEvents()
{
    TArray<FWindowsDeferredMessage> LocalMessages;
    {
        TScopedLock<FCriticalSection> Lock(MessagesCS);
        if (!Messages.IsEmpty())
        {
            LocalMessages = Move(Messages);
            Messages.Clear();
        }
    }

    // Process deferred messages
    for (const FWindowsDeferredMessage& Message : LocalMessages)
    {
        ProcessDeferredMessage(Message);
    }
}

void FWindowsApplication::UpdateInputDevices()
{
    // Poll XInput device state
    XInputDevice.UpdateDeviceState();
}

FInputDevice* FWindowsApplication::GetInputDevice()
{
    return &XInputDevice;
}

bool FWindowsApplication::SupportsHighPrecisionMouse() const
{
    // By default, no high precision mouse is supported
    return false;
}

bool FWindowsApplication::EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window)
{
    TSharedRef<FWindowsWindow> WindowsWindow = StaticCastSharedRef<FWindowsWindow>(Window);
    if (WindowsWindow && WindowsWindow->IsValid())
    {
        return RegisterRawInputDevices(WindowsWindow->GetWindowHandle());
    }

    return false;
}

FModifierKeyState FWindowsApplication::GetModifierKeyState() const
{
    EModifierFlag ModifierFlags = EModifierFlag::None;

    if (::GetKeyState(VK_CONTROL) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Ctrl;
    }
    if (::GetKeyState(VK_MENU) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Alt;
    }
    if (::GetKeyState(VK_SHIFT) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Shift;
    }
    if (::GetKeyState(VK_CAPITAL) & 0x1)
    {
        ModifierFlags |= EModifierFlag::CapsLock;
    }
    if ((::GetKeyState(VK_LWIN) | ::GetKeyState(VK_RWIN)) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Super;
    }
    if (::GetKeyState(VK_NUMLOCK) & 0x1)
    {
        ModifierFlags |= EModifierFlag::NumLock;
    }

    return FModifierKeyState(ModifierFlags);
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
    HWND Foreground = ::GetForegroundWindow();
    return GetWindowsWindowFromHWND(Foreground);
}

void FWindowsApplication::QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const
{
    ::EnumDisplayMonitors(nullptr, nullptr, &FWindowsApplication::EnumerateMonitorsProc, reinterpret_cast<LPARAM>(&OutMonitorInfo));
    OutMonitorInfo.Shrink();
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

    // Return a null shared ref if not found
    return nullptr;
}

void FWindowsApplication::AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewListener)
{
    TScopedLock Lock(WindowsMessageListenersCS);

    if (!WindowsMessageListeners.Contains(NewListener))
    {
        WindowsMessageListeners.Emplace(NewListener);
    }
    else
    {
        LOG_WARNING("Tried to add a listener that already exists");
    }
}

void FWindowsApplication::RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& Listener)
{
    TScopedLock Lock(WindowsMessageListenersCS);
    WindowsMessageListeners.Remove(Listener);
}

bool FWindowsApplication::IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& Listener) const
{
    TScopedLock Lock(WindowsMessageListenersCS);
    return WindowsMessageListeners.Contains(Listener);
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

LRESULT FWindowsApplication::WindowProc(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
{
    CHECK(GWindowsApplication != nullptr);
    return GWindowsApplication->ProcessMessage(WindowHandle, Message, wParam, lParam);
}

BOOL FWindowsApplication::EnumerateMonitorsProc(HMONITOR Monitor, HDC DeviceContext, LPRECT ClipRect, LPARAM lParam)
{
    CHECK(GWindowsApplication != nullptr);
    return GWindowsApplication->EnumerateMonitors(Monitor, DeviceContext, ClipRect, lParam);
}

BOOL FWindowsApplication::EnumerateMonitors(HMONITOR Monitor, HDC /*DeviceContext*/, LPRECT /*ClipRect*/, LPARAM lParam)
{
    TArray<FMonitorInfo>* MonitorInfos = reinterpret_cast<TArray<FMonitorInfo>*>(lParam);
    CHECK(MonitorInfos != nullptr);

    MONITORINFOEXA MonitorInfo;
    FMemory::Memzero(&MonitorInfo);
    MonitorInfo.cbSize = sizeof(MONITORINFOEXA);

    if (!::GetMonitorInfoA(Monitor, &MonitorInfo))
    {
        return TRUE; // Continue enumeration
    }

    UINT DpiX = 96;
    UINT DpiY = 96;
    ::GetDpiForMonitor(Monitor, MDT_EFFECTIVE_DPI, &DpiX, &DpiY);
    CHECK(DpiX == DpiY);

    FMonitorInfo NewMonitorInfo;
    NewMonitorInfo.DeviceName     = FString(MonitorInfo.szDevice);
    NewMonitorInfo.MainPosition   = FIntVector2(MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top);
    NewMonitorInfo.MainSize       = FIntVector2(MonitorInfo.rcMonitor.right  - MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top);
    NewMonitorInfo.WorkPosition   = FIntVector2(MonitorInfo.rcWork.left, MonitorInfo.rcWork.top);
    NewMonitorInfo.WorkSize       = FIntVector2(MonitorInfo.rcWork.right - MonitorInfo.rcWork.left, MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top);
    NewMonitorInfo.bIsPrimary     = (MonitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    NewMonitorInfo.DisplayDPI     = DpiX;
    NewMonitorInfo.DisplayScaling = static_cast<float>(DpiX) / 96.0f;

    MonitorInfos->Add(NewMonitorInfo);
    return TRUE; // Continue enumeration
}

LRESULT FWindowsApplication::ProcessMessage(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
{
    LRESULT ResultFromListeners = 0;
    bool bIsMessageHandledExternally = false;

    // Let external listeners process the native messages first
    {
        TScopedLock Lock(WindowsMessageListenersCS);
        for (const TSharedPtr<IWindowsMessageListener>& NativeListener : WindowsMessageListeners)
        {
            CHECK(NativeListener != nullptr);
            LRESULT MessageResult = NativeListener->MessageProc(WindowHandle, Message, wParam, lParam);
            if (MessageResult != 0)
            {
                // If at least one listener handles this message, store the result
                if (!bIsMessageHandledExternally)
                {
                    ResultFromListeners = MessageResult;
                    bIsMessageHandledExternally = true;
                }
            }
        }
    }

    switch (Message)
    {
        case WM_INPUT:
        {
            // Raw input messages are processed immediately
            return ProcessRawInput(WindowHandle, Message, wParam, lParam);
        }

        case WM_SIZING:
        {
            if (TSharedRef<FWindowsWindow> MsgWindow = GetWindowsWindowFromHWND(WindowHandle))
            {
                MessageHandler->OnWindowResizing(MsgWindow);
            }

            break;
        }

        // Deferred messages
        case WM_DESTROY:
        case WM_CLOSE:
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
        case WM_NCMOUSEMOVE:
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
            FWindowsDeferredMessage DeferredMsg;
            DeferredMsg.Window       = GetWindowsWindowFromHWND(WindowHandle);
            DeferredMsg.WindowHandle = WindowHandle;
            DeferredMsg.MessageType  = Message;
            DeferredMsg.wParam       = wParam;
            DeferredMsg.lParam       = lParam;

            DeferMessage(DeferredMsg);
            return 0; // We handled it by deferring
        }

        default:
        {
            break;
        }
    }

    // If not handled by us but handled externally, return external result
    if (bIsMessageHandledExternally)
    {
        return ResultFromListeners;
    }

    // Fallback to default message handling
    return ::DefWindowProc(WindowHandle, Message, wParam, lParam);
}

void FWindowsApplication::DeferMessage(const FWindowsDeferredMessage& InDeferredMessage)
{
    if (bDeferredMessagesEnabled)
    {
        TScopedLock<FCriticalSection> Lock(MessagesCS);
        Messages.Emplace(InDeferredMessage);
    }
    else
    {
        ProcessDeferredMessage(InDeferredMessage);
    }
}

LRESULT FWindowsApplication::ProcessRawInput(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
{
    HRAWINPUT RawInputHandle = reinterpret_cast<HRAWINPUT>(lParam);

    // Query size of raw input
    UINT Size = 0;
    ::GetRawInputData(RawInputHandle, RID_INPUT, nullptr, &Size, sizeof(RAWINPUTHEADER));

    TUniquePtr<uint8[]> Buffer = MakeUniquePtr<uint8[]>(Size);

    // Retrieve the raw input data
    UINT ResultSize = ::GetRawInputData(RawInputHandle, RID_INPUT, Buffer.Get(), &Size, sizeof(RAWINPUTHEADER));
    if (ResultSize != Size)
    {
        LOG_ERROR("[FWindowsApplication] GetRawInputData returned incorrect size");
        return 0;
    }

    RAWINPUT* RawInputData = reinterpret_cast<RAWINPUT*>(Buffer.Get());
    if (!RawInputData)
    {
        return 0;
    }

    switch (RawInputData->header.dwType)
    {
        case RIM_TYPEMOUSE:
        {
            int32 DeltaX = RawInputData->data.mouse.lLastX;
            int32 DeltaY = RawInputData->data.mouse.lLastY;

            if (DeltaX != 0 || DeltaY != 0)
            {
                FWindowsDeferredMessage DeferredMsg;
                DeferredMsg.Window       = GetWindowsWindowFromHWND(WindowHandle);
                DeferredMsg.WindowHandle = WindowHandle;
                DeferredMsg.MessageType  = Message;
                DeferredMsg.wParam       = wParam;
                DeferredMsg.lParam       = lParam;
                DeferredMsg.MouseDelta   = FIntVector2(DeltaX, DeltaY);

                DeferMessage(DeferredMsg);
            }

            return 0;
        }

        default:
        {
            break;
        }
    }

    return ::DefRawInputProc(&RawInputData, 1, sizeof(RAWINPUTHEADER));
}

void FWindowsApplication::ProcessDeferredMessage(const FWindowsDeferredMessage& Message)
{
    switch (Message.MessageType)
    {
        case WM_SETFOCUS:
        {
            if (Message.Window)
            {
                MessageHandler->OnWindowFocusGained(Message.Window);
            }

            break;
        }

        case WM_KILLFOCUS:
        {
            if (Message.Window)
            {
                MessageHandler->OnWindowFocusLost(Message.Window);
            }

            break;
        }

        case WM_MOUSELEAVE:
        {
            ProcessWindowHoverMessage(Message);
            break;
        }

        case WM_SIZE:
        {
            ProcessWindowResizeMessage(Message);
            break;
        }

        case WM_MOVE:
        {
            ProcessWindowMoveMessage(Message);
            break;
        }

        case WM_CLOSE:
        {
            if (Message.Window)
            {
                MessageHandler->OnWindowClosed(Message.Window);
            }

            break;
        }

        case WM_DESTROY:
        {
            if (Message.Window)
            {
                CloseWindow(Message.Window);
            }

            break;
        }

        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        {
            ProcessKeyMessage(Message);
            break;
        }

        case WM_CHAR:
        case WM_SYSCHAR:
        {
            ProcessKeyCharMessage(Message);
            break;
        }

        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
        {
            ProcessWindowHoverMessage(Message);
            ProcessMouseMoveMessage(Message);
            break;
        }

        case WM_INPUT:
        {
            MessageHandler->OnHighPrecisionMouseInput(Message.MouseDelta.X, Message.MouseDelta.Y);
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
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        {
            ProcessMouseButtonMessage(Message);
            break;
        }

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            bool bIsVertical = (Message.MessageType == WM_MOUSEWHEEL);
            float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(Message.wParam)) / static_cast<float>(WHEEL_DELTA);
            MessageHandler->OnMouseScrolled(WheelDelta, bIsVertical);
            break;
        }

        case WM_DEVICECHANGE:
        {
            if (static_cast<UINT>(Message.wParam) == DBT_DEVNODES_CHANGED)
            {
                XInputDevice.UpdateConnectionState();
            }
            break;
        }

        case WM_DISPLAYCHANGE:
        {
            MessageHandler->OnMonitorConfigurationChange();
            break;
        }

        default:
        {
            break;
        }
    }
}

void FWindowsApplication::ProcessWindowHoverMessage(const FWindowsDeferredMessage& Message)
{
    if (Message.MessageType == WM_MOUSELEAVE)
    {
        if (Message.Window)
        {
            MessageHandler->OnMouseLeft();
        }
        bIsTrackingMouse = false;
    }
    else if (Message.MessageType == WM_MOUSEMOVE || Message.MessageType == WM_NCMOUSEMOVE)
    {
        if (!bIsTrackingMouse)
        {
            TRACKMOUSEEVENT TrackEvent;
            FMemory::Memzero(&TrackEvent);

            TrackEvent.cbSize    = sizeof(TRACKMOUSEEVENT);
            TrackEvent.dwFlags   = TME_LEAVE;
            TrackEvent.hwndTrack = Message.WindowHandle;
            ::TrackMouseEvent(&TrackEvent);

            bIsTrackingMouse = true;
            if (Message.Window)
            {
                MessageHandler->OnMouseEntered();
            }
        }
    }
}

void FWindowsApplication::ProcessWindowResizeMessage(const FWindowsDeferredMessage& Message)
{
    if (Message.Window)
    {
        uint32 Width  = static_cast<uint32>(LOWORD(Message.lParam));
        uint32 Height = static_cast<uint32>(HIWORD(Message.lParam));

        MessageHandler->OnWindowResized(Message.Window, Width, Height);
    }
}

void FWindowsApplication::ProcessWindowMoveMessage(const FWindowsDeferredMessage& Message)
{
    if (Message.Window)
    {
        uint32 X = static_cast<uint32>(LOWORD(Message.lParam));
        uint32 Y = static_cast<uint32>(HIWORD(Message.lParam));

        MessageHandler->OnWindowMoved(Message.Window, X, Y);
    }
}

void FWindowsApplication::ProcessKeyMessage(const FWindowsDeferredMessage& Message)
{
    static constexpr uint32 WindowsScanCodeMask  = 0x01ff;
    static constexpr uint32 WindowsKeyRepeatMask = 0x40000000;

    const uint32 ScanCode = static_cast<uint32>((Message.lParam >> 16) & WindowsScanCodeMask);
    const EKeyboardKeyName::Type Key = FWindowsInputMapper::GetKeyCodeFromScanCode(ScanCode);

    if (Message.MessageType == WM_KEYUP || Message.MessageType == WM_SYSKEYUP)
    {
        MessageHandler->OnKeyUp(Key, GetModifierKeyState());
    }
    else
    {
        const bool bIsRepeat = ((Message.lParam & WindowsKeyRepeatMask) != 0);
        MessageHandler->OnKeyDown(Key, bIsRepeat, GetModifierKeyState());
    }
}

void FWindowsApplication::ProcessKeyCharMessage(const FWindowsDeferredMessage& Message)
{
    const uint32 Character = static_cast<uint32>(Message.wParam);
    MessageHandler->OnKeyChar(Character);
}

void FWindowsApplication::ProcessMouseMoveMessage(const FWindowsDeferredMessage& /*Message*/)
{
    POINT CursorPos;
    ::GetCursorPos(&CursorPos);
    MessageHandler->OnMouseMove(CursorPos.x, CursorPos.y);
}

void FWindowsApplication::ProcessMouseButtonMessage(const FWindowsDeferredMessage& Message)
{
    static constexpr uint32 WindowsBackButtonMask = 0x0001;

    EMouseButtonName::Type Button = EMouseButtonName::Unknown;
    switch (Message.MessageType)
    {
        case WM_LBUTTONDOWN: 
        case WM_LBUTTONDBLCLK: 
        case WM_LBUTTONUP:
        {
            Button = EMouseButtonName::Left;
            break;
        }
        
        case WM_MBUTTONDOWN: 
        case WM_MBUTTONDBLCLK: 
        case WM_MBUTTONUP:
        {

            Button = EMouseButtonName::Middle;
            break;
        }
        
        case WM_RBUTTONDOWN: 
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONUP:
        {
            Button = EMouseButtonName::Right;
            break;
        }
        
        case WM_XBUTTONDOWN: 
        case WM_XBUTTONDBLCLK: 
        case WM_XBUTTONUP:
        {
            WORD XButton = GET_XBUTTON_WPARAM(Message.wParam);
            Button = (XButton == WindowsBackButtonMask) ? EMouseButtonName::Thumb1 : EMouseButtonName::Thumb2;
            break;
        }

        default:
        {
            break;
        }
    }

    switch (Message.MessageType)
    {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        {
            if (Message.Window)
            {
                MessageHandler->OnMouseButtonDown(Message.Window, Button, GetModifierKeyState());
            }

            break;
        }

        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        {
            MessageHandler->OnMouseButtonDoubleClick(Button, GetModifierKeyState());
            break;
        }

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        {
            MessageHandler->OnMouseButtonUp(Button, GetModifierKeyState());
            break;
        }

        default:
        {   
            break;
        }
    }
}
