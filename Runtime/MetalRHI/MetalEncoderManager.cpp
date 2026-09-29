#include "MetalRHI/MetalEncoderManager.h"
#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalQuery.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalStats.h"

struct FMetalAbsoluteSubresource
{
    id<MTLTexture> RootTexture;
    NSUInteger     Level;
    NSUInteger     Slice;
};

static constexpr bool GQueueSupportsEncoder[static_cast<uint32>(EMetalQueueType::Count)][static_cast<uint32>(EMetalEncoderType::Count)] =
{
    // None, Render, Compute, Blit
    { true, true,  true,  true },
    { true, false, true,  true },
    { true, false, false, true },
};

static void AttachTimestamp(MTLRenderPassDescriptor* Descriptor, id<MTLCounterSampleBuffer> SampleBuffer, uint32 SampleIndex)
{
    MTLRenderPassSampleBufferAttachmentDescriptor* Attachment = Descriptor.sampleBufferAttachments[0];
    Attachment.sampleBuffer               = SampleBuffer;
    Attachment.startOfVertexSampleIndex   = SampleIndex;
    Attachment.endOfVertexSampleIndex     = MTLCounterDontSample;
    Attachment.startOfFragmentSampleIndex = MTLCounterDontSample;
    Attachment.endOfFragmentSampleIndex   = MTLCounterDontSample;
}

static FMetalAbsoluteSubresource GetAbsoluteSubresource(id<MTLTexture> Texture, NSUInteger Level, NSUInteger Slice)
{
    while (id<MTLTexture> Parent = Texture.parentTexture)
    {
        Level  += Texture.parentRelativeLevel;
        Slice  += Texture.parentRelativeSlice;
        Texture = Parent;
    }

    return { Texture, Level, Slice };
}

void FMetalEncoderFence::Wait(id<MTLCommandEncoder> Encoder, EMetalEncoderType Type, const FMetalCommands& Commands)
{
    if (!Commands.bUpdatesEncoderFence && !bUpdateCommitted.Load())
    {
        return;
    }

    if (Type == EMetalEncoderType::Render)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(Encoder) waitForFence:Fence beforeStages:MetalRHI::GetRenderStages()];
    }
    else if (Type == EMetalEncoderType::Compute)
    {
        [static_cast<id<MTLComputeCommandEncoder>>(Encoder) waitForFence:Fence];
    }
    else if (Type == EMetalEncoderType::Blit)
    {
        [static_cast<id<MTLBlitCommandEncoder>>(Encoder) waitForFence:Fence];
    }
}

void FMetalEncoderFence::Signal(id<MTLCommandEncoder> Encoder, EMetalEncoderType Type, FMetalCommands& Commands)
{
    if (Type == EMetalEncoderType::Render)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(Encoder) updateFence:Fence afterStages:MetalRHI::GetRenderStages()];
    }
    else if (Type == EMetalEncoderType::Compute)
    {
        [static_cast<id<MTLComputeCommandEncoder>>(Encoder) updateFence:Fence];
    }
    else if (Type == EMetalEncoderType::Blit)
    {
        [static_cast<id<MTLBlitCommandEncoder>>(Encoder) updateFence:Fence];
    }
    else
    {
        return;
    }

    Commands.bUpdatesEncoderFence = true;
}

FMetalEncoderManager::FMetalEncoderManager(FMetalQueue& InQueue)
    : FMetalDeviceChild(InQueue.GetDevice())
    , Queue(InQueue)
    , Commands(nullptr)
    , Encoder(nil)
    , EncoderType(EMetalEncoderType::None)
    , Fence(InQueue.GetEncoderFence())
    , BindingCache()
    , ScheduledTimestamps()
    , WaitedUploadValue(0)
    , DeclaredResidencyGeneration(0)
    , EncoderSerial(0)
    , bAdoptedEncoder(false)
{
}

FMetalEncoderManager::~FMetalEncoderManager()
{
    CHECK(Encoder == nil);
}

void FMetalEncoderManager::BeginCommandBuffer(FMetalCommands* InCommands)
{
    CHECK(Commands == nullptr);
    CHECK(InCommands != nullptr);

    Commands = InCommands;
}

