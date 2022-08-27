#pragma once
#include "Core/Containers/Function.h"
#include "Core/Generic/GenericThread.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThread

class CORE_API FWindowsThread final 
    : public FGenericThread
{
public:
    FWindowsThread(const FThreadFunction& InFunction);
    FWindowsThread(const FThreadFunction& InFunction, const FString& InName);
    ~FWindowsThread();

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