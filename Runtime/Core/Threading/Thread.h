#pragma once
#include "ThreadInterface.h"
#include "Core/Platform/PlatformThreadMisc.h"

class FThread : public FThreadInterface
{
public:
    FThread()
        : FThreadInterface()
        , Thread(nullptr)
    {
    }

    virtual bool Initialize(bool bSuspended)
    {
        Thread = FPlatformThreadMisc::CreateThread(this, bSuspended);
        return Thread != nullptr;
    }

    void Resume()
    {
        CHECK(Thread != nullptr);
        Thread->Start();
    }

    void WaitForCompletion()
    {
        CHECK(Thread != nullptr);
        Thread->WaitForCompletion();
    }

private:
    TSharedRef<FGenericThread> Thread;
};