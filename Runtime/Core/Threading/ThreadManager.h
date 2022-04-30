#pragma once
#include "Generic/GenericThread.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Optional.h"

typedef TSharedRef<CGenericThread> GenericThreadRef; 

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CThreadManager

class CORE_API CThreadManager
{
private:

    friend class TOptional<CThreadManager>;

    CThreadManager();
    ~CThreadManager();

public:

    static bool Initialize();

    static bool Release();

    static bool IsMainThread();

    static CThreadManager& Get();

    GenericThreadRef CreateThread(const TFunction<void()>& InFunction);

    GenericThreadRef CreateNamedThread(const TFunction<void()>& InFunction, const String& InName);

    GenericThreadRef GetNamedThread(const String& InName);

    GenericThreadRef GetThreadFromHandle(void* ThreadHandle);

private:

    static TOptional<CThreadManager>& GetConsoleManagerInstance();

    TArray<GenericThreadRef> Threads;
    void*                    MainThread;
};