#include "ThreadManager.h"

#include "Core/Platform/PlatformThreadMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FThreadManager

FThreadManager::FThreadManager()
    : Threads(0)
    , MainThread(nullptr)
{ }

FThreadManager::~FThreadManager()
{
    for (const auto& Thread : Threads)
    {
        Thread->WaitForCompletion(FTimespan::Infinity());
    }

    Threads.Clear();

    MainThread = nullptr;
}

static auto& GetThreadManagerInstance()
{
    static TOptional<FThreadManager> Instance(InPlace);
    return Instance;
}

bool FThreadManager::Initialize()
{
    FThreadManager& ThreadManager = FThreadManager::Get();
    ThreadManager.MainThread = FPlatformThreadMisc::GetThreadHandle();

    if (!ThreadManager.MainThread)
    {
        LOG_ERROR("Failed to retrieve the mainthread handle");
        return false;
    }

    return FPlatformThreadMisc::Initialize();
}

bool FThreadManager::Release()
{
    auto& ThreadManager = GetThreadManagerInstance();
    ThreadManager.Reset();

    FPlatformThreadMisc::Release();
    return true;
}

bool FThreadManager::IsMainThread()
{
    FThreadManager& ThreadManager = FThreadManager::Get();
    return (ThreadManager.MainThread == FPlatformThreadMisc::GetThreadHandle());
}

FThreadManager& FThreadManager::Get()
{
    TOptional<FThreadManager>& ThreadManager = GetThreadManagerInstance();
    return ThreadManager.GetValue();
}

FGenericThreadRef FThreadManager::CreateThread(const TFunction<void()>& InFunction)
{
    FGenericThreadRef Thread = FPlatformThreadMisc::CreateThread(InFunction);
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

FGenericThreadRef FThreadManager::CreateNamedThread(const TFunction<void()>& InFunction, const FString& InName)
{
    FGenericThreadRef Thread = FPlatformThreadMisc::CreateNamedThread(InFunction, InName);
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

FGenericThreadRef FThreadManager::GetNamedThread(const FString& InName)
{
    for (const auto& Thread : Threads)
    {
        if (Thread->GetName() == InName)
        {
            return Thread;
        }
    }

    LOG_WARNING("No thread with the name '%s'", InName.GetCString());
    return nullptr;
}

FGenericThreadRef FThreadManager::GetThreadFromHandle(void* ThreadHandle)
{
    for (const auto& Thread : Threads)
    {
        if (Thread->GetPlatformHandle() == ThreadHandle)
        {
            return Thread;
        }
    }

    LOG_WARNING("No thread with the handle '%llu'", ThreadHandle);
    return nullptr;
}