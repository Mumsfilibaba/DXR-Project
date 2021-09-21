#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Input/InputCodes.h"
#include "Core/Application/Platform/PlatformCallbacks.h"
#include "Core/Containers/Array.h"

#include "WindowsWindow.h"



class WindowsPlatform : public GenericPlatform
{
public:
    static void PreMainInit( HINSTANCE InInstance );

    static bool Init();

    static void Tick();

    static bool Release();

    static ModifierKeyState GetModifierKeyState();

    static void SetCapture( CGenericWindow* Window );
    static void SetActiveWindow( CGenericWindow* Window );

    static CGenericWindow* GetCapture();
    static CGenericWindow* GetActiveWindow();

private:
    static bool InitCursors();

    static bool RegisterWindowClass();

    static void StoreMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );

    static LRESULT MessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );
    static void HandleStoredMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );

private:
    static TArray<WindowsEvent> Messages;

    static TSharedRef<WindowsCursor> CurrentCursor;

    static bool IsTrackingMouse;

    static HINSTANCE Instance;
};

#endif