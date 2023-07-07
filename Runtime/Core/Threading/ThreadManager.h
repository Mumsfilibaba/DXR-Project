#pragma once
#include "Core/Generic/GenericThread.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Optional.h"

class CORE_API FThreadManager
{
private:
    friend class TOptional<FThreadManager>;

    FThreadManager();
    ~FThreadManager();

public:
    static bool Initialize();
    static bool Release();

    static FThreadManager& Get();

    static bool IsMainThread();

    TSharedRef<FGenericThread> CreateThread(FThreadInterface* InRunnable);
    TSharedRef<FGenericThread> GetThreadFromHandle(void* ThreadHandle);

private:
    TArray<TSharedRef<FGenericThread>> Threads;
    void*                     MainThread;
};