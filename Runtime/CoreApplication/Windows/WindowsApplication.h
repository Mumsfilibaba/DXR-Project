#pragma once 
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Windows/Windows.h"
#include "CoreApplication/Windows/XInputDevice.h"
#include "CoreApplication/Windows/WindowsWindow.h"
#include "CoreApplication/Windows/WindowsCursor.h"
#include "CoreApplication/PlatformInterface/InputCodes.h"
#include "CoreApplication/PlatformInterface/IPlatformApplication.h"

struct FWindowsDeferredMessage
{
    FORCEINLINE FWindowsDeferredMessage()
        : Window()
        , WindowHandle(0)
        , MessageType(0)
        , wParam(0)
        , lParam(0)
        , MouseDelta()
    {
    }

    FORCEINLINE FWindowsDeferredMessage(const FWindowsDeferredMessage& Other)
        : Window(Other.Window)
        , WindowHandle(Other.WindowHandle)
        , MessageType(Other.MessageType)
        , wParam(Other.wParam)
        , lParam(Other.lParam)
        , MouseDelta(Other.MouseDelta)
    {
    }

    /** @brief The window object associated with this message. */
    TSharedRef<FWindowsWindow> Window; 

    /** @brief The native HWND associated with this message. */
    HWND WindowHandle;

    /** @brief The Windows message type (WM_...). */
    uint32 MessageType;

    /** @brief Additional message information (e.g., high-order word often carries message-specific data). */
    WPARAM wParam;

    /** @brief Additional message information (e.g., low-order word often carries message-specific data). */
    LPARAM lParam;

    /** @brief Horizontal delta for raw mouse movements. */
    IntVector2 MouseDelta;
};

struct IWindowsMessageListener
{
    virtual ~IWindowsMessageListener() = default;

    virtual LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) = 0;
};

class COREAPPLICATION_API FWindowsApplication final : public IPlatformApplication
{
public:
    static TSharedPtr<IPlatformApplication> Create();

public:
    FWindowsApplication(HINSTANCE InInstance, HICON InIcon);
    virtual ~FWindowsApplication();

    // IPlatformApplication Interface
    virtual TSharedRef<IPlatformWindow> CreateWindow() override final;

    virtual void Tick(float Delta) override final;
    virtual void ProcessEvents() override final;
    virtual void ProcessDeferredEvents() override final;

    virtual void UpdateInputDevices() override final;
    virtual IPlatformInputDevice* GetInputDevice() override final;

    virtual bool SupportsHighPrecisionMouse() const override final;
    virtual bool SetHighPrecisionMouseMode(const TSharedRef<IPlatformWindow>& Window, EHighPrecisionMouseMode Mode) override final;
    virtual bool ConfineCursorToRect(const TSharedRef<IPlatformWindow>& Window, const IntVector2& Position, const IntVector2& Size) override final;
    virtual void ReleaseCursorConfinement() override final;

    virtual FModifierKeyState GetModifierKeyState() const override final;

    virtual void SetActiveWindow(const TSharedRef<IPlatformWindow>& Window) override final;
    virtual TSharedRef<IPlatformWindow> GetActiveWindow() const override final;
    virtual void SetCapture(const TSharedRef<IPlatformWindow>& Window) override final;
    virtual TSharedRef<IPlatformWindow> GetCapture() const override final;
    virtual TSharedRef<IPlatformWindow> GetWindowUnderCursor() const override final;

    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const override final;

    virtual void SetMessageHandler(const TSharedPtr<IPlatformApplicationMessageHandler>& InMessageHandler) override final;

    virtual TSharedPtr<IPlatformApplicationMessageHandler> GetMessageHandler() const override final
    {
        return MessageHandler;
    }

    virtual TSharedPtr<IPlatformCursor> GetCursor() const override final
    {
        return Cursor;
    }

    void DeferMessage(const FWindowsDeferredMessage& InDeferredMessage);

    void AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener);
    void RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener);
    bool IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener) const;

    TSharedRef<FWindowsWindow> GetWindowsWindowFromHWND(HWND Window) const;
    void CloseWindow(const TSharedRef<FWindowsWindow>& Window);

    HINSTANCE GetInstance() const
    {
        return InstanceHandle;
    }

private:
    static LRESULT WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    static BOOL    EnumerateMonitorsProc(HMONITOR Monitor, HDC DeviceContext, LPRECT ClipRect, LPARAM lParam);

    BOOL EnumerateMonitors(HMONITOR Monitor, HDC DeviceContext, LPRECT ClipRect, LPARAM Data);

    bool RegisterWindowClass();
    bool RegisterRawInputDevices(HWND Window);
    bool UnregisterRawInputDevices();

    LRESULT ProcessRawInput(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    LRESULT ProcessMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

    void ProcessDeferredMessage(const FWindowsDeferredMessage& Message);
    void ProcessWindowHoverMessage(const FWindowsDeferredMessage& Message);
    void ProcessWindowResizeMessage(const FWindowsDeferredMessage& Message);
    void ProcessWindowMoveMessage(const FWindowsDeferredMessage& Message);
    void ProcessKeyMessage(const FWindowsDeferredMessage& Message);
    void ProcessKeyCharMessage(const FWindowsDeferredMessage& Message);
    void ProcessMouseMoveMessage(const FWindowsDeferredMessage& Message);
    void ProcessMouseButtonMessage(const FWindowsDeferredMessage& Message);

    HICON                                          Icon;
    HINSTANCE                                      InstanceHandle;
    TSharedPtr<FXInputDevice>                      XInputDevice;
    bool                                           bIsTrackingMouse;
    bool                                           bDeferredMessagesEnabled;
    bool                                           bIsApplicationActive;
    TArray<FWindowsDeferredMessage>                Messages;
    FCriticalSection                               MessagesCS;
    TArray<TSharedPtr<IWindowsMessageListener>>    WindowsMessageListeners;
    mutable FCriticalSection                       WindowsMessageListenersCS;
    TArray<TSharedRef<FWindowsWindow>>             Windows;
    mutable FCriticalSection                       WindowsCS;
    TArray<TSharedRef<FWindowsWindow>>             ClosedWindows;
    FCriticalSection                               ClosedWindowsCS;
    TSharedPtr<IPlatformApplicationMessageHandler> MessageHandler;
    TSharedPtr<FWindowsCursor>                     Cursor;
};

extern COREAPPLICATION_API FWindowsApplication* GWindowsApplication;
