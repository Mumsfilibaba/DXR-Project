#include "GenericApplicationMisc.h"
#include "GenericApplication.h"

class FGenericApplication* FGenericApplicationMisc::CreateApplication()
{
    return new FGenericApplication(TSharedPtr<ICursor>(nullptr));
}