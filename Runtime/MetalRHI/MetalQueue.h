#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalDeletionQueue.h"
#include "MetalRHI/MetalRecyclePool.h"
#include "MetalRHI/MetalResource.h"

class FMetalDevice;
class FMetalCommandContext;
class FMetalBufferRHI;
class FMetalTextureRHI;
struct FMetalCommands;
struct FMetalQueryRHI;

enum class EMetalQueueType : uint8
{
    Direct  = 0,
    Compute = 1,
    Copy    = 2,
};

struct FMetalQueueFence
{
    id<MTLSharedEvent> Event;
    uint64             Value;
    bool               bEncoded;
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

private:
    void RecycleCommands(FMetalCommands* Commands);

    id<MTLCommandQueue>                       CommandQueue;
    id<MTLSharedEvent>                        SubmissionEvent;
    TAtomicInt<uint64>                        NextSubmissionValue;
    TQueue<FMetalCommands*, EQueueType::MPSC> PendingSubmissions;
    TArray<FMetalCommands*>                   FreeCommands;
    FCriticalSection                          FreeCommandsCS;
    FCriticalSection                          ConsumerCS;
    TMetalRecyclePool<FMetalCommandContext>   CommandContextPool;
    TAtomicInt<uint64>                        CurrentFrame;
    EMetalQueueType                           QueueType;
};

struct FMetalCommands
{
    static constexpr int32 MaxBreadcrumbs = 64;

    FMetalCommands(FMetalDevice* InDevice, FMetalQueue* InQueue);
    ~FMetalCommands();

    void PostExecute();
    void RecordBreadcrumb(const String& Name);
    void AddWait(FMetalQueue* Producer, uint64 Value);
    void EncodePendingWaits();

    FMetalQueue*                  Queue;
    FMetalDevice* const           Device;
    id<MTLCommandBuffer>          CommandBuffer;
    uint64                        SubmissionValue;
    TArray<FMetalDeferredObject>  DeferredObjects;
    TArray<FMetalQueryRHI*>       PendingQueries;
    TArray<id<MTLSharedEvent>>    PendingSignalEvents;
    TArray<uint64>                PendingSignalValues;
    TArray<FMetalQueueFence>      PendingWaits;
    TArray<FMetalBufferRHI*>      UsedBuffers;
    TArray<FMetalTextureRHI*>     UsedTextures;
    TArray<String>                Breadcrumbs;
    String                        DebugLabel;
};

class FMetalUploadBatch
{
public:
    explicit FMetalUploadBatch(FMetalDevice* InDevice);
    ~FMetalUploadBatch();

    bool CreateStagingBuffer(uint64 Size, FMetalResourceStorage& OutStorage);
    uint64 Submit();

    bool IsValid() const
    {
        return BlitEncoder != nil;
    }

    id<MTLBlitCommandEncoder> GetBlitEncoder() const
    {
        return BlitEncoder;
    }

private:
    FMetalDevice*             Device;
    FMetalQueue*              Queue;
    FMetalCommands*           Commands;
    id<MTLBlitCommandEncoder> BlitEncoder;
};
