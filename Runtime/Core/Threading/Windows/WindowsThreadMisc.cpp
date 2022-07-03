#include "WindowsThread.h"
#include "WindowsThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThreadMisc

FGenericThread* FWindowsThreadMisc::CreateThread(const TFunction<void()>& InFunction)
{
    return FWindowsThread::CreateWindowsThread(InFunction);
}

FGenericThread* FWindowsThreadMisc::CreateNamedThread(const TFunction<void()>& InFunction, const FString& InName)
{
    return FWindowsThread::CreateWindowsThread(InFunction, InName);
}