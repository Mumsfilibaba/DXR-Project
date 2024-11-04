#pragma once 
#include "Windows.h"
#include "XInputDevice.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericApplication.h"

// On windows we defer messages to handle all events at once. This is due to some functions causes events
// to get sent directly to the window-procedure. By deferring the messages, we can handle all events at a
// single point.

struct FWindowsMessage
{
    FORCEINLINE FWindowsMessage(HWND InWindow, uint32 InMessage, WPARAM InwParam, LPARAM InlParam, int32 InMouseDeltaX, int32 InMouseDeltaY)
        : Window(InWindow)
        , Message(InMessage)
        , wParam(InwParam)
        , lParam(InlParam)
        , MouseDeltaX(InMouseDeltaX)
        , MouseDeltaY(InMouseDeltaY)
    {
    }

    // Standard Window Message
    HWND   Window;
    uint32 Message;
    WPARAM wParam;
    LPARAM lParam;

    // Used for Raw Mouse Input
    int32 MouseDeltaX;
    int32 MouseDeltaY;
};

struct IWindowsMessageListener
{
    virtual ~IWindowsMessageListener() = default;

    // Handles messages sent from the application's MessageProc
    // See https://docs.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
    virtual LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) = 0;
};

class FGenericWindow;
class FWindowsWindow;

class COREAPPLICATION_API FWindowsApplication final : public FGenericApplication
{
public:

    // Creates a FWindowsAppliction and returns it as a FGenericApplication. It also assigns the global 
    // WindowsApplication pointer in the FWindowsApplication constructor which is later reset in the 
    // FWindowsApplication destructor.
    static TSharedPtr<FGenericApplication> Create();

public:
    FWindowsApplication(HINSTANCE InInstance, HICON InIcon);
    virtual ~FWindowsApplication();

    // FGenericApplication Interface
    virtual TSharedRef<FGenericWindow> CreateWindow() override final;
    virtual void Tick(float Delta) override final;
    virtual void UpdateInputDevices() override final;
    virtual FInputDevice* GetInputDeviceInterface() override final;
    virtual bool SupportsHighPrecisionMouse() const override final { return false; }
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) override final;
    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) override final;
    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) override final;
    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const override final;
    virtual TSharedRef<FGenericWindow> GetCapture() const override final;
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const override final;
    virtual TSharedRef<FGenericWindow> GetForegroundWindow() const override final;
    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const override final;
    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler) override final;

    TSharedRef<FWindowsWindow> GetWindowsWindowFromHWND(HWND Window) const;
    void StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);
    void AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener);
    void RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener);
    bool IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener) const;
    void CloseWindow(const TSharedRef<FWindowsWindow>& Window);

    HINSTANCE GetInstance() const 
    { 
        return InstanceHandle;
    }

private:
    static LRESULT StaticMessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    static BOOL EnumerateMonitorsProc(HMONITOR Monitor, HDC DeviceContext, LPRECT ClipRect, LPARAM Data);

    bool RegisterWindowClass();
    bool RegisterRawInputDevices(HWND Window);
    bool UnregisterRawInputDevices();
    LRESULT ProcessRawInput(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    void HandleStoredMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);

    TArray<TSharedPtr<IWindowsMessageListener>> WindowsMessageListeners;
    mutable FCriticalSection WindowsMessageListenersCS;

    HICON         Icon;
    HINSTANCE     InstanceHandle;
    bool          bIsTrackingMouse;
    FXInputDevice XInputDevice;

    TArray<FWindowsMessage> Messages;
    FCriticalSection MessagesCS;

    TArray<TSharedRef<FWindowsWindow>> Windows;
    mutable FCriticalSection WindowsCS;

    TArray<TSharedRef<FWindowsWindow>> ClosedWindows;
    FCriticalSection ClosedWindowsCS;
};

extern COREAPPLICATION_API FWindowsApplication* WindowsApplication;