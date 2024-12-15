#include "CoreApplication/Windows/WindowsApplication.h"
#include "CoreApplication/Windows/WindowsApplicationMisc.h"

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
    } while (bUntilEmpty);
}
