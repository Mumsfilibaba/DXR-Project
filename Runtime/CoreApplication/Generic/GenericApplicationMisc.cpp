#include "GenericApplicationMisc.h"
#include "GenericApplication.h"
#include "GenericConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericApplicationMisc

class FGenericApplication* FGenericApplicationMisc::CreateApplication()
{
    return dbg_new FGenericApplication(TSharedPtr<ICursor>(nullptr));
}

class FGenericConsoleWindow* FGenericApplicationMisc::CreateConsoleWindow()
{
    return dbg_new FGenericConsoleWindow();
}