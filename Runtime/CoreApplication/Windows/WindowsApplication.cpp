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
    // Get the application instance
    HINSTANCE AppInstanceHandle = static_cast<HINSTANCE>(GetModuleHandleA(0));

    // TODO: Load icon resource here
    HICON Icon = ::LoadIcon(NULL, IDI_APPLICATION);

    TSharedPtr<FWindowsApplication> NewWindowsApplication = MakeSharedPtr<FWindowsApplication>(AppInstanceHandle, Icon);
    GWindowsApplication = NewWindowsApplication.Get();
    return NewWindowsApplication;
}

FWindowsApplication::FWindowsApplication(HINSTANCE InInstanceHandle, HICON InIcon)
    : FGenericApplication(TSharedPtr<ICursor>(new FWindowsCursor()))
    , Windows()
    , Messages()
    , MessagesCS()
    , WindowsMessageListeners()
    , bIsTrackingMouse(false)
    , bDeferredMessagesEnabled(false)
    , InstanceHandle(InInstanceHandle)
    , Icon(InIcon)
{
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

    // Init the key mapping Win32 KeyCodes -> EKeyboardKeyName
    FWindowsInputMapper::Initialize();

    // Run a check for connected devices
    XInputDevice.UpdateConnectionState();

    // Cache the value of the deferred messages enabled here at startup, we do not want to mix the messaging models
    bDeferredMessagesEnabled = CVarEnableDeferredMessages.GetValue();
 }

