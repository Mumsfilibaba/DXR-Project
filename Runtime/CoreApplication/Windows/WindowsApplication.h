#pragma once 
#include "WindowsWindow.h"
#include "WindowsCursor.h"

#include "Core/Input/InputCodes.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericApplication.h"

#define ENABLE_DPI_AWARENESS (1)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsMessage

struct FWindowsMessage
{
    FORCEINLINE FWindowsMessage(
        HWND InWindow,
        uint32 InMessage,
        WPARAM InwParam,
        LPARAM InlParam,
        int32 InMouseDeltaX,
        int32 InMouseDeltaY)
        : Window(InWindow)
        , Message(InMessage)
        , wParam(InwParam)
        , lParam(InlParam)
        , MouseDeltaX(InMouseDeltaX)
        , MouseDeltaY(InMouseDeltaY)
    { }

    // Standard Window Message
    HWND   Window;
    uint32 Message;
    WPARAM wParam;
    LPARAM lParam;

    // Used for Raw Mouse Input
    int32 MouseDeltaX;
    int32 MouseDeltaY;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IWindowsMessageListener

struct IWindowsMessageListener
{
    virtual ~IWindowsMessageListener() = default;

    /*
    * Handles messages sent from the application's MessageProc
    * See https://docs.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
    */
    virtual LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsApplication

class COREAPPLICATION_API FWindowsApplication final 
    : public FGenericApplication
{
private:
    friend struct TDefaultDelete<FWindowsApplication>;

    FWindowsApplication(HINSTANCE InInstance);
    ~FWindowsApplication();

public:
    static FWindowsApplication* CreateWindowsApplication();

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericApplication Interface

    virtual FGenericWindowRef CreateWindow() override final;

    virtual void Tick(float Delta) override final;

	virtual bool SupportsHighPrecisionMouse() const override final { return false; }

    virtual bool EnableHighPrecisionMouseForWindow(const FGenericWindowRef& Window) override final;

    virtual void SetCapture(const FGenericWindowRef& Window)      override final;
    virtual void SetActiveWindow(const FGenericWindowRef& Window) override final;

    virtual FGenericWindowRef GetWindowUnderCursor() const override final;
    virtual FGenericWindowRef GetCapture()           const override final;
    virtual FGenericWindowRef GetActiveWindow()      const override final;

public:
    FWindowsWindowRef GetWindowsWindowFromHWND(HWND Window) const;

    void StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);

    void AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener);
    void RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener);

    bool IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener) const;

    void CloseWindow(const FWindowsWindowRef& Window); 

    FORCEINLINE HINSTANCE GetInstance() const 
    { 
        return InstanceHandle;
    }

private:
    static LRESULT StaticMessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

    bool RegisterWindowClass();
    bool RegisterRawInputDevices(HWND Window);
    bool UnregisterRawInputDevices();

    LRESULT ProcessRawInput(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

    void HandleStoredMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);

private:
    TArray<FWindowsMessage>   Messages;
    FCriticalSection          MessagesCS;

    TArray<FWindowsWindowRef> Windows;
    mutable FCriticalSection  WindowsCS;

    TArray<FWindowsWindowRef> ClosedWindows;
    FCriticalSection          ClosedWindowsCS;

    HINSTANCE                 InstanceHandle;
    bool                      bIsTrackingMouse;

    TArray<TSharedPtr<IWindowsMessageListener>> WindowsMessageListeners;
    mutable FCriticalSection                    WindowsMessageListenersCS;
};

extern FWindowsApplication* WindowsApplication;