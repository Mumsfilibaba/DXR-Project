#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalDeviceDebug.h"
#include "MetalRHI/MetalQuery.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalStats.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalTexture.h"
#include "Core/Containers/String.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformFile.h"
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

static TAutoConsoleVariable<bool> CVarRetainedReferences(
    "MetalRHI.RetainedReferences",
    "When enabled, command buffers retain every object they reference. A diagnostic for a suspected use-after-free",
    false);

static TAutoConsoleVariable<bool> CVarEncoderExecutionStatus(
    "MetalRHI.EncoderExecutionStatus",
    "When enabled, a failed command buffer reports which of its encoders completed, faulted or never ran, each named by the scope path it opened under. On an AMD GPU this makes indirect mesh draws time out",
    false);

static TAutoConsoleVariable<String> CVarFaultReportFilePath(
    "MetalRHI.FaultReportFilePath",
    "File a failed command buffer's report is also written to, or empty to only log it",
    "MetalFaultReport.txt");

#if METAL_ENABLE_DEBUG_LAYER
static FAutoConsoleCommand CCmdForceGPUHang(
    "MetalRHI.ForceGPUHang",
    "Submits a kernel that never returns on the Direct queue, so the macOS GPU watchdog kills it, to verify the fault report",
    FConsoleCommandDelegate::CreateStatic(&MetalForceGPUHang));
#endif

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

static void ReportCommandBufferError(id<MTLCommandBuffer> CommandBuffer, const FMetalBreadcrumbRing& Breadcrumbs)
{
    if (!CommandBuffer || CommandBuffer.status != MTLCommandBufferStatusError)
    {
        return;
    }

    const String Label(CommandBuffer.label ? CommandBuffer.label : @"<unnamed>");

    TFileRef<IPlatformFile> File;
    const String FaultReportFilePath = CVarFaultReportFilePath.GetValue();
    if (!FaultReportFilePath.IsEmpty())
    {
        File = FPlatformFile::OpenForWrite(FaultReportFilePath);
    }

    const auto WriteLine = [&File](const String& Line)
    {
        LOG_ERROR("%s", *Line);

        if (File)
        {
            String Out = Line;
            Out += '\n';
            File->Write(reinterpret_cast<const uint8*>(*Out), static_cast<uint32>(Out.Size()));
        }
    };

    NSError* Error = CommandBuffer.error;

    if (!Error)
    {
        WriteLine(String::Printf("[MetalRHI] Command buffer '%s' failed without reporting an error", *Label));
        return;
    }

    const String Description(Error.localizedDescription);

    if ([Error.domain isEqualToString:MTLCommandBufferErrorDomain])
    {
        WriteLine(String::Printf("[MetalRHI] Command buffer '%s' failed with %s: %s", *Label, ToString(MTLCommandBufferError(Error.code)), *Description));
    }
    else
    {
        const String Domain(Error.domain);
        WriteLine(String::Printf("[MetalRHI] Command buffer '%s' failed with %s(%ld): %s", *Label, *Domain, long(Error.code), *Description));
    }

    NSArray<id<MTLCommandBufferEncoderInfo>>* EncoderInfos = Error.userInfo[MTLCommandBufferEncoderInfoErrorKey];

    NSString* LastCompleted = nil;
    NSString* FirstFaulted  = nil;
    for (id<MTLCommandBufferEncoderInfo> EncoderInfo in EncoderInfos)
    {
        if (EncoderInfo.errorState == MTLCommandEncoderErrorStateCompleted)
        {
            LastCompleted = EncoderInfo.label;
        }
        else if (!FirstFaulted && EncoderInfo.errorState == MTLCommandEncoderErrorStateFaulted)
        {
            FirstFaulted = EncoderInfo.label;
        }
    }

    if (EncoderInfos.count > 0)
    {
        WriteLine(String::Printf("[MetalRHI]   Last completed: '%s', faulted: '%s'",
            *String(LastCompleted ? LastCompleted : @"<none>"), *String(FirstFaulted ? FirstFaulted : @"<unknown>")));
    }

    for (id<MTLCommandBufferEncoderInfo> EncoderInfo in EncoderInfos)
    {
        const String EncoderLabel(EncoderInfo.label ? EncoderInfo.label : @"<unnamed>");
        WriteLine(String::Printf("[MetalRHI]   Encoder '%s': %s", *EncoderLabel, ToString(EncoderInfo.errorState)));

        for (NSString* Signpost in EncoderInfo.debugSignposts)
        {
            const String SignpostLabel(Signpost);
            WriteLine(String::Printf("[MetalRHI]     %s", *SignpostLabel));
        }
    }

    if (Breadcrumbs.Count > 0)
    {
        WriteLine("[MetalRHI]   Breadcrumbs:");
        Breadcrumbs.ForEach([&WriteLine](const CHAR* Name)
        {
            WriteLine(String::Printf("[MetalRHI]     %s", Name));
        });
    }
}

