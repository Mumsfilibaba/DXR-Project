#include "WindowsThread.h"
#include "WindowsThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsThreadMisc

CGenericThread* CWindowsThreadMisc::CreateThread(const TFunction<void()>& InFunction)
{
    return CWindowsThread::CreateWindowsThread(InFunction);
}

CGenericThread* CWindowsThreadMisc::CreateNamedThread(const TFunction<void()>& InFunction, const FString& InName)
{
    return CWindowsThread::CreateWindowsThread(InFunction, InName);
}