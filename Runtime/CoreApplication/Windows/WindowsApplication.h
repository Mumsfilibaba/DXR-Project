#pragma once 
#include "WindowsWindow.h"
#include "WindowsCursor.h"
#include "IWindowsMessageListener.h"

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
// CWindowsApplication

class COREAPPLICATION_API CWindowsApplication final : public CGenericApplication
{
private:

    friend struct TDefaultDelete<CWindowsApplication>;

    CWindowsApplication(HINSTANCE InInstance);
    ~CWindowsApplication();

public:

     /** 
      * @brief: Creates an instance of the WindowsApplication, also loads the icon 
      *
      * @return: Returns the newly created WindowsApplication
      */
    static TSharedPtr<CWindowsApplication> CreateApplication();

     /**
      * @return: Returns class name for the window-class used for a Window 
      */
    static FORCEINLINE const char* GetWindowClassName() {  return "WindowClass";  }

     /**
      * @return: Returns the HINSTANCE of the application or retrieves it in case the application is not initialized 
      */
    static FORCEINLINE HINSTANCE GetStaticInstance()
    {
        return Instance ? Instance->GetInstance() : static_cast<HINSTANCE>(GetModuleHandle(0));
    }

     /** 
      * @return: Returns the instance of the windows application 
      */
    static FORCEINLINE CWindowsApplication* Get() { return Instance; }

     /** 
      * @return: Returns true if the WindowsApplication has been created 
      */
    static FORCEINLINE bool IsInitialized() { return (Instance != nullptr); }

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericApplication Interface

    virtual TSharedRef<CGenericWindow> MakeWindow() override final;

    virtual bool Initialize()      override final;
    virtual void Tick(float Delta) override final;

    virtual bool SupportsRawMouse() const;
    virtual bool EnableRawMouse(const TSharedRef<CGenericWindow>& Window);

    virtual void SetCapture(const TSharedRef<CGenericWindow>& Window)      override final;
    virtual void SetActiveWindow(const TSharedRef<CGenericWindow>& Window) override final;

    virtual TSharedRef<CGenericWindow> GetCapture()           const override final;
    virtual TSharedRef<CGenericWindow> GetActiveWindow()      const override final;
    virtual TSharedRef<CGenericWindow> GetWindowUnderCursor() const override final;

public:

     /** 
      * @brief: Searches all the created windows and return the one with the specified handle 
      *
      * @param Window: Window handle 
      * @return: Returns the WindowsWindow that belongs to the specified handle
      */
    TSharedRef<CWindowsWindow> GetWindowsWindowFromHWND(HWND Window) const;

     /**
      * @brief: Store a message for handling during next call to Tick
      */
    void StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, int32 MouseDeltaX, int32 MouseDeltaY);

     /** 
      * @brief: Add a native message listener 
      *
      * @param NewWindowsMessageListener: New WindowsMessageListener to add
      */
    void AddWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener);

     /**
      * @brief: Remove a native message listener
      *
      * @param WindowsMessageListener: WindowsMessageListener to remove
      */
    void RemoveWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener);

     /** 
      * @brief: Check if a native message listener is added 
      *
      * @param WindowsMessageListener: WindowsMessageListener to check for
      * @return: Returns true if the WindowMessageListener is added
      */
    bool IsWindowsMessageListener(const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener) const;

     /** 
      * @return: Returns the HINSTANCE of the application 
      */
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

    TArray<TSharedRef<CWindowsWindow>>          Windows;
    TArray<TSharedPtr<IWindowsMessageListener>> WindowsMessageListeners;

    TArray<SWindowsMessage> Messages;
    CCriticalSection        MessagesCriticalSection;

    bool                    bIsTrackingMouse;
    HINSTANCE               InstanceHandle;

    static CWindowsApplication* Instance;
};
