#include "MetalRHI/MetalUploadBatch.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalResource.h"
#include "MetalRHI/MetalStats.h"
#include "MetalRHI/MetalTexture.h"

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
        STAT_ADD(STAT_Metal_EncodersOpen, 1);

        BlitEncoder.label = @"Upload";
        Commands->Breadcrumbs.Push("Upload");
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

void FMetalUploadBatch::InitializePlacement(id<MTLTexture> Texture)
{
    SCOPED_AUTORELEASE_POOL();

    TArray<MTLRenderPassDescriptor*> Passes;
    MetalRHI::CreatePlacementInitPasses(Texture, Passes);

    if (!BlitEncoder || Passes.IsEmpty())
    {
        return;
    }

    FMetalEncoderFence& Fence = Queue->GetEncoderFence();
    Fence.Signal(BlitEncoder, EMetalEncoderType::Blit, *Commands);

    [BlitEncoder endEncoding];
    [BlitEncoder release];

    for (MTLRenderPassDescriptor* Pass : Passes)
    {
        id<MTLRenderCommandEncoder> RenderEncoder = [Commands->CommandBuffer renderCommandEncoderWithDescriptor:Pass];
        RenderEncoder.label = @"InitializePlacement";

        Fence.Wait(RenderEncoder, EMetalEncoderType::Render, *Commands);
        Fence.Signal(RenderEncoder, EMetalEncoderType::Render, *Commands);

        [RenderEncoder endEncoding];
    }

    BlitEncoder = [[Commands->CommandBuffer blitCommandEncoder] retain];
    BlitEncoder.label = @"Upload";
    Fence.Wait(BlitEncoder, EMetalEncoderType::Blit, *Commands);
}

uint64 FMetalUploadBatch::Submit()
{
    if (BlitEncoder)
    {
        [BlitEncoder endEncoding];
        [BlitEncoder release];
        BlitEncoder = nil;

        STAT_SUBTRACT(STAT_Metal_EncodersOpen, 1);
    }

    if (!Commands)
    {
        return 0;
    }

    const uint64 SubmissionValue = Queue->SubmitCommands(Commands);
    Commands = nullptr;

    Device->PublishUploadValue(SubmissionValue);
    return SubmissionValue;
}