FMetalCommands* FMetalEncoderManager::EndCommandBuffer()
{
    if (!Commands)
    {
        return nullptr;
    }

    EndEncoder();
    FlushPendingClears(nil);

    if (!Commands->DeferredObjects.IsEmpty())
    {
        WaitForPublishedUploads();
        Commands->EncodePendingWaits();
    }

    const int32 NumTimestamps = ScheduledTimestamps.Size();
    for (int32 Index = 0; Index < NumTimestamps; Index += 2)
    {
        EncodeTimestampPass(ScheduledTimestamps[Index], (Index + 1 < NumTimestamps) ? ScheduledTimestamps[Index + 1] : MTLCounterDontSample);
    }

    ScheduledTimestamps.Clear();

    FMetalCommands* Finished = Commands;
    Commands = nullptr;
    return Finished;
}

void FMetalEncoderManager::PrepareToOpen()
{
    CHECK(Commands != nullptr);

    EndEncoder();

    WaitForPublishedUploads();
    Commands->EncodePendingWaits();
}

void FMetalEncoderManager::WaitForPublishedUploads()
{
    if (Queue.GetType() == EMetalQueueType::Copy)
    {
        return;
    }

    const uint64 UploadValue = GetDevice()->GetLatestUploadValue();

    if (UploadValue > WaitedUploadValue)
    {
        Commands->AddWait(FMetalSyncPoint{ GetDevice()->GetQueue(EMetalQueueType::Copy), UploadValue });
        WaitedUploadValue = UploadValue;
    }
}

template<EMetalEncoderType Type, typename OpenFunctionType>
id<MTLCommandEncoder> FMetalEncoderManager::OpenEncoder(OpenFunctionType&& Open, const CHAR* Label)
{
    CHECK(GQueueSupportsEncoder[static_cast<uint32>(Queue.GetType())][static_cast<uint32>(Type)]);

    PrepareToOpen();

    Encoder = [Open() retain];

    if (!Encoder)
    {
        METAL_ERROR("Failed to open a Metal command encoder");
        return nil;
    }

    EncoderType     = Type;
    bAdoptedEncoder = false;
    EncoderSerial++;

    Fence.Wait(Encoder, Type, *Commands);

    if constexpr (Type == EMetalEncoderType::Render)
    {
        BindingCache.ResetRenderStages();
        PrepareBindless(static_cast<id<MTLRenderCommandEncoder>>(Encoder));
    }
    else if constexpr (Type == EMetalEncoderType::Compute)
    {
        BindingCache.ResetComputeStage();
        PrepareBindless(static_cast<id<MTLComputeCommandEncoder>>(Encoder));
    }

    if (Label)
    {
        Encoder.label = [NSString stringWithUTF8String:Label];
    }

    STAT_ADD(STAT_Metal_EncoderCount, 1);
    STAT_ADD(STAT_Metal_EncodersOpen, 1);
    return Encoder;
}

template<typename CommandEncoderType>
void FMetalEncoderManager::PrepareBindless(CommandEncoderType InEncoder)
{
    FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();

    if (!BindlessManager || !BindlessManager->IsEnabled())
    {
        return;
    }

    BindlessManager->Flush();

    FMetalResidencySet& ResidencySet = GetDevice()->GetResidencySet();

    if (ResidencySet.UsesEncoderFallback())
    {
        DeclaredResidencyGeneration = ResidencySet.DeclareForEncoder(InEncoder);
    }
}

template<typename CommandEncoderType>
void FMetalEncoderManager::RefreshBindlessResidency(CommandEncoderType InEncoder)
{
    FMetalResidencySet& ResidencySet = GetDevice()->GetResidencySet();

    if (ResidencySet.UsesEncoderFallback() && ResidencySet.GetFallbackGeneration() != DeclaredResidencyGeneration)
    {
        DeclaredResidencyGeneration = ResidencySet.DeclareForEncoder(InEncoder);
    }
}

template void FMetalEncoderManager::RefreshBindlessResidency(id<MTLRenderCommandEncoder>);
template void FMetalEncoderManager::RefreshBindlessResidency(id<MTLComputeCommandEncoder>);

id<MTLRenderCommandEncoder> FMetalEncoderManager::BeginRenderEncoder(MTLRenderPassDescriptor* Descriptor, const CHAR* Label)
{
    ResolvePendingClears(Descriptor);
    return OpenRenderEncoder(Descriptor, Label);
}

id<MTLRenderCommandEncoder> FMetalEncoderManager::OpenRenderEncoder(MTLRenderPassDescriptor* Descriptor, const CHAR* Label)
{
    return static_cast<id<MTLRenderCommandEncoder>>(OpenEncoder<EMetalEncoderType::Render>([this, Descriptor]() -> id<MTLCommandEncoder>
    {
        const uint32 SampleIndex = TakeScheduledTimestamp();

        if (SampleIndex != MetalInvalidQueryIndex)
        {
            AttachTimestamp(Descriptor, GetDevice()->GetTimestampQueries().GetSampleBuffer(), SampleIndex);
        }

        return [Commands->CommandBuffer renderCommandEncoderWithDescriptor:Descriptor];
    }, Label));
}

