#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalQuery.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Threading/ScopedLock.h"

#if METAL_ENABLE_LOGGING
static const CHAR* ToString(MTLCommandBufferError ErrorCode)
{
    switch (ErrorCode)
    {
        case MTLCommandBufferErrorNone:            return "None";
        case MTLCommandBufferErrorInternal:        return "Internal";
        case MTLCommandBufferErrorTimeout:         return "Timeout";
        case MTLCommandBufferErrorPageFault:       return "PageFault";
        case MTLCommandBufferErrorAccessRevoked:   return "AccessRevoked";
        case MTLCommandBufferErrorNotPermitted:    return "NotPermitted";
        case MTLCommandBufferErrorOutOfMemory:     return "OutOfMemory";
        case MTLCommandBufferErrorInvalidResource: return "InvalidResource";
        case MTLCommandBufferErrorMemoryless:      return "Memoryless";
        case MTLCommandBufferErrorDeviceRemoved:   return "DeviceRemoved";
        case MTLCommandBufferErrorStackOverflow:   return "StackOverflow";
        default:                                   return "Unknown";
    }
}

static const CHAR* ToString(MTLCommandEncoderErrorState ErrorState)
{
    switch (ErrorState)
    {
        case MTLCommandEncoderErrorStateCompleted: return "Completed";
        case MTLCommandEncoderErrorStateAffected:  return "Affected";
        case MTLCommandEncoderErrorStatePending:   return "Pending";
        case MTLCommandEncoderErrorStateFaulted:   return "Faulted";
        default:                                   return "Unknown";
    }
}

static void ReportCommandBufferError(id<MTLCommandBuffer> CommandBuffer)
{
    if (!CommandBuffer || CommandBuffer.status != MTLCommandBufferStatusError)
    {
        return;
    }

    const String Label(CommandBuffer.label ? CommandBuffer.label : @"<unnamed>");

    NSError* Error = CommandBuffer.error;
    if (!Error)
    {
        METAL_ERROR("Command buffer '%s' failed without reporting an error", *Label);
        return;
    }

    const String Description(Error.localizedDescription);
    if ([Error.domain isEqualToString:MTLCommandBufferErrorDomain])
    {
        METAL_ERROR("Command buffer '%s' failed with %s: %s", *Label, ToString(MTLCommandBufferError(Error.code)), *Description);
    }
    else
    {
        const String Domain(Error.domain);
        METAL_ERROR("Command buffer '%s' failed with %s(%ld): %s", *Label, *Domain, long(Error.code), *Description);
    }

    NSArray<id<MTLCommandBufferEncoderInfo>>* EncoderInfos = Error.userInfo[MTLCommandBufferEncoderInfoErrorKey];
    for (id<MTLCommandBufferEncoderInfo> EncoderInfo in EncoderInfos)
    {
        const String EncoderLabel(EncoderInfo.label ? EncoderInfo.label : @"<unnamed>");
        METAL_ERROR("  Encoder '%s': %s", *EncoderLabel, ToString(EncoderInfo.errorState));

        for (NSString* Signpost in EncoderInfo.debugSignposts)
        {
            const String SignpostLabel(Signpost);
            METAL_ERROR("    %s", *SignpostLabel);
        }
    }
}
#endif

FMetalQueue::FMetalQueue(FMetalDevice* InDevice, EMetalQueueType InQueueType)
    : FMetalDeviceChild(InDevice)
    , CommandQueue(nil)
    , SubmissionEvent(nil)
    , NextSubmissionValue(0)
    , PendingSubmissions()
    , FreeCommands()
    , FreeCommandsCS()
    , QueueType(InQueueType)
{
}

FMetalQueue::~FMetalQueue()
{
    WaitForCompletion();

    for (FMetalCommands* Commands : FreeCommands)
    {
        delete Commands;
    }
    FreeCommands.Clear();

    [SubmissionEvent release];
    SubmissionEvent = nil;

    [CommandQueue release];
    CommandQueue = nil;
}

bool FMetalQueue::Initialize()
{
    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();
    CommandQueue = [DeviceHandle newCommandQueue];
    if (!CommandQueue)
    {
        METAL_ERROR("Failed to create MTLCommandQueue");
        return false;
    }

    SubmissionEvent = [DeviceHandle newSharedEvent];
    if (!SubmissionEvent)
    {
        METAL_ERROR("Failed to create MTLSharedEvent");
        return false;
    }

    return true;
}

FMetalCommands* FMetalQueue::ObtainCommands()
{
    FMetalCommands* Commands = nullptr;
    {
        TScopedLock Lock(FreeCommandsCS);
        if (!FreeCommands.IsEmpty())
        {
            Commands = FreeCommands.Last();
            FreeCommands.Pop();
        }
    }

    if (!Commands)
    {
        Commands = new FMetalCommands(GetDevice(), this);
    }

    Commands->CommandBuffer    = CreateCommandBuffer();
    Commands->SubmissionValue  = 0;
    Commands->DeferredObjects.Clear();
    Commands->PendingQueries.Clear();
    Commands->PendingSignalEvents.Clear();
    Commands->PendingSignalValues.Clear();
    return Commands;
}

id<MTLCommandBuffer> FMetalQueue::CreateCommandBuffer()
{
#if METAL_ENABLE_LOGGING
    MTLCommandBufferDescriptor* Descriptor = [[MTLCommandBufferDescriptor new] autorelease];
    Descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;

    id<MTLCommandBuffer> CommandBuffer = [CommandQueue commandBufferWithDescriptor:Descriptor];
#else
    id<MTLCommandBuffer> CommandBuffer = [CommandQueue commandBuffer];
#endif
    if (CommandBuffer)
    {
        [CommandBuffer retain];
    }
    return CommandBuffer;
}

