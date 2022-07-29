#include "GenericApplicationMisc.h"
#include "GenericApplication.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGenericApplicationMisc

class FGenericApplication* FGenericApplicationMisc::CreateApplication()
{
    return dbg_new FGenericApplication(TSharedPtr<ICursor>(nullptr));
}