id<MTLRenderCommandEncoder> FMetalEncoderManager::AdoptRenderEncoder(id<MTLRenderCommandEncoder> SubEncoder, const CHAR* Label)
{
    CHECK(Commands != nullptr);
    CHECK(Encoder == nil);
    CHECK(SubEncoder != nil);

    Encoder         = [SubEncoder retain];
    EncoderType     = EMetalEncoderType::Render;
    bAdoptedEncoder = true;
    EncoderSerial++;

    BindingCache.ResetRenderStages();
    PrepareBindless(SubEncoder);

    if (Label)
    {
        Encoder.label = [NSString stringWithUTF8String:Label];
    }

    STAT_ADD(STAT_Metal_EncoderCount, 1);
    STAT_ADD(STAT_Metal_EncodersOpen, 1);
    return SubEncoder;
}

id<MTLComputeCommandEncoder> FMetalEncoderManager::RequireComputeEncoder()
{
    if (EncoderType == EMetalEncoderType::Compute)
    {
        return static_cast<id<MTLComputeCommandEncoder>>(Encoder);
    }

    return static_cast<id<MTLComputeCommandEncoder>>(OpenEncoder<EMetalEncoderType::Compute>([this]() -> id<MTLCommandEncoder>
    {
        const uint32 SampleIndex = TakeScheduledTimestamp();

        if (SampleIndex == MetalInvalidQueryIndex)
        {
            return [Commands->CommandBuffer computeCommandEncoder];
        }

        MTLComputePassDescriptor* Descriptor = [MTLComputePassDescriptor computePassDescriptor];
        MTLComputePassSampleBufferAttachmentDescriptor* Attachment = Descriptor.sampleBufferAttachments[0];
        Attachment.sampleBuffer              = GetDevice()->GetTimestampQueries().GetSampleBuffer();
        Attachment.startOfEncoderSampleIndex = SampleIndex;
        Attachment.endOfEncoderSampleIndex   = MTLCounterDontSample;
        return [Commands->CommandBuffer computeCommandEncoderWithDescriptor:Descriptor];
    }, "Compute"));
}

id<MTLBlitCommandEncoder> FMetalEncoderManager::RequireBlitEncoder()
{
    if (EncoderType == EMetalEncoderType::Blit)
    {
        return static_cast<id<MTLBlitCommandEncoder>>(Encoder);
    }

    return static_cast<id<MTLBlitCommandEncoder>>(OpenEncoder<EMetalEncoderType::Blit>([this]() -> id<MTLCommandEncoder>
    {
        const uint32 SampleIndex = TakeScheduledTimestamp();

        if (SampleIndex == MetalInvalidQueryIndex)
        {
            return [Commands->CommandBuffer blitCommandEncoder];
        }

        MTLBlitPassDescriptor* Descriptor = [MTLBlitPassDescriptor blitPassDescriptor];
        MTLBlitPassSampleBufferAttachmentDescriptor* Attachment = Descriptor.sampleBufferAttachments[0];
        Attachment.sampleBuffer              = GetDevice()->GetTimestampQueries().GetSampleBuffer();
        Attachment.startOfEncoderSampleIndex = SampleIndex;
        Attachment.endOfEncoderSampleIndex   = MTLCounterDontSample;
        return [Commands->CommandBuffer blitCommandEncoderWithDescriptor:Descriptor];
    }, "Blit"));
}

id<MTLParallelRenderCommandEncoder> FMetalEncoderManager::BeginParallelRenderEncoder(MTLRenderPassDescriptor* Descriptor, const CHAR* Label)
{
    CHECK(Queue.GetType() == EMetalQueueType::Direct);

    ResolvePendingClears(Descriptor);
    PrepareToOpen();

    const uint32 SampleIndex = TakeScheduledTimestamp();

    if (SampleIndex != MetalInvalidQueryIndex)
    {
        AttachTimestamp(Descriptor, GetDevice()->GetTimestampQueries().GetSampleBuffer(), SampleIndex);
    }

    id<MTLParallelRenderCommandEncoder> ParallelEncoder = [Commands->CommandBuffer parallelRenderCommandEncoderWithDescriptor:Descriptor];

    if (!ParallelEncoder)
    {
        METAL_ERROR("Failed to open a parallel render command encoder");
        return nil;
    }

    if (Label)
    {
        ParallelEncoder.label = [NSString stringWithUTF8String:Label];
    }

    id<MTLRenderCommandEncoder> HeadEncoder = [ParallelEncoder renderCommandEncoder];
    Fence.Wait(HeadEncoder, EMetalEncoderType::Render, *Commands);
    [HeadEncoder endEncoding];

    STAT_ADD(STAT_Metal_EncoderCount, 1);
    return ParallelEncoder;
}

