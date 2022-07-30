#pragma once
#include "Generic/GenericThread.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Optional.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FThreadManager

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

    FGenericThreadRef CreateThread(const TFunction<void()>& InFunction);
    FGenericThreadRef CreateNamedThread(const TFunction<void()>& InFunction, const FString& InName);

    FGenericThreadRef GetNamedThread(const FString& InName);
    FGenericThreadRef GetThreadFromHandle(void* ThreadHandle);

private:
    TArray<FGenericThreadRef> Threads;
    void*                     MainThread;
};