void FMetalBreadcrumbRing::Push(const StringView& Name)
{
    const uint32 Length = Math::Min<uint32>(static_cast<uint32>(Name.Length()), EntryLength - 1);
    Memory::Memcpy(Entries[Head], Name.Data(), Length);
    Entries[Head][Length] = '\0';

    Head  = (Head + 1) % NumEntries;
    Count = Math::Min(Count + 1, NumEntries);
}

FMetalQueue::FMetalQueue(FMetalDevice* InDevice, EMetalQueueType InQueueType)
    : FMetalDeviceChild(InDevice)
    , CommandQueue(nil)
    , CommandBufferDescriptor(nil)
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

    [EncoderFence.Fence release];
    EncoderFence.Fence = nil;

    [CommandBufferDescriptor release];
    CommandBufferDescriptor = nil;

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

    EncoderFence.Fence = [DeviceHandle newFence];

    if (!EncoderFence.Fence)
    {
        METAL_ERROR("Failed to create MTLFence");
        return false;
    }

    EncoderFence.Fence.label = @"MetalRHI.EncoderFence";

    CommandBufferDescriptor = [MTLCommandBufferDescriptor new];
    CommandBufferDescriptor.retainedReferences = CVarRetainedReferences.GetValue() ? YES : NO;
    CommandBufferDescriptor.errorOptions = CVarEncoderExecutionStatus.GetValue()
        ? MTLCommandBufferErrorOptionEncoderExecutionStatus
        : MTLCommandBufferErrorOptionNone;
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

    Commands->Reset();
    Commands->CommandBuffer = CreateCommandBuffer();
    return Commands;
}

id<MTLCommandBuffer> FMetalQueue::CreateCommandBuffer()
{
    SCOPED_AUTORELEASE_POOL();

    id<MTLCommandBuffer> CommandBuffer = [CommandQueue commandBufferWithDescriptor:CommandBufferDescriptor];

    if (CommandBuffer)
    {
        [CommandBuffer retain];
        STAT_ADD(STAT_Metal_CommandBuffersAlive, 1);
    }

    return CommandBuffer;
}

