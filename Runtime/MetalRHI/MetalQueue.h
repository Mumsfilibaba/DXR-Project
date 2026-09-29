#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalDeletionQueue.h"
#include "MetalRHI/MetalEncoderManager.h"
#include "MetalRHI/MetalRecyclePool.h"
#include "MetalRHI/MetalResource.h"

class FMetalDevice;
class FMetalQueue;
class FMetalCommandContext;
struct FMetalCommands;
struct FMetalQueryRHI;

enum class EMetalQueueType : uint8
{
    Direct  = 0,
    Compute = 1,
    Copy    = 2,
    Count   = 3,
};

struct FMetalSyncPoint
{
    FMetalQueue* Queue = nullptr;
    uint64       Value = 0;
};

struct FMetalBreadcrumbRing
{
    static constexpr uint32 NumEntries  = 64;
    static constexpr uint32 EntryLength = 48;

    void Push(const StringView& Name);

    void Reset()
    {
        Head  = 0;
        Count = 0;
    }

    template<typename FunctionType>
    void ForEach(FunctionType&& Function) const
    {
        const uint32 First = (Head + NumEntries - Count) % NumEntries;
        for (uint32 Index = 0; Index < Count; ++Index)
        {
            Function(Entries[(First + Index) % NumEntries]);
        }
    }

    CHAR   Entries[NumEntries][EntryLength];
    uint32 Head  = 0;
    uint32 Count = 0;
};

class FMetalQueue : public FMetalDeviceChild
{
public:
    FMetalQueue(FMetalDevice* InDevice, EMetalQueueType InQueueType);
    ~FMetalQueue();

    bool Initialize();

    FMetalCommands*      ObtainCommands();
    id<MTLCommandBuffer> CreateCommandBuffer();

    uint64 SubmitCommands(FMetalCommands* Commands);

    void ProcessCommandQueue();
    void WaitForCompletion();
    void WaitForValue(uint64 Value);

    FMetalCommandContext* ObtainCommandContext();
    void ReleaseCommandContext(FMetalCommandContext* InContext);
    void PruneCommandContexts(uint64 CurrentFrame);

    uint64 GetCompletedValue() const;

    uint64 GetLastSubmittedValue() const
    {
        return NextSubmissionValue.Load();
    }

    EMetalQueueType GetType() const
    {
        return QueueType;
    }

    id<MTLCommandQueue> GetMTLCommandQueue() const
    {
        return CommandQueue;
    }

    id<MTLSharedEvent> GetSubmissionEvent() const
    {
        return SubmissionEvent;
    }

    FMetalEncoderFence& GetEncoderFence()
    {
        return EncoderFence;
    }

private:
    void RecycleCommands(FMetalCommands* Commands);

    id<MTLCommandQueue>                       CommandQueue;
    MTLCommandBufferDescriptor*               CommandBufferDescriptor;
    id<MTLSharedEvent>                        SubmissionEvent;
    FMetalEncoderFence                        EncoderFence;
    TAtomicInt<uint64>                        NextSubmissionValue;
    TQueue<FMetalCommands*, EQueueType::MPSC> PendingSubmissions;
    TArray<FMetalCommands*>                   FreeCommands;
    FCriticalSection                          FreeCommandsCS;
    FCriticalSection                          ConsumerCS;
    TMetalRecyclePool<FMetalCommandContext>   CommandContextPool;
    TAtomicInt<uint64>                        CurrentFrame;
    EMetalQueueType                           QueueType;
};

struct FMetalEventValue
{
    id<MTLSharedEvent> Event = nil;
    uint64             Value = 0;
};

struct FMetalCommands
{
    FMetalCommands(FMetalDevice* InDevice, FMetalQueue* InQueue);
    ~FMetalCommands();

    void Reset();
    void PostExecute();
    void AddWait(const FMetalSyncPoint& SyncPoint);
    void EncodePendingWaits();

    FMetalQueue*                  Queue;
    FMetalDevice* const           Device;
    id<MTLCommandBuffer>          CommandBuffer;
    uint64                        SubmissionValue;
    TArray<FMetalDeferredObject>  DeferredObjects;
    TArray<FMetalQueryRHI*>       PendingQueries;
    TArray<FMetalEventValue>      PendingSignals;
    TArray<FMetalSyncPoint>       PendingWaits;
    FMetalBreadcrumbRing          Breadcrumbs;
    bool                          bUpdatesEncoderFence;
};
