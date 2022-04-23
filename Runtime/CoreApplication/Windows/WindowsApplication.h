#pragma once 

#if PLATFORM_WINDOWS
#include "WindowsWindow.h"
#include "WindowsCursor.h"
#include "IWindowsMessageListener.h"

#include "Core/Input/InputCodes.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Interface/PlatformApplication.h"

#define ENABLE_DPI_AWARENESS (0)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Struct used to store messages between calls to PumpMessages and CWindowsApplication::Tick */

struct SWindowsMessage
{
    FORCEINLINE SWindowsMessage(HWND InWindow, uint32 InMessage, WPARAM InwParam, LPARAM InlParam, int32 InMouseDeltaX, int32 InMouseDeltaY)
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
/* Class representing an application on the windows- platform */

class COREAPPLICATION_API CWindowsApplication final : public CPlatformApplication
{
private:
    
    friend class TDefaultDelete<CWindowsApplication>;
    
    CWindowsApplication(HINSTANCE InInstance);
    ~CWindowsApplication();

public:

    /* @return: Creates and returns an instance of the WindowsApplication, also loads the icon */
    static TSharedPtr<CWindowsApplication> CreateApplication();

    /* @return: Returns the window-class name */
    static const char* GetWindowClassName() { return "WindowClass"; }

    /* @return: Returns the HINSTANCE of the application or retrieves it in case the application is not initialized */
    static HINSTANCE GetStaticInstance()
    {
        return Instance ? Instance->GetInstance() : static_cast<HINSTANCE>(GetModuleHandle(0));
    }

    /* @return: Returns the instance of the windows application */
    static CWindowsApplication* Get() { return Instance; }

    /* @return: Returns true if the WindowsApplication has been created */
    static bool IsInitialized() { return (Instance != nullptr); }

    /* Searches all the created windows and return the one with the specified handle */
    TSharedRef<CWindowsWindow> GetWindowsWindowFromHWND(HWND Window) const;

    /* Stores messages for handling in the future */
    void StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);

    /* Add a native message listener */
    void AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener);

    /* Remove a native message listener */
    void RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener);

    /* Check if a native message listener is added */
    bool IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener) const;

    /* Returns the HINSTANCE of the application */
    HINSTANCE GetInstance() const { return InstanceHandle; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformApplication Interface

    virtual TSharedRef<CPlatformWindow> MakeWindow() override final;

    virtual bool Initialize() override final;

    virtual void Tick(float Delta) override final;

    virtual bool SupportsRawMouse() const;

    virtual bool EnableRawMouse(const TSharedRef<CPlatformWindow>& Window);

    virtual void SetCapture(const TSharedRef<CPlatformWindow>& Window) override final;
    virtual void SetActiveWindow(const TSharedRef<CPlatformWindow>& Window) override final;

    virtual TSharedRef<CPlatformWindow> GetCapture() const override final;
    virtual TSharedRef<CPlatformWindow> GetActiveWindow() const override final;
    virtual TSharedRef<CPlatformWindow> GetWindowUnderCursor() const override final;

private:
    static LRESULT StaticMessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

    bool RegisterWindowClass();
    bool RegisterRawInputDevices(HWND Window);

    // TODO: Investigate how to do this for a specific window
    bool UnregisterRawInputDevices();

    LRESULT ProcessRawInput(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

    void HandleStoredMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);

private:
    TArray<TSharedRef<CWindowsWindow>> Windows;

    TArray<SWindowsMessage>            Messages;
    CCriticalSection                   MessagesCriticalSection;

    TArray<TSharedPtr<IWindowsMessageListener>> WindowsMessageListeners;

    bool bIsTrackingMouse;

    HINSTANCE InstanceHandle;

    static CWindowsApplication* Instance;
};

#endif