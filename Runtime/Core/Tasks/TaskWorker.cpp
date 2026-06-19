#include "Core/Tasks/TaskWorker.h"
#include "Core/Tasks/TaskGraph.h"
#include "Core/Tasks/TaskBase.h"
#include "Core/Platform/PlatformEvent.h"
#include "Core/Platform/PlatformThread.h"
#include "Core/Platform/PlatformTLS.h"
#include "Core/Misc/OutputDeviceLogger.h"

uint32 FTaskWorker::CurrentWorkerTLSSlot = CORE_INVALID_TLS_INDEX;

FTaskWorker::FTaskWorker(FTaskGraph* InGraph, ENamedThread::Type InLane, int32 InWorkerIndex)
    : bIsIdle(false)
    , Graph(InGraph)
    , Lane(InLane)
    , WorkerIndex(InWorkerIndex)
    , WakeEvent(nullptr)
    , Thread(nullptr)
    , bIsRunning(false)
{
}

FTaskWorker::~FTaskWorker()
{
    if (Thread)
    {
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }

    if (WakeEvent)
    {
        FPlatformEvent::Recycle(WakeEvent);
        WakeEvent = nullptr;
    }
}

bool FTaskWorker::Initialize(const CHAR* InThreadName)
{
    WakeEvent = FPlatformEvent::Create(false);
    if (!WakeEvent)
    {
        LOG_ERROR("[FTaskWorker] Failed to create wake event");
        return false;
    }

    Thread = FPlatformThread::Create(this, InThreadName);
    if (!Thread)
    {
        LOG_ERROR("[FTaskWorker] Failed to create thread");
        return false;
    }

    if (!Thread->Start())
    {
        LOG_ERROR("[FTaskWorker] Failed to start thread");
        return false;
    }

    return true;
}

bool FTaskWorker::Start()
{
    bIsRunning.Store(true);
    return true;
}

int32 FTaskWorker::Run()
{
    FPlatformTLS::SetTLSValue(CurrentWorkerTLSSlot, this);

    while (bIsRunning.Load())
    {
        FGraphTask* Task = Graph->TryAcquireWork(this);
        if (Task)
        {
            Task->Execute();
            continue;
        }

        Graph->MarkWorkerIdle(this);

        Task = Graph->TryAcquireWork(this);
        if (Task)
        {
            Graph->MarkWorkerActive(this);
            Task->Execute();
            continue;
        }

        WakeEvent->Wait(FTimespan::Infinity());
        Graph->MarkWorkerActive(this);
    }

    FPlatformTLS::SetTLSValue(CurrentWorkerTLSSlot, nullptr);
    return 0;
}

void FTaskWorker::Stop()
{
    bIsRunning.Store(false);
    Wake();
}

void FTaskWorker::Wake()
{
    if (WakeEvent)
    {
        WakeEvent->Trigger();
    }
}

FTaskWorker* FTaskWorker::GetCurrentWorker()
{
    return static_cast<FTaskWorker*>(FPlatformTLS::GetTLSValue(CurrentWorkerTLSSlot));
}

bool FTaskWorker::AllocateTLSSlot()
{
    CurrentWorkerTLSSlot = FPlatformTLS::AllocTLSSlot();
    if (CurrentWorkerTLSSlot == CORE_INVALID_TLS_INDEX)
    {
        LOG_ERROR("[FTaskWorker] Failed to allocate TLS slot for current-worker tracking");
        return false;
    }

    return true;
}

void FTaskWorker::FreeTLSSlot()
{
    if (CurrentWorkerTLSSlot != CORE_INVALID_TLS_INDEX)
    {
        FPlatformTLS::FreeTLSSlot(CurrentWorkerTLSSlot);
        CurrentWorkerTLSSlot = CORE_INVALID_TLS_INDEX;
    }
}