FMetalCommandContext* FMetalQueue::ObtainCommandContext()
{
    FMetalCommandContext* CommandContext = CommandContextPool.Acquire([this](int32) -> FMetalCommandContext*
    {
        return new FMetalCommandContext(GetDevice(), *this);
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

#if METAL_ENABLE_DEBUG_LAYER
    [Commands->CommandBuffer setLabel:[NSString stringWithFormat:@"MetalQueue-%llu", Value]];
#endif

    NSMutableArray<id<MTLSharedEvent>>* SignaledEvents = [[NSMutableArray alloc] initWithCapacity:Commands->PendingSignals.Size()];
    for (const FMetalEventValue& Signal : Commands->PendingSignals)
    {
        [SignaledEvents addObject:Signal.Event];
    }

    const FMetalBreadcrumbRing Breadcrumbs = Commands->Breadcrumbs;
    [Commands->CommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> CompletedBuffer)
    {
        ReportCommandBufferError(CompletedBuffer, Breadcrumbs);
        ReportFunctionLogs(CompletedBuffer);
        (void)SignaledEvents;
    }];
    [SignaledEvents release];

    Commands->EncodePendingWaits();
    Commands->Device->GetTimestampQueries().EncodeResolve(Commands->CommandBuffer, Commands->PendingQueries);

    for (const FMetalEventValue& Signal : Commands->PendingSignals)
    {
        [Commands->CommandBuffer encodeSignalEvent:Signal.Event value:Signal.Value];
    }

    [Commands->CommandBuffer encodeSignalEvent:SubmissionEvent value:Value];

    if (FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
    {
        BindlessManager->Flush();
    }

    GetDevice()->GetResidencyManager().PrepareForExecution(Commands->ResidencyList, *this, Value);
    GetDevice()->GetResidencySet().CommitIfDirty();
    [Commands->CommandBuffer commit];

    if (Commands->bUpdatesEncoderFence)
    {
        EncoderFence.bUpdateCommitted.Store(true);
    }

    for (FMetalQueryRHI* Query : Commands->PendingQueries)
    {
        if (Query)
        {
            Query->SubmissionValue = Value;
            Query->SubmittedQueue  = this;
        }
    }

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
    , PendingSignals()
    , PendingWaits()
    , RetiredStorage()
    , RetiredPins()
    , ResidencyList()
    , Breadcrumbs()
    , bUpdatesEncoderFence(false)
{
}

void FMetalCommands::Reset()
{
    CHECK(CommandBuffer == nil);

    SubmissionValue = 0;
    DeferredObjects.Clear();
    PendingQueries.Clear();
    PendingSignals.Clear();
    PendingWaits.Clear();
    ReleaseRetiredStorage();
    ResidencyList.Reset();
    Breadcrumbs.Reset();
    bUpdatesEncoderFence = false;
}

FMetalCommands::~FMetalCommands()
{
    if (CommandBuffer)
    {
        [CommandBuffer release];
        CommandBuffer = nil;

        STAT_SUBTRACT(STAT_Metal_CommandBuffersAlive, 1);
    }
}

void FMetalCommands::AddWait(const FMetalSyncPoint& SyncPoint)
{
    if (!SyncPoint.Queue || SyncPoint.Value == 0 || SyncPoint.Queue == Queue)
    {
        return;
    }

    for (FMetalSyncPoint& Existing : PendingWaits)
    {
        if (Existing.Queue == SyncPoint.Queue)
        {
            Existing.Value = Math::Max(Existing.Value, SyncPoint.Value);
            return;
        }
    }

    PendingWaits.Add(SyncPoint);
}

void FMetalCommands::EncodePendingWaits()
{
    if (!CommandBuffer)
    {
        return;
    }

    for (const FMetalSyncPoint& SyncPoint : PendingWaits)
    {
        [CommandBuffer encodeWaitForEvent:SyncPoint.Queue->GetSubmissionEvent() value:SyncPoint.Value];
    }

    PendingWaits.Clear();
}

void FMetalCommands::RetireStorage(TUniquePtr<FMetalResourceStorage> Storage, FMetalResidencyEntry* PinnedEntry)
{
    if (Storage)
    {
        RetiredStorage.Add(Move(Storage));
    }

    if (PinnedEntry)
    {
        RetiredPins.Add(PinnedEntry);
    }
}

void FMetalCommands::ReleaseRetiredStorage()
{
    FMetalResidencyManager& ResidencyManager = Device->GetResidencyManager();
    for (FMetalResidencyEntry* PinnedEntry : RetiredPins)
    {
        ResidencyManager.Unpin(PinnedEntry);
    }

    RetiredPins.Clear();
    RetiredStorage.Clear();
}

void FMetalCommands::PostExecute()
{
    FMetalQueryRHI::ResolveQueries(PendingQueries);
    FMetalDeferredObject::ProcessItems(DeferredObjects);
    DeferredObjects.Clear();
    ReleaseRetiredStorage();

    if (CommandBuffer)
    {
        [CommandBuffer release];
        CommandBuffer = nil;

        STAT_SUBTRACT(STAT_Metal_CommandBuffersAlive, 1);
    }
}
