#pragma once
#include "Core/Threading/Generic/GenericThread.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacThread

class FMacThread final : public FGenericThread
{
private:

    FMacThread(const TFunction<void()>& InFunction);
    FMacThread(const TFunction<void()>& InFunction, const FString& InName);
    ~FMacThread() = default;

public:

	static FMacThread* CreateMacThread(const TFunction<void()>& InFunction) { return new FMacThread(InFunction); }
	static FMacThread* CreateMacThread(const TFunction<void()>& InFunction, const FString& InName) { return new FMacThread(InFunction, InName); }
	
public:
	
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericThread Interface

    virtual int32 WaitForCompletion(uint64 TimeoutInMs) override final;

    virtual bool Start() override final;

    virtual void SetName(const FString& InName) override final;

    virtual void* GetPlatformHandle() override final;

    virtual FString GetName() const override final { return Name; }

private:

    static void* ThreadRoutine(void* ThreadParameter);

    FString    Name;

    pthread_t Thread;
	
	int32     ThreadExitCode;

    bool      bIsRunning;
};
