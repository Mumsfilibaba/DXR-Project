#include "Core/Threading/ThreadManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceLogger.h"

FThreadManager::FThreadManager()
    : MainThreadHandle(nullptr)
    , Threads(0)
    , ThreadsCS()
{
}

FThreadManager::~FThreadManager()
{
    Threads.Clear();
    MainThreadHandle = nullptr;
}

static TOptional<FThreadManager>& GetThreadManagerInstance()
{
    static TOptional<FThreadManager> StaticThreadManager(EInPlace::InPlace);
    return StaticThreadManager;
}

bool FThreadManager::Initialize()
{
    FThreadManager& ThreadManager = FThreadManager::Get();
    ThreadManager.MainThreadHandle = FPlatformThreadMisc::GetCurrentThreadHandle();

    if (!ThreadManager.MainThreadHandle)
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
    return ThreadManager.MainThreadHandle == FPlatformThreadMisc::GetCurrentThreadHandle();
}

FThreadManager& FThreadManager::Get()
{
    TOptional<FThreadManager>& ThreadManager = GetThreadManagerInstance();
    return ThreadManager.GetValue();
}

void FThreadManager::RegisterThread(FGenericPlatformThread* InThread)
{
    TScopedLock Lock(ThreadsCS);
    Threads.AddUnique(InThread);
}
void FThreadManager::UnregisterThread(FGenericPlatformThread* InThread)
{
    TScopedLock Lock(ThreadsCS);
    Threads.Remove(InThread);
}

FGenericPlatformThread* FThreadManager::GetThreadFromHandle(void* ThreadHandle)
{
    TScopedLock Lock(ThreadsCS);

    for (FGenericPlatformThread* Thread : Threads)
    {
        if (Thread->GetPlatformHandle() == ThreadHandle)
        {
            return Thread;
        }
    }

    LOG_WARNING("No thread registered with the handle '%llu'", ThreadHandle);
    return nullptr;
}
