#pragma once
#include "Core/Generic/GenericThread.h"

#include <pthread.h>

class FMacThread final : public FGenericThread
{
public:
    FMacThread(FThreadInterface* InRunnable);
    virtual ~FMacThread() = default;

    virtual bool Start() override final;

    virtual void WaitForCompletion() override final;

    virtual void* GetPlatformHandle() override final;

    virtual FString GetName() const override final { return Name; }
    
    virtual void SetName(const FString& InName) override final;

private:
    static void* ThreadRoutine(void* ThreadParameter);

    FString   Name;
    pthread_t Thread;
    bool      bIsRunning;
};
