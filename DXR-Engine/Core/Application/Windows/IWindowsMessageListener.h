#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Windows/Windows.h"

class IWindowsMessageListener
{
public:

	/*
    * Handles messages sent from the application's messageproc
    * See https://docs.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
    */
    virtual LRESULT MessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam ) = 0;

protected:
    ~IWindowsMessageListener() = default;
};

#endif