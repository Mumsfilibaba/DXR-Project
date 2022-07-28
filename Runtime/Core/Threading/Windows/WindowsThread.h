#pragma once
#include "Core/Containers/Function.h"
#include "Core/Threading/Generic/GenericThread.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThread

class CORE_API FWindowsThread final 
    : public FGenericThread
{
    friend struct FWindowsThreadMisc;

private:
    FWindowsThread(const TFunction<void()>& InFunction);
    FWindowsThread(const TFunction<void()>& InFunction, const FString& InName);
    ~FWindowsThread();

public:
    virtual bool Start() override final;

    virtual int32 WaitForCompletion(uint64 TimeoutInMs) override final;

    virtual void* GetPlatformHandle() override final;

    virtual FString GetName() const override final { return Name; }
    virtual void    SetName(const FString& InName) override final;

private:
    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE Thread;
    DWORD  hThreadID;

    FString Name;
};