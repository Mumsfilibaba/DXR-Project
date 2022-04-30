#include "ThreadManager.h"

#include "Platform/PlatformThreadMisc.h"

#include "Core/Logging/Log.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CThreadManager

CThreadManager::CThreadManager()
    : Threads(0)
    , MainThread(nullptr)
{ }

CThreadManager::~CThreadManager()
{
    MainThread = nullptr;
}

TOptional<CThreadManager>& CThreadManager::GetConsoleManagerInstance()
{
    static TOptional<CThreadManager> Instance(InPlace);
    return Instance;
}

bool CThreadManager::Initialize()
{
    CThreadManager& ThreadManager = CThreadManager::Get();
    ThreadManager.MainThread = PlatformThreadMisc::GetThreadHandle();

    if (!ThreadManager.MainThread)
    {
        LOG_ERROR("Failed to retrieve the mainthread handle");
        return false;
    }

    return PlatformThreadMisc::Initialize();
}

bool CThreadManager::Release()
{
    TOptional<CThreadManager>& ThreadManager = GetConsoleManagerInstance();
    for (const auto& Thread : ThreadManager->Threads)
    {
        Thread->WaitUntilFinished(kWaitForThreadInfinity);
    }

    ThreadManager.Reset();
    return true;
}

bool CThreadManager::IsMainThread()
{
    CThreadManager& ThreadManager = CThreadManager::Get();
    return (ThreadManager.MainThread == PlatformThreadMisc::GetThreadHandle());
}

CThreadManager& CThreadManager::Get()
{
    TOptional<CThreadManager>& ThreadManager = GetConsoleManagerInstance();
    return ThreadManager.GetValue();
}

GenericThreadRef CThreadManager::CreateThread(const TFunction<void()>& InFunction)
{
    GenericThreadRef Thread = PlatformThreadMisc::CreateThread(InFunction);
    if (Thread)
    {
        Threads.Push(Thread);
        return Thread;
    }
    else
    {
        LOG_ERROR("Failed to create thread");
        return nullptr;
    }
}

GenericThreadRef CThreadManager::CreateNamedThread(const TFunction<void()>& InFunction, const String& InName)
{
    GenericThreadRef Thread = PlatformThreadMisc::CreateNamedThread(InFunction, InName);
    if (Thread)
    {
        Threads.Push(Thread);
        return Thread;
    }
    else
    {
        LOG_ERROR("Failed to create thread");
        return nullptr;
    }
}

GenericThreadRef CThreadManager::GetNamedThread(const String& InName)
{
    for (const auto& Thread : Threads)
    {
        if (Thread->GetName() == InName)
        {
            return Thread;
        }
    }

    LOG_WARNING("No thread with the name '" + InName + "'");
    return nullptr;
}

GenericThreadRef CThreadManager::GetThreadFromHandle(void* ThreadHandle)
{
    for (const auto& Thread : Threads)
    {
        if (Thread->GetPlatformHandle() == ThreadHandle)
        {
            return Thread;
        }
    }

    LOG_WARNING("No thread with the handle '" + ToString(ThreadHandle) + "'");
    return nullptr;
}