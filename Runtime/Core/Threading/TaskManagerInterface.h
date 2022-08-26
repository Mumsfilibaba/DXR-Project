#pragma once
#include "AtomicInt.h"

#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/ConditionVariable.h"
#include "Core/Platform/PlatformThread.h"
#include "Core/Delegates/Delegate.h"

typedef int64 DispatchID;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncTask

class FAsyncTask
{
public:

    DECLARE_DELEGATE(FTaskDelegate);
    FTaskDelegate Delegate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTaskManagerInterface

class CORE_API FTaskManagerInterface
{
protected:
    FTaskManagerInterface()  = default;
    ~FTaskManagerInterface() = default;

public:
    static FTaskManagerInterface& Get();

    virtual bool Initialize() = 0;
    virtual void Release()    = 0;

    virtual DispatchID Dispatch(const FAsyncTask& NewTask) = 0;

    virtual void WaitFor(DispatchID Task, bool bUseThisThreadWhileWaiting = true) = 0;
    virtual void WaitForAll(bool bUseThisThreadWhileWaiting = true)               = 0;
};
