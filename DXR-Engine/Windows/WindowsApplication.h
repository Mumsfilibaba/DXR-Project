#pragma once
#include "Application/InputCodes.h"
#include "Application/Generic/GenericApplication.h"
#include "Application/Generic/GenericApplicationEventHandler.h"

#include "Windows.h"

class WindowsWindow;
class GenericApplicationDelegate;
class WindowsCursor;

struct WindowsEvent
{
    WindowsEvent(HWND InHwnd, UInt32 InMessage, WPARAM InwParam, LPARAM InlParam)
        : Hwnd(InHwnd)
        , Message(InMessage)
        , wParam(InwParam)
        , lParam(InlParam)
    {
    }

    HWND   Hwnd;
    UInt32 Message;
    WPARAM wParam; 
    LPARAM lParam;
};

class WindowsApplication : public GenericApplication
{
    WindowsApplication(HINSTANCE hInstance);

    void AddWindow(WindowsWindow* Window);

    LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

public:
    ~WindowsApplication();

    WindowsWindow* GetWindowFromHWND(HWND Window) const;

    HINSTANCE GetInstance() const { return InstanceHandle; }

    // GenericApplication Interface
    virtual GenericWindow* MakeWindow() override final;
    virtual GenericCursor* MakeCursor() override final;

    virtual Bool Init() override final;
    virtual void Tick() override final;

    virtual void SetCursor(GenericCursor* Cursor)       override final;
    virtual void SetActiveWindow(GenericWindow* Window) override final;
    virtual void SetCapture(GenericWindow* Window)      override final;

    virtual GenericWindow* GetActiveWindow() const override final;
    virtual GenericWindow* GetCapture()      const override final;
    virtual GenericCursor* GetCursor()       const override final;
    
    virtual void SetCursorPos(GenericWindow* RelativeWindow, Int32 x, Int32 y) override final;    
    virtual void GetCursorPos(GenericWindow* RelativeWindow, Int32& OutX, Int32& OutY) const override final;
    
    static Bool PeekMessageUntilNoMessage();
    static ModifierKeyState GetModifierKeyState();

    static GenericApplication* Make();

private:
    HINSTANCE InstanceHandle = 0;

    TRef<WindowsCursor> CurrentCursor;
    TArray<TRef<WindowsWindow>> Windows;
    TArray<WindowsEvent> Events;

    Bool IsTrackingMouse = false;
};

extern WindowsApplication* GlobalWindowsApplication;