#include "GenericApplicationMisc.h"
#include "GenericApplication.h"

TSharedPtr<class FGenericApplication> FGenericApplicationMisc::CreateApplication()
{
    return MakeShared<FGenericApplication>(TSharedPtr<ICursor>(nullptr));
}