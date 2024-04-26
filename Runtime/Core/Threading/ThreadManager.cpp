#include "ThreadManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"

FThreadManager::FThreadManager()
    : Threads(0)
    , MainThreadHandle(nullptr)
{
}

FThreadManager::~FThreadManager()
{
    Threads.Clear();
    MainThreadHandle = nullptr;
}

static auto& GetThreadManagerInstance()
{
    static TOptional<FThreadManager> Instance(InPlace);
    return Instance;
}

bool FThreadManager::Initialize()
{
    FThreadManager& ThreadManager = FThreadManager::Get();
    ThreadManager.MainThreadHandle = FPlatformThreadMisc::GetThreadHandle();

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
    return ThreadManager.MainThreadHandle == FPlatformThreadMisc::GetThreadHandle();
}

FThreadManager& FThreadManager::Get()
{
    TOptional<FThreadManager>& ThreadManager = GetThreadManagerInstance();
    return ThreadManager.GetValue();
}

void FThreadManager::RegisterThread(FGenericThread* InThread)
{
    TScopedLock Lock(ThreadsCS);
    Threads.AddUnique(InThread);
}
void FThreadManager::UnregisterThread(FGenericThread* InThread)
{
    TScopedLock Lock(ThreadsCS);
    Threads.Remove(InThread);
}

FGenericThread* FThreadManager::GetThreadFromHandle(void* ThreadHandle)
{
    TScopedLock Lock(ThreadsCS);

    for (FGenericThread* Thread : Threads)
    {
        if (Thread->GetPlatformHandle() == ThreadHandle)
        {
            return Thread;
        }
    }

    LOG_WARNING("No thread registered with the handle '%llu'", ThreadHandle);
    return nullptr;
}
