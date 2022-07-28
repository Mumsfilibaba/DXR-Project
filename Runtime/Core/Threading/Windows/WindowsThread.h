#pragma once
#include "Core/Containers/Function.h"
#include "Core/Threading/Generic/GenericThread.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThread

class CORE_API FWindowsThread final 
    : public FGenericThread
{
private:
    FWindowsThread(const TFunction<void()>& InFunction);
    FWindowsThread(const TFunction<void()>& InFunction, const FString& InName);
    ~FWindowsThread();

public:

    static FWindowsThread* CreateWindowsThread(const TFunction<void()>& InFunction);
    static FWindowsThread* CreateWindowsThread(const TFunction<void()>& InFunction, const FString& InName);

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericThread Interface

    virtual int32 WaitForCompletion(uint64 TimeoutInMs) override final;

    virtual bool Start() override final;

    virtual void SetName(const FString& InName) override final;

    virtual void* GetPlatformHandle() override final;

    virtual FString GetName() const override final { return Name; }

private:

    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE Thread;
    DWORD  hThreadID;

    FString Name;
};