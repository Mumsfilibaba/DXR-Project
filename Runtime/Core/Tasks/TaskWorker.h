#pragma once
#include "Core/Core.h"
#include "Core/Tasks/TaskTypes.h"
#include "Core/Threading/Runnable.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Templates/Utility/NonCopyable.h"

class FGenericPlatformThread;
class FGenericPlatformEvent;

class CORE_API FTaskWorker : public FRunnable, private FNonCopyable
{
    friend class FTaskGraph;

public:

    /** @return Returns the worker running on the calling thread, or nullptr for unmanaged threads. */
    static FTaskWorker* GetCurrentWorker();

    /** @brief Allocates the OS TLS slot used to track the current worker. Call before any worker starts. */
    static bool AllocateTLSSlot();

    /** @brief Frees the OS TLS slot used to track the current worker. Call after all workers are joined. */
    static void FreeTLSSlot();

public:
    FTaskWorker(FTaskGraph* InGraph, ENamedThread::Type InLane, int32 InWorkerIndex);
    ~FTaskWorker();
    
    // FRunnable interface
    virtual bool Start() override final;
    virtual int32 Run() override final;
    virtual void Stop() override final;

    /** @brief Creates the wake event and OS thread for this worker. */
    bool Initialize(const CHAR* InThreadName);

    /** @brief Signals the worker's wake event so it leaves its idle wait. */
    void Wake();

    FORCEINLINE ENamedThread::Type GetLane() const
    {
        return Lane;
    }

    FORCEINLINE int32 GetWorkerIndex() const
    {
        return WorkerIndex;
    }

private:
    AtomicBool              bIsIdle;
    FTaskGraph*             Graph;
    ENamedThread::Type      Lane;
    int32                   WorkerIndex;
    FGenericPlatformEvent*  WakeEvent;
    FGenericPlatformThread* Thread;
    AtomicBool              bIsRunning;

    static uint32 CurrentWorkerTLSSlot;
};
