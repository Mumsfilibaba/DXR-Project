#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Queue.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalDeletionQueue.h"
#include "MetalRHI/MetalResource.h"

class FMetalDevice;
struct FMetalCommands;
struct FMetalQueryRHI;

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

    uint64 SubmitCommands(FMetalCommands* Commands);

    void ProcessCommandQueue();
    void WaitForCompletion();
    void WaitForValue(uint64 Value);

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
    static constexpr int32 MaxBreadcrumbs = 64;

    FMetalCommands(FMetalDevice* InDevice, FMetalQueue* InQueue);
    ~FMetalCommands();

    void PostExecute();
    void RecordBreadcrumb(const String& Name);

    FMetalQueue*                  Queue;
    FMetalDevice* const           Device;
    id<MTLCommandBuffer>          CommandBuffer;
    uint64                        SubmissionValue;
    TArray<FMetalDeferredObject>  DeferredObjects;
    TArray<FMetalQueryRHI*>       PendingQueries;
    TArray<id<MTLSharedEvent>>    PendingSignalEvents;
    TArray<uint64>                PendingSignalValues;
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
