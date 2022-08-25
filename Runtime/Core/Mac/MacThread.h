#pragma once
#include "Core/Generic/GenericThread.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacThread

class FMacThread final 
    : public FGenericThread
{
public:
    FMacThread(const FThreadFunction& InFunction);
    FMacThread(const FThreadFunction& InFunction, const FString& InName);

    virtual int32 WaitForCompletion(uint64 TimeoutInMs) override final;

    virtual bool Start() override final;

    virtual void* GetPlatformHandle() override final;

    virtual FString GetName() const override final { return Name; }
    virtual void    SetName(const FString& InName) override final;

private:

    static void* ThreadRoutine(void* ThreadParameter);

    FString    Name;

    pthread_t Thread;
	int32     ThreadExitCode;

    bool      bIsRunning;
};