uint64 FMetalQueue::SubmitCommands(FMetalCommands* Commands)
{
    CHECK(Commands != nullptr);
    CHECK(Commands->CommandBuffer != nil);

    const uint64 Value = NextSubmissionValue.Increment();
    Commands->SubmissionValue = Value;

#if METAL_ENABLE_LOGGING
    [Commands->CommandBuffer setLabel:[NSString stringWithFormat:@"MetalQueue-%llu", Value]];

    [Commands->CommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> CompletedBuffer)
    {
        ReportCommandBufferError(CompletedBuffer);
    }];
#endif

    Commands->Device->GetTimestampQueries().EncodeResolve(Commands->CommandBuffer, Commands->PendingQueries);

    CHECK(Commands->PendingSignalEvents.Size() == Commands->PendingSignalValues.Size());
    for (int32 Index = 0; Index < Commands->PendingSignalEvents.Size(); ++Index)
    {
        [Commands->CommandBuffer encodeSignalEvent:Commands->PendingSignalEvents[Index] value:Commands->PendingSignalValues[Index]];
    }

    [Commands->CommandBuffer encodeSignalEvent:SubmissionEvent value:Value];
    [Commands->CommandBuffer commit];

    for (FMetalQueryRHI* Query : Commands->PendingQueries)
    {
        if (Query)
        {
            Query->SubmissionValue = Value;
        }
    }

    STAT_ADD(STAT_Metal_CommandBufferCount, 1);
    PendingSubmissions.Enqueue(Commands);
    return Value;
}

void FMetalQueue::ProcessCommandQueue()
{
    const uint64 CompletedValue = GetCompletedValue();

    bool bProcess = true;
    while (bProcess)
    {
        FMetalCommands* Commands = nullptr;
        if (PendingSubmissions.Peek(Commands))
        {
            CHECK(Commands != nullptr);
            if (Commands->SubmissionValue > CompletedValue)
            {
                bProcess = false;
            }
            else
            {
                PendingSubmissions.Dequeue();
                Commands->PostExecute();
                RecycleCommands(Commands);
            }
        }
        else
        {
            bProcess = false;
        }
    }
}

void FMetalQueue::WaitForCompletion()
{
    const uint64 LastSubmitted = NextSubmissionValue.Load();
    if (LastSubmitted >= 1 && SubmissionEvent)
    {
        [SubmissionEvent waitUntilSignaledValue:LastSubmitted timeoutMS:UINT64_MAX];
    }

    ProcessCommandQueue();
}

void FMetalQueue::WaitForValue(uint64 Value)
{
    if (Value > 0 && SubmissionEvent && GetCompletedValue() < Value)
    {
        [SubmissionEvent waitUntilSignaledValue:Value timeoutMS:UINT64_MAX];
    }

    ProcessCommandQueue();
}

uint64 FMetalQueue::GetCompletedValue() const
{
    return SubmissionEvent ? SubmissionEvent.signaledValue : 0;
}

void FMetalQueue::RecycleCommands(FMetalCommands* Commands)
{
    TScopedLock Lock(FreeCommandsCS);
    FreeCommands.Add(Commands);
}

FMetalCommands::FMetalCommands(FMetalDevice* InDevice, FMetalQueue* InQueue)
    : Queue(InQueue)
    , Device(InDevice)
    , CommandBuffer(nil)
    , SubmissionValue(0)
    , DeferredObjects()
    , PendingQueries()
    , PendingSignalEvents()
    , PendingSignalValues()
{
}

FMetalCommands::~FMetalCommands()
{
    if (CommandBuffer)
    {
        [CommandBuffer release];
        CommandBuffer = nil;
    }
}

void FMetalCommands::PostExecute()
{
    ResolveMetalQueries(PendingQueries);
    FMetalDeferredObject::ProcessItems(DeferredObjects);
    DeferredObjects.Clear();

    if (CommandBuffer)
    {
        [CommandBuffer release];
        CommandBuffer = nil;
    }
}

FMetalUploadBatch::FMetalUploadBatch(FMetalDevice* InDevice)
    : Device(InDevice)
    , Queue(InDevice ? InDevice->GetQueue() : nullptr)
    , Commands(nullptr)
    , BlitEncoder(nil)
{
    if (!Queue)
    {
        return;
    }

    Commands = Queue->ObtainCommands();
    if (!Commands || !Commands->CommandBuffer)
    {
        METAL_ERROR("Failed to obtain a command buffer for an upload batch");
        return;
    }

    BlitEncoder = [[Commands->CommandBuffer blitCommandEncoder] retain];
    METAL_ERROR_COND(BlitEncoder != nil, "Failed to create a blit encoder for an upload batch");
}

FMetalUploadBatch::~FMetalUploadBatch()
{
    Submit();
}

id<MTLBuffer> FMetalUploadBatch::CreateStagingBuffer(uint64 Size)
{
    if (!Commands || Size == 0)
    {
        return nil;
    }

    id<MTLDevice> DeviceHandle  = Device->GetMTLDevice();
    id<MTLBuffer> StagingBuffer = [DeviceHandle newBufferWithLength:Size options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache];

    if (!StagingBuffer)
    {
        METAL_ERROR("Failed to allocate a %llu byte staging buffer", Size);
        return nil;
    }

    Commands->DeferredObjects.Emplace(StagingBuffer);
    [StagingBuffer release];
    return StagingBuffer;
}

uint64 FMetalUploadBatch::Submit()
{
    if (BlitEncoder)
    {
        [BlitEncoder endEncoding];
        [BlitEncoder release];
        BlitEncoder = nil;
    }

    if (!Commands)
    {
        return 0;
    }

    const uint64 SubmissionValue = Queue->SubmitCommands(Commands);
    Commands = nullptr;
    return SubmissionValue;
}
