#pragma once
#include "Core/Application/InputCodes.h"
#include "Core/Application/Generic/GenericApplication.h"
#include "Core/Application/Generic/GenericApplicationEventHandler.h"
#include "Core/Containers/Array.h"

#include "WindowsWindow.h"
#include "WindowsCursor.h"

class WindowsWindow;
class GenericApplicationDelegate;
class WindowsCursor;

struct WindowsEvent
{
    WindowsEvent(HWND InWindow, uint32 InMessage, WPARAM InwParam, LPARAM InlParam)
        : Window(InWindow)
        , Message(InMessage)
        , wParam(InwParam)
        , lParam(InlParam)
    {
    }

    HWND   Window;
    uint32 Message;
    WPARAM wParam; 
    LPARAM lParam;
};

class WindowsApplication : public GenericApplication
{
public:
    WindowsApplication();
    ~WindowsApplication();

    WindowsWindow* GetWindowFromHWND(HWND Window) const;

    virtual GenericWindow* CreateWindow(const std::string& Title, uint32 Width, uint32 Height, WindowStyle Style) override final;

    virtual bool Init() override final;
    virtual void Tick() override final;
    virtual void Release() override final;

    virtual void SetActiveWindow(GenericWindow* Window) override final;
    virtual void SetCapture(GenericWindow* Window) override final;

    virtual GenericWindow* GetActiveWindow() const override final;
    virtual GenericWindow* GetCapture() const override final;
    
    virtual ModifierKeyState GetModifierKeyState() override final;

    static void PreMainInit(HINSTANCE InInstance);

    static LPCSTR GetWindowClassName() { return "WindowClass"; }
    static HINSTANCE GetInstance() { return Instance; }

    static GenericApplication& Get();

private:
    bool RegisterWindowClass();
    
    void AddWindow(WindowsWindow* Window);

    static LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    LRESULT ApplicationProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

    void StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    void HandleStoredMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

private:
    TArray<TRef<WindowsWindow>> Windows;
    TArray<WindowsEvent> Messages;

    bool IsTrackingMouse;

    static HINSTANCE Instance;
};

extern WindowsApplication GWindowsApplication;