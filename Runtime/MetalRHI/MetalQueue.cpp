#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalQuery.h"
#include "MetalRHI/MetalStats.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalTexture.h"
#include "Core/Containers/String.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ScopedLock.h"

static TAutoConsoleVariable<int32> CVarMaxPendingSubmissions(
    "MetalRHI.MaxPendingSubmissions",
    "Maximum number of pending GPU submissions before the CPU waits for the GPU to catch up",
    32);

static TAutoConsoleVariable<int32> CVarCommandContextMaxIdleFrames(
    "MetalRHI.CommandContextPool.MaxIdleFrames",
    "Number of frames a pooled CommandContext may sit unused before it is destroyed",
    16);

static TAutoConsoleVariable<int32> CVarCommandContextMinRetained(
    "MetalRHI.CommandContextPool.MinRetained",
    "Number of CommandContexts the pool keeps alive regardless of how long they have been idle",
    2);

static void ReportFunctionLogs(id<MTLCommandBuffer> CommandBuffer)
{
    if (!CommandBuffer || !CommandBuffer.logs)
    {
        return;
    }

    for (id<MTLFunctionLog> FunctionLog in CommandBuffer.logs)
    {
        const String Description(FunctionLog.description);
        if (CString::Stristr(*Description, "error") != nullptr || CString::Stristr(*Description, "fault") != nullptr)
        {
            METAL_ERROR("[Metal Function] %s", *Description);
        }
        else
        {
            METAL_WARNING("[Metal Function] %s", *Description);
        }
    }
}

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

static void ReportCommandBufferError(id<MTLCommandBuffer> CommandBuffer, const TArray<String>& Breadcrumbs)
{
    if (!CommandBuffer || CommandBuffer.status != MTLCommandBufferStatusError)
    {
        return;
    }

    const String Label(CommandBuffer.label ? CommandBuffer.label : @"<unnamed>");

    NSError* Error = CommandBuffer.error;
    if (!Error)
    {
        LOG_ERROR("[MetalRHI] Command buffer '%s' failed without reporting an error", *Label);
        return;
    }

    const String Description(Error.localizedDescription);
    if ([Error.domain isEqualToString:MTLCommandBufferErrorDomain])
    {
        LOG_ERROR("[MetalRHI] Command buffer '%s' failed with %s: %s", *Label, ToString(MTLCommandBufferError(Error.code)), *Description);
    }
    else
    {
        const String Domain(Error.domain);
        LOG_ERROR("[MetalRHI] Command buffer '%s' failed with %s(%ld): %s", *Label, *Domain, long(Error.code), *Description);
    }

    NSArray<id<MTLCommandBufferEncoderInfo>>* EncoderInfos = Error.userInfo[MTLCommandBufferEncoderInfoErrorKey];
    for (id<MTLCommandBufferEncoderInfo> EncoderInfo in EncoderInfos)
    {
        const String EncoderLabel(EncoderInfo.label ? EncoderInfo.label : @"<unnamed>");
        LOG_ERROR("[MetalRHI]   Encoder '%s': %s", *EncoderLabel, ToString(EncoderInfo.errorState));

        for (NSString* Signpost in EncoderInfo.debugSignposts)
        {
            const String SignpostLabel(Signpost);
            LOG_ERROR("[MetalRHI]     %s", *SignpostLabel);
        }
    }

    if (!Breadcrumbs.IsEmpty())
    {
        LOG_ERROR("[MetalRHI]   Breadcrumbs:");
        for (const String& Name : Breadcrumbs)
        {
            LOG_ERROR("[MetalRHI]     %s", *Name);
        }
    }
}

FMetalQueue::FMetalQueue(FMetalDevice* InDevice, EMetalQueueType InQueueType)
    : FMetalDeviceChild(InDevice)
    , CommandQueue(nil)
    , SubmissionEvent(nil)
    , NextSubmissionValue(0)
    , PendingSubmissions()
    , FreeCommands()
    , FreeCommandsCS()
    , ConsumerCS()
    , CommandContextPool()
    , CurrentFrame(0)
    , QueueType(InQueueType)
{
}

