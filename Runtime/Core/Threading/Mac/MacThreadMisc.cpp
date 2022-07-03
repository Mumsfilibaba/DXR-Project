#include "MacThread.h"
#include "MacThreadMisc.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacThreadMisc

FGenericThread* FMacThreadMisc::CreateThread(const TFunction<void()>& InFunction)
{
    return FMacThread::CreateMacThread(InFunction);
}

FGenericThread* FMacThreadMisc::CreateNamedThread(const TFunction<void()>& InFunction, const String& InName)
{
    return FMacThread::CreateMacThread(InFunction, InName);
}
