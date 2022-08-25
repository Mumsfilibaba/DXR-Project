#include "MacThread.h"
#include "MacThreadMisc.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacThreadMisc

FGenericThread* FMacThreadMisc::CreateThread(const FThreadFunction& InFunction)
{
    return new FMacThread(InFunction);
}

FGenericThread* FMacThreadMisc::CreateNamedThread(const FThreadFunction& InFunction, const FString& InName)
{
    return new FMacThread(InFunction, InName);
}