FWindowsApplication::~FWindowsApplication()
{
    Windows.Clear();

    if (GWindowsApplication == this)
    {
        GWindowsApplication = nullptr;
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
    WindowClass.lpfnWndProc   = FWindowsApplication::WindowProc;

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
    static constexpr uint32 DeviceCount = 1;

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
    TSharedRef<FWindowsWindow> NewWindow = FWindowsWindow::Create(this);
 
    {
        TScopedLock Lock(WindowsCS);
        Windows.Emplace(NewWindow);
    }

    return NewWindow;
}

void FWindowsApplication::Tick(float)
{
    if (!ClosedWindows.IsEmpty())
    {
        TScopedLock Lock(ClosedWindowsCS);
        ClosedWindows.Clear();
    }
}

void FWindowsApplication::ProcessEvents()
{
    MSG Message;

    BOOL Result = PeekMessage(&Message, 0, 0, 0, PM_REMOVE);
    if (Result)
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
}

void FWindowsApplication::ProcessDeferredEvents()
{
    TArray<FWindowsDeferredMessage> LocalMessages;
    if (!Messages.IsEmpty())
    {
        TScopedLock<FCriticalSection> Lock(MessagesCS);
        LocalMessages = Move(Messages);
        Messages.Clear();
    }

    // Handle all the messages 
    for (const FWindowsDeferredMessage& Message : LocalMessages)
    {
        ProcessDeferredMessage(Message);
    }
}

void FWindowsApplication::UpdateInputDevices()
{
    XInputDevice.UpdateDeviceState();
}

FInputDevice* FWindowsApplication::GetInputDevice()
{
    return &XInputDevice;
}

bool FWindowsApplication::SupportsHighPrecisionMouse() const
{
    return false;
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

FModifierKeyState FWindowsApplication::GetModifierKeyState() const
{
    EModifierFlag ModifierFlags = EModifierFlag::None;
    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Ctrl;
    }
    if (GetKeyState(VK_MENU) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Alt;
    }
    if (GetKeyState(VK_SHIFT) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Shift;
    }
    if (GetKeyState(VK_CAPITAL) & 0x1)
    {
        ModifierFlags |= EModifierFlag::CapsLock;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Super;
    }
    if (GetKeyState(VK_NUMLOCK) & 0x1)
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
    // Use the for
    HWND ForegroundWindow = ::GetForegroundWindow();
    return GetWindowsWindowFromHWND(ForegroundWindow);
}

void FWindowsApplication::QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const
{
    ::EnumDisplayMonitors(nullptr, nullptr, FWindowsApplication::EnumerateMonitorsProc, reinterpret_cast<LPARAM>(&OutMonitorInfo));
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

LRESULT FWindowsApplication::WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    CHECK(GWindowsApplication != nullptr);
    return GWindowsApplication->ProcessMessage(Window, Message, wParam, lParam);
}

BOOL FWindowsApplication::EnumerateMonitorsProc(HMONITOR Monitor, HDC DeviceContext, LPRECT ClipRect, LPARAM lParam)
{
    CHECK(GWindowsApplication != nullptr);
    return GWindowsApplication->EnumerateMonitors(Monitor, DeviceContext, ClipRect, lParam);
}

BOOL FWindowsApplication::EnumerateMonitors(HMONITOR Monitor, HDC DeviceContext, LPRECT ClipRect, LPARAM lParam)
{
    TArray<FMonitorInfo>* MonitorInfos = reinterpret_cast<TArray<FMonitorInfo>*>(lParam);
    CHECK(MonitorInfos != nullptr);

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

    FMonitorInfo NewMonitorInfo;
    NewMonitorInfo.DeviceName     = FString(MonitorInfo.szDevice);
    NewMonitorInfo.MainPosition   = FIntVector2(MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top);
    NewMonitorInfo.MainSize       = FIntVector2(MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top);
    NewMonitorInfo.WorkPosition   = FIntVector2(MonitorInfo.rcWork.left, MonitorInfo.rcWork.top);
    NewMonitorInfo.WorkSize       = FIntVector2(MonitorInfo.rcWork.right - MonitorInfo.rcWork.left, MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top);
    NewMonitorInfo.bIsPrimary     = (MonitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    NewMonitorInfo.DisplayDPI     = DpiX;
    NewMonitorInfo.DisplayScaling = static_cast<float>(DpiX) / 96.0f;

    MonitorInfos->Add(NewMonitorInfo);
    return TRUE;
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
            const bool bIsVertical = Message.MessageType == WM_MOUSEWHEEL;

            const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(Message.wParam)) / static_cast<float>(WHEEL_DELTA);
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
    else if (Message.MessageType == WM_MOUSEMOVE)
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
        const uint32 Width  = static_cast<uint32>(LOWORD(Message.lParam));
        const uint32 Height = static_cast<uint32>(HIWORD(Message.lParam));
        
        MessageHandler->OnWindowResized(Message.Window, Width, Height);
    }
}

void FWindowsApplication::ProcessWindowMoveMessage(const FWindowsDeferredMessage& Message)
{
    if (Message.Window)
    {
        const uint32 x = static_cast<uint32>(LOWORD(Message.lParam));
        const uint32 y = static_cast<uint32>(HIWORD(Message.lParam));

        MessageHandler->OnWindowMoved(Message.Window, x, y);
    }
}

void FWindowsApplication::ProcessKeyMessage(const FWindowsDeferredMessage& Message)
{
    static constexpr uint32 WindowsScanCodeMask  = 0x01ff;
    static constexpr uint32 WindowsKeyRepeatMask = 0x40000000;

    // Retrieve the ScanCode...
    const uint32 ScanCode = static_cast<uint32>(HIWORD(Message.lParam) & WindowsScanCodeMask);
    
    // ... then use the ScanCode to retrieve a KeyboardKeyName
    const EKeyboardKeyName::Type Key = FWindowsInputMapper::GetKeyCodeFromScanCode(ScanCode);

    if (Message.MessageType == WM_KEYUP || Message.MessageType == WM_SYSKEYUP)
    {
        MessageHandler->OnKeyUp(Key, GetModifierKeyState());
    }
    else if (Message.MessageType == WM_KEYDOWN || Message.MessageType == WM_SYSKEYDOWN)
    {
        const bool bIsRepeat = (Message.lParam & WindowsKeyRepeatMask) != 0;
        MessageHandler->OnKeyDown(Key, bIsRepeat, GetModifierKeyState());
    }
}

