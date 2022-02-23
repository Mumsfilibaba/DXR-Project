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
// SWindowsMessage

struct SWindowsMessage
{
    FORCEINLINE SWindowsMessage(HWND InWindow, uint32 InMessage, WPARAM InwParam, LPARAM InlParam, int32 InMouseDeltaX, int32 InMouseDeltaY)
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsApplication - Class representing an application on the windows- platform

class COREAPPLICATION_API CWindowsApplication final : public CPlatformApplication
{
public:

    /**
     * Retrieve the window-class name 
     * 
     * @return: Returns a string with the window-class name
     */
    static FORCEINLINE const char* GetWindowClassName() { return "WindowClass"; }

    /**
     * Retrieve the WindowsApplication Instance
     * 
     * @return: Returns a reference to the WindowsApplication
     */
    static FORCEINLINE CWindowsApplication& Get() 
    {
        Assert(IsInitialized());
        return *Instance; 
    }

    /**
     * Check if the WindowsApplication is initialized or not
     * 
     * @return: Returns true if the WindowsApplication is initialized
     */
    static FORCEINLINE bool IsInitialized() 
    { 
        return (Instance != nullptr);
    }

    /**
     * Searches all the created windows and return the one with the specified handle 
     * 
     * @param Window: Window-handle to search for
     * @return: Returns the window with the specified handle
     */
    TSharedRef<CWindowsWindow> GetWindowsWindowFromHWND(HWND Window) const;

    /**
     * Add a native message listener 
     * 
     * @param NewWindowsMessageListener: Message listener to add
     */
    void AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener);


    /**
     * Remove a native message listener
     *
     * @param WindowsMessageListener: Message listener to remove
     */
    void RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener);

    /**
     * Check if a native message listener is added 
     *
     * @param WindowsMessageListener: Message listener to check for
     */
    bool IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener) const;

    /**
     * Retrieve the HINSTANCE of the application 
     * 
     * @return: Returns the HINSTANCE of the application 
     */
    FORCEINLINE HINSTANCE GetInstance() const 
    { 
        return InstanceHandle;
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformApplication - Interface
    
    static TSharedPtr<CWindowsApplication> CreateApplication();

    virtual TSharedRef<CPlatformWindow> CreateWindow() override final;

    virtual bool Initialize() override final;

    virtual void Tick(float Delta) override final;

    virtual bool SupportsHighPrecisionMouse() const override final;
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<CPlatformWindow>& Window) override final;

    virtual void SetCapture(const TSharedRef<CPlatformWindow>& Window) override final;
    virtual void SetActiveWindow(const TSharedRef<CPlatformWindow>& Window) override final;

    virtual TSharedRef<CPlatformWindow> GetCapture() const override final;
    virtual TSharedRef<CPlatformWindow> GetActiveWindow() const override final;
    virtual TSharedRef<CPlatformWindow> GetWindowUnderCursor() const override final;

private:

    friend struct TDefaultDelete<CWindowsApplication>;
    friend class CWindowsApplicationMisc;

    CWindowsApplication(HINSTANCE InInstance);
    ~CWindowsApplication();

    static LRESULT StaticMessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

    bool RegisterWindowClass();
    bool RegisterRawInputDevices(HWND Window);
    bool UnregisterRawInputDevices();

    LRESULT ProcessRawInput(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

    void HandleStoredMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);
    void StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);

private:
    TArray<TSharedRef<CWindowsWindow>> Windows;
    
    // Messages that are waiting to be processed
    TArray<SWindowsMessage> Messages;
    CCriticalSection        MessagesCriticalSection;

    TArray<TSharedPtr<IWindowsMessageListener>> WindowsMessageListeners;

    bool bIsTrackingMouse;

    HINSTANCE InstanceHandle;

    static CWindowsApplication* Instance;
};

#endif