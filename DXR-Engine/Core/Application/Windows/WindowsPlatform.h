#pragma once
#include "Core/Input/InputCodes.h"
#include "Core/Application/Generic/GenericPlatform.h"
#include "Core/Application/Platform/PlatformCallbacks.h"
#include "Core/Containers/Array.h"

#include "WindowsWindow.h"

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

struct WindowHandle
{
    WindowHandle() = default;

    WindowHandle(HWND InHandle)
        : Handle(InHandle)
    {
    }

    WindowsWindow* GetWindow() const
    {
        WindowsWindow* Window = (WindowsWindow*)GetWindowLongPtrA(Handle, GWLP_USERDATA);
        if (Window)
        {
            Window->AddRef();
        }

        return Window;
    }

    HWND Handle;
};

class WindowsPlatform : public GenericPlatform
{
public:
    static void PreMainInit(HINSTANCE InInstance);

    static bool Init();

    static void Tick();

    static bool Release();

    static ModifierKeyState GetModifierKeyState();

    static void SetCapture(GenericWindow* Window);
    static void SetActiveWindow(GenericWindow* Window);

    static GenericWindow* GetCapture();
    static GenericWindow* GetActiveWindow();

    static void SetCursor(GenericCursor* Cursor);
    static GenericCursor* GetCursor();

    static void SetCursorPos(GenericWindow* RelativeWindow, int32 x, int32 y);
    static void GetCursorPos(GenericWindow* RelativeWindow, int32& OutX, int32& OutY);

    static LPCSTR GetWindowClassName() { return "WindowClass"; }
    static HINSTANCE GetInstance()     { return Instance; }
    
    static void StoreMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

private:
    static bool InitCursors();

    static bool RegisterWindowClass();
    
    static LRESULT MessageProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
    static void HandleStoredMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

private:
    static TArray<WindowsEvent> Messages;
    
    static TRef<WindowsCursor> CurrentCursor;

    static bool IsTrackingMouse;

    static HINSTANCE Instance;
};