void FMetalEncoderManager::EndParallelRenderEncoder(id<MTLParallelRenderCommandEncoder> ParallelEncoder)
{
    CHECK(ParallelEncoder != nil);

    id<MTLRenderCommandEncoder> TailEncoder = [ParallelEncoder renderCommandEncoder];
    Fence.Signal(TailEncoder, EMetalEncoderType::Render, *Commands);
    [TailEncoder endEncoding];

    [ParallelEncoder endEncoding];
}

void FMetalEncoderManager::EncodeLoadStorePass(MTLRenderPassDescriptor* Descriptor, const CHAR* Label)
{
    BeginRenderEncoder(Descriptor, Label);
    EndEncoder();
}

void FMetalEncoderManager::EndEncoder()
{
    if (!Encoder)
    {
        return;
    }

    if (!bAdoptedEncoder)
    {
        Fence.Signal(Encoder, EncoderType, *Commands);
    }

    [Encoder endEncoding];
    [Encoder release];

    Encoder         = nil;
    EncoderType     = EMetalEncoderType::None;
    bAdoptedEncoder = false;

    STAT_SUBTRACT(STAT_Metal_EncodersOpen, 1);
}

void FMetalEncoderManager::MemoryBarrier()
{
    static constexpr MTLBarrierScope Scope = MTLBarrierScopeBuffers | MTLBarrierScopeTextures;

    if (EncoderType == EMetalEncoderType::Compute)
    {
        [static_cast<id<MTLComputeCommandEncoder>>(Encoder) memoryBarrierWithScope:Scope];
    }
    else if (EncoderType == EMetalEncoderType::Render)
    {
        static constexpr MTLRenderStages Stages = MTLRenderStageVertex | MTLRenderStageFragment;
        [static_cast<id<MTLRenderCommandEncoder>>(Encoder) memoryBarrierWithScope:Scope afterStages:Stages beforeStages:Stages];
    }
}

void FMetalEncoderManager::AddPendingClear(MTLRenderPassAttachmentDescriptor* Attachment, const FMetalPendingClear& Clear)
{
    CHECK(Attachment.texture != nil);

    for (FMetalPendingClear& Existing : PendingClears)
    {
        if (Existing.Texture != Attachment.texture || Existing.Level != Attachment.level || Existing.Slice != Attachment.slice || Existing.DepthPlane != Attachment.depthPlane)
        {
            continue;
        }

        if (IsEnumFlagSet(Clear.Aspects, EMetalClearAspects::Color))
        {
            Existing.Color = Clear.Color;
        }

        if (IsEnumFlagSet(Clear.Aspects, EMetalClearAspects::Depth))
        {
            Existing.Depth = Clear.Depth;
        }

        if (IsEnumFlagSet(Clear.Aspects, EMetalClearAspects::Stencil))
        {
            Existing.Stencil = Clear.Stencil;
        }

        Existing.Aspects |= Clear.Aspects;
        return;
    }

    const FMetalAbsoluteSubresource Subresource = GetAbsoluteSubresource(Attachment.texture, Attachment.level, Attachment.slice);

    FMetalPendingClear& Added = PendingClears.Add(Clear);
    Added.Texture     = Attachment.texture;
    Added.Level       = Attachment.level;
    Added.Slice       = Attachment.slice;
    Added.DepthPlane  = Attachment.depthPlane;
    Added.RootTexture = Subresource.RootTexture;
    Added.RootLevel   = Subresource.Level;
    Added.RootSlice   = Subresource.Slice;
}

void FMetalEncoderManager::FlushPendingClears(id<MTLTexture> Texture)
{
    if (PendingClears.IsEmpty())
    {
        return;
    }

    id<MTLTexture> RootTexture = Texture ? GetAbsoluteSubresource(Texture, 0, 0).RootTexture : nil;
    for (int32 Index = 0; Index < PendingClears.Size();)
    {
        if (RootTexture && PendingClears[Index].RootTexture != RootTexture)
        {
            ++Index;
            continue;
        }

        const FMetalPendingClear Clear = PendingClears[Index];
        PendingClears.RemoveAt(Index);
        EncodePendingClear(Clear);
    }
}