FMetalQueue::~FMetalQueue()
{
    WaitForCompletion();
    ProcessCommandQueue();
    CommandContextPool.DestroyAll();

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
    Commands->PendingWaits.Clear();
    Commands->UsedBuffers.Clear();
    Commands->UsedTextures.Clear();
    Commands->Breadcrumbs.Clear();
    Commands->DebugLabel.Clear();
    return Commands;
}

id<MTLCommandBuffer> FMetalQueue::CreateCommandBuffer()
{
    MTLCommandBufferDescriptor* Descriptor = [[MTLCommandBufferDescriptor new] autorelease];
    Descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;

    id<MTLCommandBuffer> CommandBuffer = [CommandQueue commandBufferWithDescriptor:Descriptor];
    if (CommandBuffer)
    {
        [CommandBuffer retain];
    }
    return CommandBuffer;
}

FMetalCommandContext* FMetalQueue::ObtainCommandContext()
{
    FMetalCommandContext* CommandContext = CommandContextPool.Acquire([this](int32) -> FMetalCommandContext*
    {
        FMetalCommandContext* NewCommandContext = new FMetalCommandContext(GetDevice(), *this);
        if (!NewCommandContext->Initialize())
        {
            DEBUG_BREAK();
            delete NewCommandContext;
            return nullptr;
        }

        return NewCommandContext;
    });

    if (!CommandContext)
    {
        METAL_ERROR_CRITICAL("Failed to Obtain CommandContext");
    }

    return CommandContext;
}

void FMetalQueue::ReleaseCommandContext(FMetalCommandContext* InContext)
{
    CHECK(InContext != nullptr);
    CHECK(!InContext->IsRecording());

    InContext->SetLastUsedFrame(CurrentFrame.Load());
    CommandContextPool.Release(InContext);
}

void FMetalQueue::PruneCommandContexts(uint64 InCurrentFrame)
{
    CurrentFrame.Store(InCurrentFrame);

    const uint64 MaxIdleFrames = static_cast<uint64>(Math::Max<int32>(0, CVarCommandContextMaxIdleFrames.GetValue()));
    const int32  MinRetained   = Math::Max<int32>(0, CVarCommandContextMinRetained.GetValue());

    CommandContextPool.PruneFree(MinRetained, [InCurrentFrame, MaxIdleFrames](FMetalCommandContext* Context)
    {
        return (InCurrentFrame - Context->GetLastUsedFrame()) > MaxIdleFrames;
    });
}

uint64 FMetalQueue::SubmitCommands(FMetalCommands* Commands)
{
    CHECK(Commands != nullptr);
    CHECK(Commands->CommandBuffer != nil);

    const uint64 Value = NextSubmissionValue.Increment();
    Commands->SubmissionValue = Value;

    if (FMetalUploadHeapAllocator* UploadHeapAllocator = GetDevice()->GetUploadHeapAllocator())
    {
        UploadHeapAllocator->RetireAllocations(this, Value);
    }
    if (FMetalLinearAllocator* StagingBufferAllocator = GetDevice()->GetStagingBufferAllocator())
    {
        StagingBufferAllocator->RetireAllocations(this, Value);
    }
    if (FMetalLinearAllocator* DynamicConstantsAllocator = GetDevice()->GetDynamicConstantsAllocator())
    {
        DynamicConstantsAllocator->RetireAllocations(this, Value);
    }

    TArray<String> BreadcrumbCopy = Commands->Breadcrumbs;
    if (!Commands->DebugLabel.IsEmpty())
    {
        [Commands->CommandBuffer setLabel:Commands->DebugLabel.GetNSString()];
    }
    else
    {
        [Commands->CommandBuffer setLabel:[NSString stringWithFormat:@"MetalQueue-%llu", Value]];
    }

    [Commands->CommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> CompletedBuffer)
    {
        ReportCommandBufferError(CompletedBuffer, BreadcrumbCopy);
        ReportFunctionLogs(CompletedBuffer);
    }];

    Commands->EncodePendingWaits();
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
            Query->SubmittedQueue  = this;
        }
    }

    for (FMetalBufferRHI* Buffer : Commands->UsedBuffers)
    {
        if (Buffer)
        {
            Buffer->StampLastUse(this, Value);
        }
    }

    for (FMetalTextureRHI* Texture : Commands->UsedTextures)
    {
        if (Texture)
        {
            Texture->StampLastUse(this, Value);
        }
    }

    Commands->UsedBuffers.Clear();
    Commands->UsedTextures.Clear();

    STAT_ADD(STAT_Metal_CommandBufferCount, 1);
    PendingSubmissions.Enqueue(Commands);

    const int32 MaxPending = Math::Max(CVarMaxPendingSubmissions.GetValue(), 1);
    {
        TScopedLock Lock(ConsumerCS);

        while (PendingSubmissions.Size() > MaxPending)
        {
            FMetalCommands* Oldest = nullptr;
            if (PendingSubmissions.Peek(Oldest) && Oldest)
            {
                if (Oldest->SubmissionValue > 0 && SubmissionEvent && GetCompletedValue() < Oldest->SubmissionValue)
                {
                    [SubmissionEvent waitUntilSignaledValue:Oldest->SubmissionValue timeoutMS:UINT64_MAX];
                }

                PendingSubmissions.Dequeue();
                Oldest->PostExecute();
                RecycleCommands(Oldest);
            }
            else
            {
                break;
            }
        }
    }

    return Value;
}

