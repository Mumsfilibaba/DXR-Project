#pragma once
#include "Core/Threading/Generic/GenericThread.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacThread

class CMacThread final : public CGenericThread
{
private:

    CMacThread(ThreadFunction InFunction);
    CMacThread(ThreadFunction InFunction, const String& InName);
    ~CMacThread() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericThread Interface

    static TSharedRef<CMacThread> Make(ThreadFunction InFunction) { return new CMacThread(InFunction); }
    static TSharedRef<CMacThread> Make(ThreadFunction InFunction, const String& InName) { return new CMacThread(InFunction, InName); }

    virtual bool Start() override final;

    virtual void WaitUntilFinished() override final;

    virtual void SetName(const String& InName) override final;

    virtual void* GetPlatformHandle() override final;

private:

    static void* ThreadRoutine(void* ThreadParameter);

    pthread_t      Thread;
    ThreadFunction Function;

    String         Name;

    bool           bIsRunning;
};
