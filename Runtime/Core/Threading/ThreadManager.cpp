#include "ThreadManager.h"

#include "Core/Platform/PlatformThreadMisc.h"

FThreadManager::FThreadManager()
    : Threads(0)
    , MainThread(nullptr)
{
}

FThreadManager::~FThreadManager()
{
    for (const auto& Thread : Threads)
    {
        Thread->WaitForCompletion();
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

    return true;
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

TSharedRef<FGenericThread> FThreadManager::CreateThread(FThreadInterface* InRunnable)
{
    TSharedRef<FGenericThread> Thread = FPlatformThreadMisc::CreateThread(InRunnable);
    if (Thread)
    {
        Threads.Add(Thread);
        return Thread;
    }
    else
    {
        LOG_ERROR("Failed to create thread");
        return nullptr;
    }
}

TSharedRef<FGenericThread> FThreadManager::GetThreadFromHandle(void* ThreadHandle)
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