void FMetalEncoderManager::ResolvePendingClears(MTLRenderPassDescriptor* Descriptor)
{
    if (PendingClears.IsEmpty())
    {
        return;
    }

    struct FAttachment
    {
        MTLRenderPassAttachmentDescriptor* Descriptor;
        EMetalClearAspects                 Aspect;
    };

    FAttachment Attachments[RHI_MAX_RENDER_TARGETS + 2];
    uint32      NumAttachments = 0;
    for (uint32 Slot = 0; Slot < RHI_MAX_RENDER_TARGETS; ++Slot)
    {
        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = Descriptor.colorAttachments[Slot];

        if (ColorAttachment.texture)
        {
            Attachments[NumAttachments++] = { ColorAttachment, EMetalClearAspects::Color };
        }
    }

    if (Descriptor.depthAttachment.texture)
    {
        Attachments[NumAttachments++] = { Descriptor.depthAttachment, EMetalClearAspects::Depth };
    }

    if (Descriptor.stencilAttachment.texture)
    {
        Attachments[NumAttachments++] = { Descriptor.stencilAttachment, EMetalClearAspects::Stencil };
    }

    const bool bLayered = Descriptor.renderTargetArrayLength > 1;
    for (int32 Index = 0; Index < PendingClears.Size();)
    {
        FMetalPendingClear& Clear     = PendingClears[Index];
        bool                bObserved = false;

        for (uint32 AttachmentIndex = 0; AttachmentIndex < NumAttachments; ++AttachmentIndex)
        {
            MTLRenderPassAttachmentDescriptor* Attachment  = Attachments[AttachmentIndex].Descriptor;
            const FMetalAbsoluteSubresource    Subresource = GetAbsoluteSubresource(Attachment.texture, Attachment.level, Attachment.slice);

            if (Subresource.RootTexture != Clear.RootTexture || Subresource.Level != Clear.RootLevel)
            {
                continue;
            }

            const bool bSameAttachment = !bLayered && Attachment.texture == Clear.Texture && Attachment.slice == Clear.Slice && Attachment.depthPlane == Clear.DepthPlane;
            if (!bSameAttachment)
            {
                bObserved |= bLayered || (Subresource.Slice == Clear.RootSlice && Attachment.depthPlane == Clear.DepthPlane);
                continue;
            }

            const EMetalClearAspects Aspect = Attachments[AttachmentIndex].Aspect;
            if (!IsEnumFlagSet(Clear.Aspects, Aspect))
            {
                continue;
            }

            if (Attachment.loadAction == MTLLoadActionLoad)
            {
                Attachment.loadAction = MTLLoadActionClear;

                if (Aspect == EMetalClearAspects::Color)
                {
                    static_cast<MTLRenderPassColorAttachmentDescriptor*>(Attachment).clearColor = Clear.Color;
                }
                else if (Aspect == EMetalClearAspects::Depth)
                {
                    static_cast<MTLRenderPassDepthAttachmentDescriptor*>(Attachment).clearDepth = Clear.Depth;
                }
                else
                {
                    static_cast<MTLRenderPassStencilAttachmentDescriptor*>(Attachment).clearStencil = Clear.Stencil;
                }
            }

            Clear.Aspects &= ~Aspect;
        }

        if (bObserved || Clear.Aspects == EMetalClearAspects::None)
        {
            const FMetalPendingClear Remaining = Clear;
            PendingClears.RemoveAt(Index);

            if (Remaining.Aspects != EMetalClearAspects::None)
            {
                EncodePendingClear(Remaining);
            }

            continue;
        }

        ++Index;
    }
}

