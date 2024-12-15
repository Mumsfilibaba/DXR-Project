#pragma once 
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Windows/Windows.h"
#include "CoreApplication/Windows/XInputDevice.h"
#include "CoreApplication/Windows/WindowsWindow.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericApplication.h"

/**
 * @struct FWindowsDeferredMessage
 * @brief Holds data about a Windows message that is deferred for later processing.
 *
 * On Windows, certain functions and user actions can trigger messages that are posted
 * immediately to the window procedure. By deferring them, we process all messages at once
 * in a controlled manner (e.g., in the application's Tick function), ensuring we handle
 * events consistently and avoid reentrant calls into the window procedure.
 */

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
    FIntVector2 MouseDelta;
};

/**
 * @struct IWindowsMessageListener
 * @brief An interface for classes that want to listen to Windows messages directly.
 *
 * Any class implementing this interface can be registered with FWindowsApplication
 * to receive and process Windows messages before they're handled by the default system.
 */

struct IWindowsMessageListener
{
    virtual ~IWindowsMessageListener() = default;

    /**
     * @brief Handles messages sent from the application's MessageProc.
     *
     * See https://docs.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
     *
     * @param Window The HWND that received the message.
     * @param Message The message type (WM_XXX).
     * @param wParam Additional message data (meaning depends on the message type).
     * @param lParam Additional message data (meaning depends on the message type).
     * @return A result code determined by the message type and the handler.
     */
    virtual LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) = 0;
};

/**
 * @class FWindowsApplication
 * @brief The Windows-specific implementation of the generic application interface.
 *
 * FWindowsApplication handles platform-specific initialization, message processing,
 * event deferral, window management, and input device updates on Windows.
 * It integrates closely with the Win32 API, implementing a static window procedure to dispatch messages.
 *
 * By deferring messages (FWindowsDeferredMessage), it allows all events to be handled
 * in a single pipeline, reducing complexity from immediate and reentrant window procedure calls.
 */

class COREAPPLICATION_API FWindowsApplication final : public FGenericApplication
{
public:

    /**
     * @brief Creates a FWindowsApplication and returns it as a FGenericApplication interface.
     *
     * This function also assigns the global pointer GWindowsApplication inside the constructor
     * of FWindowsApplication. That pointer is reset to null in the destructor.
     *
     * @return A shared pointer to the newly created FGenericApplication instance.
     */
    static TSharedPtr<FGenericApplication> Create();

public:

    FWindowsApplication(HINSTANCE InInstance, HICON InIcon);
    virtual ~FWindowsApplication();

public:

    // FGenericApplication Interface
    virtual TSharedRef<FGenericWindow> CreateWindow() override final;

    virtual void Tick(float Delta) override final;

    virtual void ProcessEvents() override final;

    virtual void ProcessDeferredEvents() override final;

    virtual void UpdateInputDevices() override final;

    virtual FInputDevice* GetInputDevice() override final;

    virtual bool SupportsHighPrecisionMouse() const override final;

    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) override final;

    virtual FModifierKeyState GetModifierKeyState() const override final;

    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) override final;

    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) override final;

    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const override final;

    virtual TSharedRef<FGenericWindow> GetCapture() const override final;

    virtual TSharedRef<FGenericWindow> GetActiveWindow() const override final;

    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const override final;

    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler) override final;

public:

    /**
     * @brief Retrieves the engine-level FWindowsWindow corresponding to the given HWND.
     *
     * @param Window The native HWND for which to find the corresponding FWindowsWindow.
     * @return A shared reference to the matching FWindowsWindow.
     */
    TSharedRef<FWindowsWindow> GetWindowsWindowFromHWND(HWND Window) const;

    /**
     * @brief Defers a Windows message for later processing in Tick, rather than handling it immediately.
     *
     * @param Window The HWND receiving this message.
     * @param Message The Windows message (WM_XXX).
     * @param wParam  The wParam data for this message.
     * @param lParam  The lParam data for this message.
     * @param MouseDeltaX Horizontal mouse delta if this is a raw input message.
     * @param MouseDeltaY Vertical mouse delta if this is a raw input message.
     */
    void DeferMessage(const FWindowsDeferredMessage& InDeferredMessage);

    /**
     * @brief Registers a new listener to receive Windows messages before or after the default handler.
     *
     * @param NewWindowsMessageListener The listener object to register.
     */
    void AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener);

    /**
     * @brief Unregisters a previously-added Windows message listener.
     *
     * @param WindowsMessageListener The listener object to remove.
     */
    void RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener);

    /**
     * @brief Checks if a given listener is already registered for Windows messages.
     *
     * @param WindowsMessageListener The listener to check.
     * @return True if found, false otherwise.
     */
    bool IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener) const;

    /**
     * @brief Closes the specified window, initiating cleanup in the engine.
     *
     * @param Window The FWindowsWindow to close.
     */
    void CloseWindow(const TSharedRef<FWindowsWindow>& Window);

    /**
     * @brief Retrieves the HINSTANCE (Windows instance handle) of this application.
     *
     * @return The HINSTANCE this application is using.
     */
    HINSTANCE GetInstance() const
    {
        return InstanceHandle;
    }

private:
    static LRESULT WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    static BOOL EnumerateMonitorsProc(HMONITOR Monitor, HDC DeviceContext, LPRECT ClipRect, LPARAM lParam);

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

    HICON         Icon;
    HINSTANCE     InstanceHandle;
    FXInputDevice XInputDevice;

    bool bIsTrackingMouse;
    bool bDeferredMessagesEnabled;

    TArray<FWindowsDeferredMessage>             Messages;
    FCriticalSection                            MessagesCS;
    TArray<TSharedPtr<IWindowsMessageListener>> WindowsMessageListeners;
    mutable FCriticalSection                    WindowsMessageListenersCS;
    TArray<TSharedRef<FWindowsWindow>>          Windows;
    mutable FCriticalSection                    WindowsCS;
    TArray<TSharedRef<FWindowsWindow>>          ClosedWindows;
    FCriticalSection                            ClosedWindowsCS;
};

extern COREAPPLICATION_API FWindowsApplication* GWindowsApplication;
