#pragma once

#if PLATFORM_WINDOWS
#include "Core/Windows/Windows.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Interface for handling windows messages before they get handled by the NativeApplication

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

#endif