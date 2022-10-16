#include "GenericApplicationMisc.h"
#include "GenericApplication.h"

class FGenericApplication* FGenericApplicationMisc::CreateApplication()
{
    return dbg_new FGenericApplication(TSharedPtr<ICursor>(nullptr));
}