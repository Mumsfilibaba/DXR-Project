#pragma once
#include "Core/PlatformInterface/IPlatformThread.h"
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

    // Register a thread this is called from the constructor of the platform thread
    void RegisterThread(IPlatformThread* InThread);

    // Unregister a thread, this is called from the destructor of the platform thread
    void UnregisterThread(IPlatformThread* InThread);

    // Retrieve a ThreadObject from a native ThreadHandle
    IPlatformThread* GetThreadFromHandle(void* ThreadHandle);

    // Check if the thread-handle is for the main-thread
    bool IsMainThread(void* ThreadHandle) const 
    {
        return MainThreadHandle == ThreadHandle;
    }

private:
    friend class TOptional<FThreadManager>;
    
    FThreadManager();
    ~FThreadManager();

    void*                    MainThreadHandle;
    TArray<IPlatformThread*> Threads;
    FCriticalSection         ThreadsCS;
};
