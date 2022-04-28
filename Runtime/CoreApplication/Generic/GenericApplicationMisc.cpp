#include "GenericApplicationMisc.h"
#include "GenericApplication.h"
#include "GenericConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericApplicationMisc

class CGenericApplication* CGenericApplicationMisc::CreateApplication()
{
    return dbg_new CGenericApplication(TSharedPtr<ICursor>(nullptr));
}

class CGenericConsoleWindow* CGenericApplicationMisc::CreateConsoleWindow()
{
    return dbg_new CGenericConsoleWindow();
}