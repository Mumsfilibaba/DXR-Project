#pragma once
#include "Core/Containers/Function.h"
#include "Core/Threading/Generic/GenericThread.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsThread

class CORE_API CWindowsThread final : public CGenericThread
{
private:

    CWindowsThread(const TFunction<void()>& InFunction);
    CWindowsThread(const TFunction<void()>& InFunction, const String& InName);
    ~CWindowsThread();

public:

    static CWindowsThread* CreateWindowsThread(const TFunction<void()>& InFunction);
    static CWindowsThread* CreateWindowsThread(const TFunction<void()>& InFunction, const String& InName);

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericThread Interface

    virtual int32 WaitForCompletion(uint64 TimeoutInMs) override final;

    virtual bool Start() override final;

    virtual void SetName(const String& InName) override final;

    virtual void* GetPlatformHandle() override final;

    virtual String GetName() const override final { return Name; }

private:

    static DWORD WINAPI ThreadRoutine(LPVOID ThreadParameter);

    HANDLE Thread;
    DWORD  hThreadID;

    String Name;
};