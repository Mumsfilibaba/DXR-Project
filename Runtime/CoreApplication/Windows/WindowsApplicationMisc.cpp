#include "WindowsApplication.h"
#include "WindowsApplicationMisc.h"
#include "WindowsOutputDeviceConsole.h"
#include "CoreApplication/Generic/GenericApplicationMisc.h"

FOutputDeviceConsole* FWindowsApplicationMisc::CreateOutputDeviceConsole()
{
    return new FWindowsOutputDeviceConsole();
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
    EModifierFlag ModifierFlags = EModifierFlag::None;
    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Ctrl;
    }
    if (GetKeyState(VK_MENU) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Alt;
    }
    if (GetKeyState(VK_SHIFT) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Shift;
    }
    if (GetKeyState(VK_CAPITAL) & 0x1)
    {
        ModifierFlags |= EModifierFlag::CapsLock;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
    {
        ModifierFlags |= EModifierFlag::Super;
    }
    if (GetKeyState(VK_NUMLOCK) & 0x1)
    {
        ModifierFlags |= EModifierFlag::NumLock;
    }

    return FModifierKeyState(ModifierFlags);
}