void FWindowsApplication::ProcessKeyCharMessage(const FWindowsDeferredMessage& Message)
{
    const uint32 Character = static_cast<uint32>(Message.wParam);
    MessageHandler->OnKeyChar(Character);
}

void FWindowsApplication::ProcessMouseMoveMessage(const FWindowsDeferredMessage& Message)
{
    POINT CursorPos;
    ::GetCursorPos(&CursorPos);

    MessageHandler->OnMouseMove(CursorPos.x, CursorPos.y);
}

void FWindowsApplication::ProcessMouseButtonMessage(const FWindowsDeferredMessage& Message)
{
    static constexpr uint32 WindowsBackButtonMask = 0x0001;

    switch(Message.MessageType)
    {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        {
            EMouseButtonName::Type Button = EMouseButtonName::Unknown;
            if (Message.MessageType == WM_LBUTTONDOWN)
            {
                Button = EMouseButtonName::Left;
            }
            else if (Message.MessageType == WM_MBUTTONDOWN)
            {
                Button = EMouseButtonName::Middle;
            }
            else if (Message.MessageType == WM_RBUTTONDOWN)
            {
                Button = EMouseButtonName::Right;
            }
            else if (Message.MessageType == WM_XBUTTONDOWN)
            {
                if (GET_XBUTTON_WPARAM(Message.wParam) == WindowsBackButtonMask)
                {
                    Button = EMouseButtonName::Thumb1;
                }
                else
                {
                    Button = EMouseButtonName::Thumb2;
                }
            }

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
            EMouseButtonName::Type Button = EMouseButtonName::Unknown;
            if (Message.MessageType == WM_LBUTTONDBLCLK)
            {
                Button = EMouseButtonName::Left;
            }
            else if (Message.MessageType == WM_MBUTTONDBLCLK)
            {
                Button = EMouseButtonName::Middle;
            }
            else if (Message.MessageType == WM_RBUTTONDBLCLK)
            {
                Button = EMouseButtonName::Right;
            }
            else if (Message.MessageType == WM_XBUTTONDBLCLK)
            {
                if (GET_XBUTTON_WPARAM(Message.wParam) == WindowsBackButtonMask)
                {
                    Button = EMouseButtonName::Thumb1;
                }
                else
                {
                    Button = EMouseButtonName::Thumb2;
                }
            }

            MessageHandler->OnMouseButtonDoubleClick(Button, GetModifierKeyState());
            break;
        }

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        {
            EMouseButtonName::Type Button = EMouseButtonName::Unknown;
            if (Message.MessageType == WM_LBUTTONUP)
            {
                Button = EMouseButtonName::Left;
            }
            else if (Message.MessageType == WM_MBUTTONUP)
            {
                Button = EMouseButtonName::Middle;
            }
            else if (Message.MessageType == WM_RBUTTONUP)
            {
                Button = EMouseButtonName::Right;
            }
            else if (Message.MessageType == WM_XBUTTONUP)
            {
                if (GET_XBUTTON_WPARAM(Message.wParam) == WindowsBackButtonMask)
                {
                    Button = EMouseButtonName::Thumb1;
                }
                else
                {
                    Button = EMouseButtonName::Thumb2;
                }
            }

            MessageHandler->OnMouseButtonUp(Button, GetModifierKeyState());
            break;
        }
    }
}

