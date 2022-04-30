#include "MacThread.h"
#include "MacThreadMisc.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacThreadMisc

CGenericThread* CMacThreadMisc::CreateThread(const TFunction<void()>& InFunction)
{
    return CMacThread::CreateMacThread(InFunction);
}

CGenericThread* CMacThreadMisc::CreateNamedThread(const TFunction<void()>& InFunction, const String& InName)
{
    return CMacThread::CreateMacThread(InFunction, InName);
}
