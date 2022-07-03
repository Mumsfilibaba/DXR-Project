#include "WindowsApplication.h"
#include "WindowsApplicationMisc.h"
#include "WindowsConsoleWindow.h"

#include "Core/Input/ModifierKeyState.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsApplicationMisc

FGenericApplication* FWindowsApplicationMisc::CreateApplication()
{
    return FWindowsApplication::CreateWindowsApplication();
}

FGenericConsoleWindow* FWindowsApplicationMisc::CreateConsoleWindow()
{
    return FWindowsConsoleWindow::CreateWindowsConsole();
}

void FWindowsApplicationMisc::PumpMessages(bool bUntilEmpty)
{
    MSG Message;

    do
    {
        BOOL Result = PeekMessage(&Message, 0, 0, 0, PM_REMOVE);
        if (!Result)
        {
            break;
        }

        TranslateMessage(&Message);
        DispatchMessage(&Message);

        if (Message.message == WM_QUIT)
        {
            if (WindowsApplication)
            {
                WindowsApplication->StoreMessage(Message.hwnd, Message.message, Message.wParam, Message.lParam, 0, 0);
            }
        }

    } while (bUntilEmpty);
}

FModifierKeyState FWindowsApplicationMisc::GetModifierKeyState()
{
    uint8 ModifierMask = 0;
    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Ctrl;
    }

    if (GetKeyState(VK_MENU) & 0x8000)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Alt;
    }

    if (GetKeyState(VK_SHIFT) & 0x8000)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Shift;
    }

    if (GetKeyState(VK_CAPITAL) & 1)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_CapsLock;
    }

    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Super;
    }

    if (GetKeyState(VK_NUMLOCK) & 1)
    {
        ModifierMask |= EModifierFlag::ModifierFlag_NumLock;
    }

    return FModifierKeyState(ModifierMask);
}
