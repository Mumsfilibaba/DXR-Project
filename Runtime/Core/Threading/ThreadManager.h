#pragma once
#include "Generic/GenericThread.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Optional.h"

typedef TSharedRef<FGenericThread> FGenericThreadRef; 

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

    static bool IsMainThread();

    static FThreadManager& Get();

    FGenericThreadRef CreateThread(const TFunction<void()>& InFunction);

    FGenericThreadRef CreateNamedThread(const TFunction<void()>& InFunction, const FString& InName);

    FGenericThreadRef GetNamedThread(const FString& InName);

    FGenericThreadRef GetThreadFromHandle(void* ThreadHandle);

private:

    static TOptional<FThreadManager>& GetConsoleManagerInstance();

    TArray<FGenericThreadRef> Threads;
    void*                    MainThread;
};