#include "CoreApplication/Windows/WindowsApplication.h"
#include "CoreApplication/Windows/WindowsApplicationMisc.h"
#include "CoreApplication/Windows/WindowsOutputDeviceConsole.h"
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
            if (GWindowsApplication)
            {
                GWindowsApplication->StoreMessage(Message.hwnd, Message.message, Message.wParam, Message.lParam, 0, 0);
            }
        }

    } while (bUntilEmpty);
}
