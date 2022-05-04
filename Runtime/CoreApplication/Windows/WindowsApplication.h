#pragma once 
#include "WindowsWindow.h"
#include "WindowsCursor.h"

#include "Core/Input/InputCodes.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericApplication.h"

#define ENABLE_DPI_AWARENESS (0)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowsMessage

struct SWindowsMessage
{
    FORCEINLINE SWindowsMessage( HWND InWindow
                               , uint32 InMessage
                               , WPARAM InwParam
                               , LPARAM InlParam
                               , int32 InMouseDeltaX
                               , int32 InMouseDeltaY)
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

class IWindowsMessageListener
{
public:

    virtual ~IWindowsMessageListener() = default;

    /*
    * Handles messages sent from the application's MessageProc
    * See https://docs.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
    */
    virtual LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsApplication

class COREAPPLICATION_API CWindowsApplication final : public CGenericApplication
{
private:

    friend struct TDefaultDelete<CWindowsApplication>;

    CWindowsApplication(HINSTANCE InInstance);
    ~CWindowsApplication();

public:

    static CWindowsApplication* CreateWindowsApplication();

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericApplication Interface

    virtual TSharedRef<CGenericWindow> MakeWindow() override final;

    virtual bool Initialize() override final;

    virtual void Tick(float Delta) override final;

    virtual bool SupportsRawMouse() const;

    virtual bool EnableRawMouse(const TSharedRef<CGenericWindow>& Window);

    virtual void SetCapture(const TSharedRef<CGenericWindow>& Window) override final;

    virtual void SetActiveWindow(const TSharedRef<CGenericWindow>& Window) override final;

    virtual TSharedRef<CGenericWindow> GetCapture() const override final;

    virtual TSharedRef<CGenericWindow> GetActiveWindow() const override final;

    virtual TSharedRef<CGenericWindow> GetWindowUnderCursor() const override final;

public:

    TSharedRef<CWindowsWindow> GetWindowsWindowFromHWND(HWND Window) const;

    void StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);

    void AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener);

    void RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener);

    bool IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener) const;

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

    TArray<SWindowsMessage> Messages;
    CCriticalSection        MessagesCriticalSection;

    bool bIsTrackingMouse;
    
    HINSTANCE InstanceHandle;
    
    TArray<TSharedRef<CWindowsWindow>>          Windows;

    TArray<TSharedPtr<IWindowsMessageListener>> WindowsMessageListeners;
};

extern CWindowsApplication* WindowsApplication;