#pragma once
#include "Core/Generic/GenericThread.h"
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

    // Register a thread this is called from the constructor of the FGenericThread
    void RegisterThread(FGenericThread* InThread);

    // Unregister a thread, this is called from the destructor of the FGenericThread
    void UnregisterThread(FGenericThread* InThread);

    // Retrieve a ThreadObject from a native ThreadHandle
    FGenericThread* GetThreadFromHandle(void* ThreadHandle);

    // Check if the thread-handle is for the main-thread
    bool IsMainThread(void* ThreadHandle) const 
    {
        return MainThreadHandle == ThreadHandle;
    }

    // Retrieve the main-thread handle
    void* GetMainThreadHandle() const
    {
        return MainThreadHandle;
    }

private:
    friend class TOptional<FThreadManager>;
    
    FThreadManager();
    ~FThreadManager();

    void*                   MainThreadHandle;
    TArray<FGenericThread*> Threads;
    FCriticalSection        ThreadsCS;
};
