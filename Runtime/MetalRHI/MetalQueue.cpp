#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalDevice.h"
#include "Core/Threading/ScopedLock.h"

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
    return Commands;
}

id<MTLCommandBuffer> FMetalQueue::CreateCommandBuffer()
{
    id<MTLCommandBuffer> CommandBuffer = [CommandQueue commandBuffer];
    if (CommandBuffer)
    {
        [CommandBuffer retain];
    }
    return CommandBuffer;
}

void FMetalQueue::SubmitCommands(FMetalCommands* Commands)
{
    CHECK(Commands != nullptr);
    CHECK(Commands->CommandBuffer != nil);

    const uint64 Value = NextSubmissionValue.Increment();
    Commands->SubmissionValue = Value;
    [Commands->CommandBuffer encodeSignalEvent:SubmissionEvent value:Value];
    [Commands->CommandBuffer commit];

    PendingSubmissions.Enqueue(Commands);
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
    FMetalDeferredObject::ProcessItems(DeferredObjects);
    DeferredObjects.Clear();

    if (CommandBuffer)
    {
        [CommandBuffer release];
        CommandBuffer = nil;
    }
}