LRESULT FWindowsApplication::ProcessMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    LRESULT ResultFromListeners = 0;

    // Let the message listeners (a.k.a other modules) listen to native messages
    bool bIsMessageHandledExternally = false;
    for (TSharedPtr<IWindowsMessageListener> NativeMessageListener : WindowsMessageListeners)
    {
        CHECK(NativeMessageListener != nullptr);

        LRESULT MessageResult = NativeMessageListener->MessageProc(Window, Message, wParam, lParam);
        if (MessageResult)
        {
            if (!bIsMessageHandledExternally)
            {
                ResultFromListeners         = MessageResult;
                bIsMessageHandledExternally = true;
            }
        }
    }

    switch (Message)
    {
        // We need to process the "raw" input here and store the result later 
        case WM_INPUT:
        {
            return ProcessRawInput(Window, Message, wParam, lParam);
        }

        // If the window is currently resizing we need to handle it directly
        case WM_SIZING:
        {
            if (TSharedRef<FWindowsWindow> MessageWindow = GetWindowsWindowFromHWND(Window))
            {
                MessageHandler->OnWindowResizing(MessageWindow);
            }

            break;
        }

        // Defer these messages to be handled later when the event loop is explicitly run
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
            FWindowsDeferredMessage DeferredMessage;
            DeferredMessage.Window       = GetWindowsWindowFromHWND(Window);
            DeferredMessage.WindowHandle = Window;
            DeferredMessage.MessageType  = Message;
            DeferredMessage.wParam       = wParam;
            DeferredMessage.lParam       = lParam;

            DeferMessage(DeferredMessage);
            return 0;
        }

        default:
        {
            break;
        }
    }

    // If we do not handle the message, but an external message handler has handled the message, 
    // we return that result here. 
    if (bIsMessageHandledExternally)
    {
        return ResultFromListeners;
    }

    // We did not handle the message and there were no external message handler that dealt with this message,
    // so now we return the default value for this message.
    return ::DefWindowProc(Window, Message, wParam, lParam);
}

void FWindowsApplication::DeferMessage(const FWindowsDeferredMessage& InDeferredMessage)
{
    // If the we want to defer messages we put them into a message queue here...
    if (bDeferredMessagesEnabled = CVarEnableDeferredMessages.GetValue())
    {
        TScopedLock<FCriticalSection> Lock(MessagesCS);
        Messages.Emplace(InDeferredMessage);
    }
    else
    {
        // ... otherwise we process it directly here.
        ProcessDeferredMessage(InDeferredMessage);
    }
}

LRESULT FWindowsApplication::ProcessRawInput(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    HRAWINPUT ParamInput = reinterpret_cast<HRAWINPUT>(lParam);

    // Retrieve the size of the input data
    UINT Size = 0;
    ::GetRawInputData(ParamInput, RID_INPUT, nullptr, &Size, sizeof(RAWINPUTHEADER));

    TUniquePtr<uint8[]> Buffer = MakeUniquePtr<uint8[]>(Size);

    // Retrieve the input data
    UINT ResultSize = ::GetRawInputData(ParamInput, RID_INPUT, Buffer.Get(), &Size, sizeof(RAWINPUTHEADER));
    if (ResultSize != Size)
    {
        LOG_ERROR("[FWindowsApplication] GetRawInputData did not return correct size");
        return 0;
    }

    RAWINPUT* RawInputData = reinterpret_cast<RAWINPUT*>(Buffer.Get());
    if (RawInputData)
    {
        switch(RawInputData->header.dwType)
        {
            case RIM_TYPEMOUSE:
            {
                int32 DeltaX = RawInputData->data.mouse.lLastX;
                int32 DeltaY = RawInputData->data.mouse.lLastY;

                if (DeltaX != 0 || DeltaY != 0)
                {
                    FWindowsDeferredMessage DeferredMessage;
                    DeferredMessage.Window       = GetWindowsWindowFromHWND(Window);
                    DeferredMessage.WindowHandle = Window;
                    DeferredMessage.MessageType  = Message;
                    DeferredMessage.wParam       = wParam;
                    DeferredMessage.lParam       = lParam;
                    DeferredMessage.MouseDelta   = FIntVector2(DeltaX, DeltaY);

                    DeferMessage(DeferredMessage);
                }

                return 0;
            }
        }
    }

    return ::DefRawInputProc(&RawInputData, 1, sizeof(RAWINPUTHEADER));
}