void FMetalEncoderManager::EncodePendingClear(const FMetalPendingClear& Clear)
{
    SCOPED_AUTORELEASE_POOL();

    MTLRenderPassDescriptor* Descriptor = [MTLRenderPassDescriptor renderPassDescriptor];

    const auto ApplyClear = [&Clear](MTLRenderPassAttachmentDescriptor* Attachment)
    {
        Attachment.texture     = Clear.Texture;
        Attachment.level       = Clear.Level;
        Attachment.slice       = Clear.Slice;
        Attachment.depthPlane  = Clear.DepthPlane;
        Attachment.loadAction  = MTLLoadActionClear;
        Attachment.storeAction = MTLStoreActionStore;
    };

    if (IsEnumFlagSet(Clear.Aspects, EMetalClearAspects::Color))
    {
        ApplyClear(Descriptor.colorAttachments[0]);
        Descriptor.colorAttachments[0].clearColor = Clear.Color;
    }

    if (IsEnumFlagSet(Clear.Aspects, EMetalClearAspects::Depth))
    {
        ApplyClear(Descriptor.depthAttachment);
        Descriptor.depthAttachment.clearDepth = Clear.Depth;
    }

    if (IsEnumFlagSet(Clear.Aspects, EMetalClearAspects::Stencil))
    {
        ApplyClear(Descriptor.stencilAttachment);
        Descriptor.stencilAttachment.clearStencil = Clear.Stencil;
    }

    OpenRenderEncoder(Descriptor, "Clear");
    EndEncoder();
}

void FMetalEncoderManager::ScheduleTimestamp(uint32 SampleIndex)
{
    if (EncoderType != EMetalEncoderType::Render)
    {
        EndEncoder();
    }

    ScheduledTimestamps.Add(SampleIndex);
}

#if !METAL_ASSUME_APPLE_GPU
void FMetalEncoderManager::SampleCounters(uint32 SampleIndex)
{
    id<MTLCounterSampleBuffer> SampleBuffer = GetDevice()->GetTimestampQueries().GetSampleBuffer();

    if (EncoderType == EMetalEncoderType::Render && GMetalFeatures.bDrawBoundaryTimestamps)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(Encoder) sampleCountersInBuffer:SampleBuffer atSampleIndex:SampleIndex withBarrier:YES];
        return;
    }

    if (EncoderType == EMetalEncoderType::Compute && GMetalFeatures.bDispatchBoundaryTimestamps)
    {
        [static_cast<id<MTLComputeCommandEncoder>>(Encoder) sampleCountersInBuffer:SampleBuffer atSampleIndex:SampleIndex withBarrier:YES];
        return;
    }

    if (EncoderType == EMetalEncoderType::Render)
    {
        METAL_WARNING("This GPU cannot sample timestamps inside a render pass, the query will read zero");
        return;
    }

    const bool bQueueHasCompute = GQueueSupportsEncoder[static_cast<uint32>(Queue.GetType())][static_cast<uint32>(EMetalEncoderType::Compute)];

    if (GMetalFeatures.bBlitBoundaryTimestamps)
    {
        [RequireBlitEncoder() sampleCountersInBuffer:SampleBuffer atSampleIndex:SampleIndex withBarrier:YES];
    }
    else if (GMetalFeatures.bDispatchBoundaryTimestamps && bQueueHasCompute)
    {
        [RequireComputeEncoder() sampleCountersInBuffer:SampleBuffer atSampleIndex:SampleIndex withBarrier:YES];
    }
}
#endif

void FMetalEncoderManager::EncodeTimestampPass(uint32 StartIndex, uint32 EndIndex)
{
    CHECK(Encoder == nil);

    MTLBlitPassDescriptor* Descriptor = [MTLBlitPassDescriptor blitPassDescriptor];
    MTLBlitPassSampleBufferAttachmentDescriptor* Attachment = Descriptor.sampleBufferAttachments[0];
    Attachment.sampleBuffer              = GetDevice()->GetTimestampQueries().GetSampleBuffer();
    Attachment.startOfEncoderSampleIndex = StartIndex;
    Attachment.endOfEncoderSampleIndex   = EndIndex;

    id<MTLBlitCommandEncoder> TimestampEncoder = [Commands->CommandBuffer blitCommandEncoderWithDescriptor:Descriptor];
    [TimestampEncoder endEncoding];
}

uint32 FMetalEncoderManager::TakeScheduledTimestamp()
{
    const int32 NumTimestamps = ScheduledTimestamps.Size();

    if (NumTimestamps == 0)
    {
        return MetalInvalidQueryIndex;
    }

    const int32 NumCarried = NumTimestamps - 1;
    for (int32 Index = 0; Index < NumCarried; Index += 2)
    {
        EncodeTimestampPass(ScheduledTimestamps[Index], (Index + 1 < NumCarried) ? ScheduledTimestamps[Index + 1] : MTLCounterDontSample);
    }

    const uint32 SampleIndex = ScheduledTimestamps[NumCarried];
    ScheduledTimestamps.Clear();
    return SampleIndex;
}
