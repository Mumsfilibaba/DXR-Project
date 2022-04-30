#pragma once
#include "Core/Threading/Generic/GenericThread.h"

#include <pthread.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacThread

class CMacThread final : public CGenericThread
{
private:

    CMacThread(const TFunction<void()>& InFunction);
    CMacThread(const TFunction<void()>& InFunction, const String& InName);
    ~CMacThread() = default;

public:

	static CMacThread* CreateMacThread(const TFunction<void()>& InFunction) { return new CMacThread(InFunction); }
	static CMacThread* CreateMacThread(const TFunction<void()>& InFunction, const String& InName) { return new CMacThread(InFunction, InName); }
	
public:
	
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericThread Interface

    virtual int32 WaitForCompletion(uint64 TimeoutInMs) override final;

    virtual bool Start() override final;

    virtual void SetName(const String& InName) override final;

    virtual void* GetPlatformHandle() override final;

    virtual String GetName() const override final { return Name; }

private:

    static void* ThreadRoutine(void* ThreadParameter);

    String    Name;

    pthread_t Thread;
	
	int32     ThreadExitCode;

    bool      bIsRunning;
};