void FMetalQueue::ProcessCommandQueue()
{
    TScopedLock Lock(ConsumerCS);
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
    , PendingWaits()
    , UsedBuffers()
    , UsedTextures()
    , Breadcrumbs()
    , DebugLabel()
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

void FMetalCommands::RecordBreadcrumb(const String& Name)
{
    if (Name.IsEmpty())
    {
        return;
    }

    if (DebugLabel.IsEmpty())
    {
        DebugLabel = Name;
    }

    if (Breadcrumbs.Size() >= MaxBreadcrumbs)
    {
        Breadcrumbs.RemoveAt(0);
    }

    Breadcrumbs.Emplace(Name);
}

void FMetalCommands::AddWait(FMetalQueue* Producer, uint64 Value)
{
    if (!Producer || Value == 0)
    {
        return;
    }

    id<MTLSharedEvent> Event = Producer->GetSubmissionEvent();
    if (!Event)
    {
        return;
    }

    for (FMetalQueueFence& Existing : PendingWaits)
    {
        if (Existing.Event == Event)
        {
            if (Value > Existing.Value)
            {
                Existing.Value    = Value;
                Existing.bEncoded = false;
            }
            return;
        }
    }

    FMetalQueueFence Fence;
    Fence.Event    = Event;
    Fence.Value    = Value;
    Fence.bEncoded = false;
    PendingWaits.Add(Fence);
}

void FMetalCommands::EncodePendingWaits()
{
    if (!CommandBuffer)
    {
        return;
    }

    for (FMetalQueueFence& Fence : PendingWaits)
    {
        if (Fence.bEncoded || !Fence.Event)
        {
            continue;
        }

        [CommandBuffer encodeWaitForEvent:Fence.Event value:Fence.Value];
        Fence.bEncoded = true;
    }
}

void FMetalCommands::PostExecute()
{
    FMetalQueryRHI::ResolveQueries(PendingQueries);
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
    , Queue(InDevice ? InDevice->GetQueue(EMetalQueueType::Copy) : nullptr)
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
    if (BlitEncoder)
    {
        BlitEncoder.label = @"Blit";
        Commands->RecordBreadcrumb("Blit");
    }
    METAL_ERROR_COND(BlitEncoder != nil, "Failed to create a blit encoder for an upload batch");
}

FMetalUploadBatch::~FMetalUploadBatch()
{
    Submit();
}

bool FMetalUploadBatch::CreateStagingBuffer(uint64 Size, FMetalResourceStorage& OutStorage)
{
    OutStorage.Reset();

    if (!Commands || Size == 0)
    {
        return false;
    }

    void* Mapped = Device->GetStagingBufferAllocator()->Allocate(Size, BUFFER_ALIGNMENT, Queue, OutStorage);
    if (!Mapped)
    {
        METAL_ERROR("Failed to allocate a %llu byte staging buffer", Size);
        OutStorage.Reset();
        return false;
    }

    return true;
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
