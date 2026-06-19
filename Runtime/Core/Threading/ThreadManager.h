#pragma once
#include "Core/Generic/GenericPlatformThread.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Optional.h"
#include "Core/Platform/CriticalSection.h"

class CORE_API FThreadManager
{
public:
    static FThreadManager& Get();

    static bool Initialize();
    static bool Release();

    static bool IsMainThread();

public:

    // Register a thread this is called from the constructor of the FGenericPlatformThread
    void RegisterThread(FGenericPlatformThread* InThread);

    // Unregister a thread, this is called from the destructor of the FGenericPlatformThread
    void UnregisterThread(FGenericPlatformThread* InThread);

    // Retrieve a ThreadObject from a native ThreadHandle
    FGenericPlatformThread* GetThreadFromHandle(void* ThreadHandle);

    // Check if the thread-handle is for the main-thread
    bool IsMainThread(void* ThreadHandle) const 
    {
        return MainThreadHandle == ThreadHandle;
    }

private:
    friend class TOptional<FThreadManager>;
    
    FThreadManager();
    ~FThreadManager();

    void*                           MainThreadHandle;
    TArray<FGenericPlatformThread*> Threads;
    FCriticalSection                ThreadsCS;
};
