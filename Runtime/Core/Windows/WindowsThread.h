#pragma once
#include "Core/Containers/Function.h"
#include "Core/Generic/GenericThread.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsThread

class CORE_API FWindowsThread final 
    : public FGenericThread
{
public:
    FWindowsThread(FThreadInterface* InRunnable);
    ~FWindowsThread();

    virtual bool Start() override final;

    virtual void WaitForCompletion() override final;

    virtual void* GetPlatformHandle() override final;

    virtual FString GetName() const override final { return Name; }
    
    virtual void SetName(const FString& InName) override final;

private:
    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE Thread;
    DWORD  hThreadID;

    FString Name;
};