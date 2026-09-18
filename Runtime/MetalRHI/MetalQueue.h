#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalDeletionQueue.h"

class FMetalDevice;
struct FMetalCommands;

enum class EMetalQueueType : uint8
{
    Direct  = 0,
    Compute = 1,
    Copy    = 2,
};

class FMetalQueue : public FMetalDeviceChild
{
public:
    FMetalQueue(FMetalDevice* InDevice, EMetalQueueType InQueueType);
    ~FMetalQueue();

    bool Initialize();

    FMetalCommands*      ObtainCommands();
    id<MTLCommandBuffer> CreateCommandBuffer();

    void SubmitCommands(FMetalCommands* Commands);
    void ProcessCommandQueue();
    void WaitForCompletion();

    uint64 GetCompletedValue() const;

    id<MTLCommandQueue> GetMTLCommandQueue() const { return CommandQueue; }
    EMetalQueueType     GetType()            const { return QueueType; }

private:
    void RecycleCommands(FMetalCommands* Commands);

    id<MTLCommandQueue>                       CommandQueue;
    id<MTLSharedEvent>                        SubmissionEvent;
    TAtomicInt<uint64>                        NextSubmissionValue;
    TQueue<FMetalCommands*, EQueueType::MPSC> PendingSubmissions;
    TArray<FMetalCommands*>                   FreeCommands;
    FCriticalSection                          FreeCommandsCS;
    EMetalQueueType                           QueueType;
};

struct FMetalCommands
{
    FMetalCommands(FMetalDevice* InDevice, FMetalQueue* InQueue);
    ~FMetalCommands();

    void PostExecute();

    FMetalQueue*                  Queue;
    FMetalDevice* const           Device;
    id<MTLCommandBuffer>          CommandBuffer;
    uint64                        SubmissionValue;
    TArray<FMetalDeferredObject>  DeferredObjects;
};
