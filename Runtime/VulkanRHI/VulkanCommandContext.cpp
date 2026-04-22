#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanDeviceDebug.h"

static TAutoConsoleVariable<int32> CVarMaxCommandsPerCommandBuffer(
    "VulkanRHI.MaxCommandsPerCommandBuffer",
    "Number of commands allowed before submitting the current CommandBuffer to the GPU",
    10000);

static TAutoConsoleVariable<bool> CVarTimestampTopOfPipe(
    "VulkanRHI.TimestampTopOfPipe",
    "Use top-of-pipe timestamps instead of bottom-of-pipe",
    false);

#if VULKAN_ENABLE_CRASH_MARKERS
static TAutoConsoleVariable<int32> CVarVulkanCrashMarkerLevel(
    "VulkanRHI.CrashMarkerLevel",
    "GPU crash marker tracking level. 0=off, 1=markers only, 2=per draw/dispatch. Requires VK_AMD_buffer_marker or VK_NV_device_diagnostic_checkpoints.",
    1);

static int32 GetCrashMarkerLevel()
{
    return CVarVulkanCrashMarkerLevel.GetValue();
}
#endif

static constexpr bool GVulkanEnableNegativeViewportHeight = true;

void FVulkanBarrierBatcher::AddMemoryBarrier(VkDependencyFlags DependencyFlags, const VkMemoryBarrier2& InBarrier)
{
    CHECK(InBarrier.sType == VK_STRUCTURE_TYPE_MEMORY_BARRIER_2);

    for (FBatch& Batch : Batches)
    {
        if (Batch.DependencyFlags == DependencyFlags)
        {
            Batch.MemoryBarriers.Add(InBarrier);
            return;
        }
    }

    FBatch& Batch = Batches.Emplace(DependencyFlags);
    Batch.MemoryBarriers.Add(InBarrier);
}

void FVulkanBarrierBatcher::AddBufferMemoryBarrier(VkDependencyFlags DependencyFlags, const VkBufferMemoryBarrier2& InBarrier)
{
    CHECK(InBarrier.sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2);
    CHECK(InBarrier.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
    CHECK(InBarrier.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);

    for (FBatch& Batch : Batches)
    {
        if (Batch.DependencyFlags == DependencyFlags)
        {
            Batch.BufferMemoryBarriers.Add(InBarrier);
            return;
        }
    }

    FBatch& Batch = Batches.Emplace(DependencyFlags);
    Batch.BufferMemoryBarriers.Add(InBarrier);
}

void FVulkanBarrierBatcher::AddImageMemoryBarrier(VkDependencyFlags DependencyFlags, const VkImageMemoryBarrier2& InBarrier)
{
    CHECK(InBarrier.sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2);
    CHECK(InBarrier.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
    CHECK(InBarrier.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);

    for (FBatch& Batch : Batches)
    {
        if (Batch.DependencyFlags == DependencyFlags)
        {
            // Coalesce with existing barrier if same image + subresource range
            for (VkImageMemoryBarrier2& Barrier : Batch.ImageMemoryBarriers)
            {
                if (Barrier.image == InBarrier.image)
                {
                    const VkImageSubresourceRange& RangeA = Barrier.subresourceRange;
                    const VkImageSubresourceRange& RangeB = InBarrier.subresourceRange;

                    const bool bSameRange = (RangeA.aspectMask == RangeB.aspectMask && 
                        RangeA.baseMipLevel == RangeB.baseMipLevel && 
                        RangeA.levelCount == RangeB.levelCount && 
                        RangeA.baseArrayLayer == RangeB.baseArrayLayer && 
                        RangeA.layerCount == RangeB.layerCount);

                    if (bSameRange)
                    {
                        // Keep original oldLayout, advance to the latest newLayout.
                        Barrier.newLayout = InBarrier.newLayout;

                        // Be conservative. Union access + stage masks.
                        Barrier.srcAccessMask |= InBarrier.srcAccessMask;
                        Barrier.dstAccessMask |= InBarrier.dstAccessMask;
                        Barrier.srcStageMask  |= InBarrier.srcStageMask;
                        Barrier.dstStageMask  |= InBarrier.dstStageMask;
                        return;
                    }
                }
            }

            // ...otherwise add a new barrier
            Batch.ImageMemoryBarriers.Add(InBarrier);
            return;
        }
    }

    FBatch& Batch = Batches.Emplace(DependencyFlags);
    Batch.ImageMemoryBarriers.Add(InBarrier);
}

void FVulkanBarrierBatcher::FlushBarriers(FVulkanCommandBuffer& CommandBuffer)
{
    VkDependencyInfo DependencyInfo = {};
    DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

    for (FBatch& Batch : Batches)
    {
        DependencyInfo.pMemoryBarriers          = Batch.MemoryBarriers.Data();
        DependencyInfo.memoryBarrierCount       = Batch.MemoryBarriers.Size();
        DependencyInfo.pImageMemoryBarriers     = Batch.ImageMemoryBarriers.Data();
        DependencyInfo.imageMemoryBarrierCount  = Batch.ImageMemoryBarriers.Size();
        DependencyInfo.pBufferMemoryBarriers    = Batch.BufferMemoryBarriers.Data();
        DependencyInfo.bufferMemoryBarrierCount = Batch.BufferMemoryBarriers.Size();
        DependencyInfo.dependencyFlags          = Batch.DependencyFlags;

        CommandBuffer->PipelineBarrier2(&DependencyInfo);
    }

    Batches.Clear();
}

FVulkanCommandContext::FVulkanCommandContext(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : FVulkanDeviceChild(InDevice)
    , Queue(InQueue)
    , CommandPool(nullptr)
    , CommandBuffer(nullptr)
    , Commands(nullptr)
    , TimestampQueryAllocator(InDevice, VK_QUERY_TYPE_TIMESTAMP)
    , OcclusionQueryAllocator(InDevice, VK_QUERY_TYPE_OCCLUSION)
    , PipelineStatsQueryAllocator(InDevice, VK_QUERY_TYPE_PIPELINE_STATISTICS)
    , ContextState(InDevice, *this)
    , ActiveQueryCount(0)
    , TransientDescriptorAllocator(nullptr)
{
#if !VULKAN_USE_DESCRIPTOR_CACHE
    TransientDescriptorAllocator = new FVulkanTransientDescriptorAllocator(InDevice, InDevice->GetDescriptorPoolManager());
#endif
}

FVulkanCommandContext::~FVulkanCommandContext()
{
    ContextState.ResetState();
    SAFE_DELETE(TransientDescriptorAllocator);
}

bool FVulkanCommandContext::Initialize()
{
    if (!ContextState.Initialize())
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize ContextState");
        return false;
    }

    return true;
}

void FVulkanCommandContext::BeginFrame()
{
    FVulkanRHI::Get()->BeginFrame();
}

void FVulkanCommandContext::EndFrame()
{
    FVulkanRHI::Get()->EndFrame();
}

void FVulkanCommandContext::ObtainCommandBuffer()
{
    TRACE_FUNCTION_SCOPE();

    if (!CommandPool)
    {
        CommandPool = Queue.ObtainCommandPool();
        if (!CommandPool)
        {
            VULKAN_ERROR_CRITICAL("Failed to Obtain CommandPool");
            return;
        }
    }
    
    // At this point we cannot have a valid CommandBuffer
    if (!CommandBuffer)
    {
        CommandBuffer = CommandPool->GetOrCreateBuffer();
        if (!CommandBuffer)
        {
            VULKAN_ERROR_CRITICAL("Failed to Obtain CommandBuffer");
            return;
        }

        // Begin to record to this CommandBuffer
        const VkCommandBufferUsageFlags Flags = GVulkanAllowResetCommandBuffers ? 0 : VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (!CommandBuffer->Begin(Flags))
        {
            VULKAN_ERROR_CRITICAL("Failed to Begin CommandBuffer");
        }

        CommandBuffer->InsertBeginTimestamp(TimestampQueryAllocator);

        ReopenEventStack();
    }

    if (!Commands)
    {
        Commands = new FVulkanCommands(GetDevice(), Queue);
        Commands->AcquireFence();
    }
}

FVulkanImageLayoutState& FVulkanCommandContext::RetrievePendingImageState(FVulkanTexture* Texture)
{
    CHECK(Texture != nullptr);

    FVulkanImageLayoutState& LocalState = PendingImageStates.FindOrAdd(Texture);
    if (!LocalState.IsInitialized())
    {
        const VkImageCreateInfo& CreateInfo = Texture->GetVkImageCreateInfo();

        const uint32 NumSubresources = CreateInfo.arrayLayers * CreateInfo.mipLevels;
        LocalState.Initialize(NumSubresources);
        LocalState.SetImageLayout(VK_IMAGE_LAYOUT_TO_BE_DETERMINED);
    }

    return LocalState;
}

FVulkanBufferState& FVulkanCommandContext::RetrievePendingBufferState(FVulkanBuffer* Buffer)
{
    CHECK(Buffer != nullptr);

    FVulkanBufferState& LocalState = PendingBufferStates.FindOrAdd(Buffer);
    if (LocalState.GetAccess() == 0 && LocalState.GetStage() == 0)
    {
        LocalState.SetState(VK_ACCESS_FLAGS_2_TO_BE_DETERMINED, VK_PIPELINE_STAGE_FLAGS_2_TO_BE_DETERMINED);
    }

    return LocalState;
}

void FVulkanCommandContext::FinishCommandBuffer(bool bFlushPool, bool bResolveQueries, FVulkanFence** OutFence)
{
    CHECK(CommandBuffer != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    CommandBuffer->InsertEndTimestamp(TimestampQueryAllocator);

    const uint32 NumCommands = CommandBuffer->GetNumCommands();
    if (NumCommands == 0)
    {
        PendingImageBarriers.Clear();
        PendingBufferBarriers.Clear();
        PendingImageStates.Clear();
        PendingBufferStates.Clear();
        ContextState.ResetStateForNewCommandBuffer();

        CommandBuffer->End();
        CommandPool->RecycleBuffer(CommandBuffer);
        CommandBuffer = nullptr;

        FVulkanFenceManager& FenceManager = GetDevice()->GetFenceManager();
        FenceManager.RecycleFence(Commands->Fence);
        Commands->Fence = nullptr;
        delete Commands;
        Commands = nullptr;

        if (OutFence)
        {
            *OutFence = nullptr;
        }
        
        return;
    }

    CloseEventStack();

#if VULKAN_USE_CPU_QUERY_RESOLVE
    TimestampQueryAllocator.Reset(Commands->QueryRanges);
    OcclusionQueryAllocator.Reset(Commands->QueryRanges);
    PipelineStatsQueryAllocator.Reset(Commands->QueryRanges);
#else
    if (bResolveQueries)
    {
        TimestampQueryAllocator.Reset(Commands->QueryRanges);
        OcclusionQueryAllocator.Reset(Commands->QueryRanges);
        PipelineStatsQueryAllocator.Reset(Commands->QueryRanges);

        Commands->Flags |= EVulkanCommandsFlags::ResolveQueries;

        TArray<FVulkanQueryRange>& AllRanges = Commands->QueryRanges;

        AllRanges.SortWithPredicate([](const FVulkanQueryRange& FirstRange, const FVulkanQueryRange& SecondRange)
        {
            if (FirstRange.Pool != SecondRange.Pool)
            {
                return reinterpret_cast<uintptr_t>(FirstRange.Pool) < reinterpret_cast<uintptr_t>(SecondRange.Pool);
            }

            return FirstRange.StartIndex < SecondRange.StartIndex;
        });

        for (int32 i = 0; i < AllRanges.Size(); )
        {
            const FVulkanQueryRange& Range = AllRanges[i];
            if (Range.Count <= 0 || !Range.Pool)
            {
                i++;
                continue;
            }

            FVulkanQueryPool* Pool = Range.Pool;
            int32 MergedStart = Range.StartIndex;
            int32 MergedEnd   = MergedStart + Range.Count;

            int32 j = i + 1;
            while (j < AllRanges.Size() && AllRanges[j].Pool == Pool && AllRanges[j].StartIndex <= MergedEnd)
            {
                int32 RangeEnd = AllRanges[j].StartIndex + AllRanges[j].Count;
                if (RangeEnd > MergedEnd)
                {
                    MergedEnd = RangeEnd;
                }
                j++;
            }

            const uint64 Stride = Pool->GetQuerySize();
            const VkDeviceSize BaseOffset = Pool->GetReadbackBufferOffset();
            GetCommandBuffer()->CopyQueryPoolResults(
                Pool->GetVkQueryPool(),
                MergedStart,
                MergedEnd - MergedStart,
                Pool->GetReadbackBuffer(),
                BaseOffset + MergedStart * Stride,
                Stride,
                VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

            i = j;
        }
    }
#endif

    if (!CommandBuffer->End())
    {
        VULKAN_ERROR_CRITICAL("Failed to End CommandBuffer");
    }

    Commands->PendingImageBarriers  = Move(PendingImageBarriers);
    Commands->PendingBufferBarriers = Move(PendingBufferBarriers);
    Commands->PendingImageStates    = Move(PendingImageStates);
    Commands->PendingBufferStates   = Move(PendingBufferStates);

    if (TransientDescriptorAllocator)
    {
        TransientDescriptorAllocator->FlushPools();
    }

    Commands->AddCommandBuffer(CommandBuffer);
    CommandBuffer = nullptr;

    if (bFlushPool)
    {
        Commands->AddCommandPool(CommandPool);
        CommandPool = nullptr;
    }

    Commands->PendingQueries = Move(PendingQueries);

    if (OutFence)
    {
        *OutFence = Commands->Fence;
    }

    FVulkanRHI::Get()->FlushDeletionQueue(Commands);
    Commands->Queue.SubmitCommands(Commands);
    Commands = nullptr;

    ContextState.ResetStateForNewCommandBuffer();
}

void FVulkanCommandContext::SplitCommandBuffer(bool bFlushPool, bool bWaitForQueue)
{
    if (CommandBuffer)
    {
    #if VULKAN_ENABLE_CRASH_MARKERS
        if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 1)
        {
            FVulkanRHI::Get()->GetCrashMarkers()->WriteSplitMarker(GetCommandBuffer());
        }
    #endif

        FinishCommandBuffer(bFlushPool, false);
    }
    
    if (bWaitForQueue)
    {
        GetCommandQueue().WaitForCompletion();
    }

    ObtainCommandBuffer();
}

void FVulkanCommandContext::ConditionalSplitCommandBuffer()
{
    if (ActiveQueryCount > 0)
    {
        return;
    }

    const uint32 MaxCommands = static_cast<uint32>(CVarMaxCommandsPerCommandBuffer.GetValue());
    const uint32 NumCommands = CommandBuffer->GetNumCommands();
    
    if (NumCommands >= MaxCommands)
    {
        const bool bWasInsideRenderPass = IsInsideRenderPass();
        if (bWasInsideRenderPass)
        {
            ContextState.PauseRenderPass();
        }

        SplitCommandBuffer(true, false);

        if (bWasInsideRenderPass)
        {
            ContextState.ResumeRenderPass();
        }
    }
}

void FVulkanCommandContext::ForceFlushCommandPool()
{
    // -------------------------------------------------------------------------------------------
    // Forces submission of the current command-pool to the active command payload. This is 
    // necessary because, at the end of a frame, there may be no further commands to submit after 
    // the Present() call. In such cases, FinishContext() will not flush or reset the 
    // command-pool, causing it to accumulate memory allocations across frames.
    //
    // By explicitly retiring the current command pool here, we ensure that command-buffers are 
    // released and the pool is properly recycled, even in frames with minimal or no recorded 
    // GPU work.
    // -------------------------------------------------------------------------------------------

    if (!CommandPool)
    {
        return;
    }

    if (Commands)
    {
        Commands->AddCommandPool(CommandPool);
        CommandPool = nullptr;
    }
}

void FVulkanCommandContext::StartContext()
{
    // -------------------------------------------------------------------------------------------
    // NOTE: This context is intended to be used from a single thread. The lock only enforces 
    // that the same thread which starts the context is the one that later finishes it. Once 
    // the codebase guarantees single-threaded use per context, this lock can be removed.
    // -------------------------------------------------------------------------------------------

    CommandContextCS.Lock();

    // -------------------------------------------------------------------------------------------
    // Phase Transition: Finished -> Recording
    // -------------------------------------------------------------------------------------------
    
    ContextState.OnStartRecording();

    // -------------------------------------------------------------------------------------------
    // Clear cached bindings, barriers, and any transient state accumulated in the previous 
    // frame/phase.
    // -------------------------------------------------------------------------------------------
    
    ContextState.ResetState();
    EventStack.Clear();

    // -------------------------------------------------------------------------------------------
    // Acquire/allocate a fresh command buffer so the caller can immediately begin recording 
    // GPU work in this context.
    // -------------------------------------------------------------------------------------------
    
    ObtainCommandBuffer();

#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() > 0)
    {
        FVulkanRHI::Get()->GetCrashMarkers()->ResetMarkers(GetCommandBuffer());
    }
#endif
}

void FVulkanCommandContext::FinishContext()
{
    // -------------------------------------------------------------------------------------------
    // Phase Validation
    // -------------------------------------------------------------------------------------------

    CHECK(ContextState.IsRecording());

    // -------------------------------------------------------------------------------------------
    // Finish the active command-buffer and request pool retirement/reset. We want one 
    // command-pool per context per frame-in-flight. The actual pool retirement happens as part 
    // of submit/payload path if there are commands to submit.
    // -------------------------------------------------------------------------------------------

    FinishCommandBuffer(true);

    // -------------------------------------------------------------------------------------------
    // In frames where Present() is the last operation and no additional commands are 
    // recorded/submitted, ensure we don't keep accumulating command-buffers in the pool across 
    // frames by retiring the pool here.
    // -------------------------------------------------------------------------------------------

    ForceFlushCommandPool();

    // -------------------------------------------------------------------------------------------
    // Phase Transition: Recording -> Finished
    // -------------------------------------------------------------------------------------------

    ContextState.OnFinishRecording();

    // -------------------------------------------------------------------------------------------
    // See note in StartContext(): once guaranteed single-threaded use is enforced by design, 
    // this lock can be removed.
    // -------------------------------------------------------------------------------------------
    
    CommandContextCS.Unlock();
}

void FVulkanCommandContext::BeginQuery(FRHIQuery* Query)
{
    FVulkanQueryRHI* VulkanQuery = FVulkanRHI::ResourceCast(Query);
    CHECK(VulkanQuery != nullptr);

    const EQueryType Type = VulkanQuery->GetType();
    if (Type == EQueryType::Occlusion)
    {
        OcclusionQueryAllocator.Allocate(VulkanQuery->CurrentQuery, VulkanQuery->QueryResult, EVulkanQueryType::Occlusion);
    }
    else if (Type == EQueryType::PipelineStatistics)
    {
        PipelineStatsQueryAllocator.Allocate(VulkanQuery->CurrentQuery, VulkanQuery->QueryResult, EVulkanQueryType::PipelineStatistics);
    }
    else
    {
        VULKAN_ERROR_CRITICAL("BeginQuery is not supported for this query type");
        return;
    }

    if (!VulkanQuery->CurrentQuery.IsValid())
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate Query");
        return;
    }

    GetCommandBuffer().BeginQuery(VulkanQuery->CurrentQuery);
    PendingQueries.Add(VulkanQuery);
    ActiveQueryCount++;
}

void FVulkanCommandContext::EndQuery(FRHIQuery* Query)
{
    FVulkanQueryRHI* VulkanQuery = FVulkanRHI::ResourceCast(Query);
    CHECK(VulkanQuery != nullptr);

    if (!VulkanQuery->CurrentQuery.IsValid())
    {
        VULKAN_ERROR_CRITICAL("No valid query allocation, ensure that RHIBeginQuery was called correctly");
        return;
    }

    ActiveQueryCount--;
    CHECK(ActiveQueryCount >= 0);

    GetCommandBuffer().EndQuery(VulkanQuery->CurrentQuery);
}

void FVulkanCommandContext::QueryTimestamp(FRHIQuery* Query)
{
    FVulkanQueryRHI* VulkanQuery = FVulkanRHI::ResourceCast(Query);
    CHECK(VulkanQuery != nullptr);

    if (!TimestampQueryAllocator.Allocate(VulkanQuery->CurrentQuery, VulkanQuery->QueryResult, EVulkanQueryType::Timestamp))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate timestamp query");
        return;
    }

    const VkPipelineStageFlagBits PipelineStage = CVarTimestampTopOfPipe.GetValue()
        ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    GetCommandBuffer().EndQuery(VulkanQuery->CurrentQuery, PipelineStage);
    PendingQueries.Add(VulkanQuery);
}

void FVulkanCommandContext::ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    FVulkanTexture* VulkanTexture = FVulkanRHI::ResourceCast(this, RenderTargetView.Texture);
    CHECK(VulkanTexture != nullptr);

    FVulkanHashableImageView HashableImageView;
    HashableImageView.ArrayIndex     = RenderTargetView.ArrayIndex;
    HashableImageView.NumArraySlices = RenderTargetView.NumArraySlices;
    HashableImageView.Format         = RenderTargetView.Format;
    HashableImageView.MipLevel       = RenderTargetView.MipLevel;

    if (FVulkanResourceView* ImageView = VulkanTexture->GetOrCreateImageView(HashableImageView))
    {
        // NOTE: Here the image is expected to be in a "RenderTargetState" so we need to transition
        // it to TransferDst, we then need to transition back when the clear is done.
        VkImageMemoryBarrier2 ImageBarrier = {};
        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.oldLayout                       = FVulkanRHI::ResourceStateToImageLayout(EResourceAccess::RenderTarget);
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = VulkanTexture->GetVkImage();
        ImageBarrier.srcAccessMask                   = FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess::RenderTarget);
        ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VulkanTexture->GetVkFormat());
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        VkClearColorValue VulkanClearColor;
        FMemory::Memcpy(VulkanClearColor.float32, ClearColor.XYZW, sizeof(VulkanClearColor.float32));

        const FVulkanResourceView::FImageView& ImageViewInfo = ImageView->GetImageViewInfo();
        GetCommandBuffer()->ClearColorImage(ImageViewInfo.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &VulkanClearColor, 1, &ImageViewInfo.SubresourceRange);
        
        // .. And transition back into "RenderTargetState"
        ImageBarrier.newLayout           = FVulkanRHI::ResourceStateToImageLayout(EResourceAccess::RenderTarget);
        ImageBarrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstAccessMask       = FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess::RenderTarget);
        ImageBarrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
    }
}

void FVulkanCommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
    FVulkanTexture* VulkanTexture = FVulkanRHI::ResourceCast(this, DepthStencilView.Texture);
    CHECK(VulkanTexture != nullptr);

    FVulkanHashableImageView HashableImageView;
    HashableImageView.ArrayIndex     = DepthStencilView.ArrayIndex;
    HashableImageView.NumArraySlices = DepthStencilView.NumArraySlices;
    HashableImageView.Format         = DepthStencilView.Format;
    HashableImageView.MipLevel       = DepthStencilView.MipLevel;

    if (FVulkanResourceView* ImageView = VulkanTexture->GetOrCreateImageView(HashableImageView))
    {
        // NOTE: Here the image is expected to be in a "DepthStencilState" so we need to transition 
        // it to TransferDst, we then need to transition back when the clear is done.
        VkImageMemoryBarrier2 ImageBarrier = {};
        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.oldLayout                       = FVulkanRHI::ResourceStateToImageLayout(EResourceAccess::DepthWrite);
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = VulkanTexture->GetVkImage();
        ImageBarrier.srcAccessMask                   = FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess::DepthWrite);
        ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VulkanTexture->GetVkFormat());
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        VkClearDepthStencilValue DepthStencilValue;
        DepthStencilValue.depth   = Depth;
        DepthStencilValue.stencil = Stencil;

        const FVulkanResourceView::FImageView& ImageViewInfo = ImageView->GetImageViewInfo();
        GetCommandBuffer()->ClearDepthStencilImage(ImageViewInfo.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &DepthStencilValue, 1, &ImageViewInfo.SubresourceRange);

        // .. And transition back into "DepthStencilState"
        ImageBarrier.newLayout           = FVulkanRHI::ResourceStateToImageLayout(EResourceAccess::DepthWrite);
        ImageBarrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstAccessMask       = FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess::DepthWrite);
        ImageBarrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
    }
}

void FVulkanCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
    FVulkanUnorderedAccessView* VulkanUnorderedAccessView = FVulkanRHI::ResourceCast(UnorderedAccessView);
    CHECK(VulkanUnorderedAccessView != nullptr);
    
    VkClearColorValue VulkanClearColor;
    FMemory::Memcpy(VulkanClearColor.float32, ClearColor.XYZW, sizeof(VulkanClearColor.float32));

    const FVulkanResourceView::EType Type = VulkanUnorderedAccessView->GetType();
    if (Type == FVulkanResourceView::EType::ImageView)
    {
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        const FVulkanResourceView::FImageView& ImageViewInfo = VulkanUnorderedAccessView->GetImageViewInfo();
        GetCommandBuffer()->ClearColorImage(ImageViewInfo.Image, VK_IMAGE_LAYOUT_GENERAL, &VulkanClearColor, 1, &ImageViewInfo.SubresourceRange);
    }
    else if (Type == FVulkanResourceView::EType::StructuredBufferView)
    {
        uint32 FillData;
        FMemory::Memcpy(&FillData, &ClearColor.X, sizeof(uint32));
        
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        const FVulkanResourceView::FStructuredBufferView& BufferInfo = VulkanUnorderedAccessView->GetStructuredBufferInfo();
        GetCommandBuffer()->FillBuffer(BufferInfo.Buffer, BufferInfo.Offset, BufferInfo.Range, FillData);
    }
    else if (Type == FVulkanResourceView::EType::TypedBufferView)
    {
        uint32 FillData;
        FMemory::Memcpy(&FillData, &ClearColor.X, sizeof(uint32));
        
        BarrierBatcher.FlushBarriers(GetCommandBuffer());
        
        const FVulkanResourceView::FTypedBufferView& BufferInfo = VulkanUnorderedAccessView->GetTypedBufferInfo();
        GetCommandBuffer()->FillBuffer(BufferInfo.Buffer, 0, VK_WHOLE_SIZE, FillData);
    }
    else
    {
        VULKAN_ERROR("Unsupported UAV type for ClearUnorderedAccessViewFloat");
    }
}

void FVulkanCommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
    FVulkanUnorderedAccessView* VulkanUnorderedAccessView = FVulkanRHI::ResourceCast(UnorderedAccessView);
    CHECK(VulkanUnorderedAccessView != nullptr);

    const FVulkanResourceView::EType Type = VulkanUnorderedAccessView->GetType();
    if (Type == FVulkanResourceView::EType::ImageView)
    {
        VkClearColorValue VulkanClearColor;
        FMemory::Memcpy(VulkanClearColor.uint32, Values, sizeof(VulkanClearColor.uint32));

        const FVulkanResourceView::FImageView& ImageViewInfo = VulkanUnorderedAccessView->GetImageViewInfo();
        GetCommandBuffer()->ClearColorImage(ImageViewInfo.Image, VK_IMAGE_LAYOUT_GENERAL, &VulkanClearColor, 1, &ImageViewInfo.SubresourceRange);
    }
    else if (Type == FVulkanResourceView::EType::StructuredBufferView)
    {
        const FVulkanResourceView::FStructuredBufferView& BufferInfo = VulkanUnorderedAccessView->GetStructuredBufferInfo();
        BarrierBatcher.FlushBarriers(GetCommandBuffer());
        GetCommandBuffer()->FillBuffer(BufferInfo.Buffer, BufferInfo.Offset, BufferInfo.Range, Values[0]);
    }
    else if (Type == FVulkanResourceView::EType::TypedBufferView)
    {
        const FVulkanResourceView::FTypedBufferView& BufferInfo = VulkanUnorderedAccessView->GetTypedBufferInfo();
        BarrierBatcher.FlushBarriers(GetCommandBuffer());
        GetCommandBuffer()->FillBuffer(BufferInfo.Buffer, 0, VK_WHOLE_SIZE, Values[0]);
    }
    else
    {
        VULKAN_ERROR("Unsupported UAV type for ClearUnorderedAccessViewUint");
    }
}

void FVulkanCommandContext::BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo)
{
    CHECK(ContextState.IsRecording() && !ContextState.IsInsideRenderPass() && !ContextState.IsRenderPassPaused());
    ContextState.BeginRenderPass(BeginRenderPassInfo);
}

void FVulkanCommandContext::EndRenderPass()  
{
    CHECK(ContextState.IsInsideRenderPass());
    ContextState.EndRenderPass();
}

void FVulkanCommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    VkViewport Viewport = {};
    if (GVulkanEnableNegativeViewportHeight)
    {
        Viewport.width    =  ViewportRegion.Width;
        Viewport.height   = -ViewportRegion.Height;
        Viewport.maxDepth =  ViewportRegion.MaxDepth;
        Viewport.minDepth =  ViewportRegion.MinDepth;
        Viewport.x        =  ViewportRegion.PositionX;
        Viewport.y        =  ViewportRegion.Height - ViewportRegion.PositionY;
    }
    else
    {
        Viewport.width    = ViewportRegion.Width;
        Viewport.height   = ViewportRegion.Height;
        Viewport.maxDepth = ViewportRegion.MaxDepth;
        Viewport.minDepth = ViewportRegion.MinDepth;
        Viewport.x        = ViewportRegion.PositionX;
        Viewport.y        = ViewportRegion.PositionY;
    }
    
    ContextState.SetViewports(&Viewport, 1);
}

void FVulkanCommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
    VkRect2D ScissorRect = {};
    ScissorRect.offset.x      = static_cast<int32_t>(ScissorRegion.PositionX);
    ScissorRect.offset.y      = static_cast<int32_t>(ScissorRegion.PositionY);
    ScissorRect.extent.width  = static_cast<int32_t>(ScissorRegion.Width);
    ScissorRect.extent.height = static_cast<int32_t>(ScissorRegion.Height);
    
    ContextState.SetScissorRects(&ScissorRect, 1);
}

void FVulkanCommandContext::SetBlendFactor(const FVector4& Color)
{
    ContextState.SetBlendFactor(Color.XYZW);
}

void FVulkanCommandContext::SetStencilRef(uint32 StencilRef)
{
    ContextState.SetStencilRef(StencilRef);
}

void FVulkanCommandContext::SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias)
{
    ContextState.SetDepthBias(DepthBias, DepthBiasClamp, SlopeScaledDepthBias);
}

void FVulkanCommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    ContextState.SetStreamOutputTargets(Buffers, Offsets);
}

void FVulkanCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 Index = 0; Index < InVertexBuffers.Size(); ++Index)
    {
        FVulkanBuffer* VulkanVertexBuffer = FVulkanRHI::ResourceCast(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(VulkanVertexBuffer, BufferSlot + Index);
    }
}

void FVulkanCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FVulkanBuffer* VulkanIndexBuffer = FVulkanRHI::ResourceCast(IndexBuffer);
    ContextState.SetIndexBuffer(VulkanIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FVulkanCommandContext::SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FVulkanGraphicsPipelineState* VulkanPipelineState = FVulkanRHI::ResourceCast(PipelineState);
    ContextState.SetGraphicsPipelineState(VulkanPipelineState);
}

void FVulkanCommandContext::SetComputePipelineState(class FRHIComputePipelineState* PipelineState)  
{
    FVulkanComputePipelineState* VulkanPipelineState = FVulkanRHI::ResourceCast(PipelineState);
    ContextState.SetComputePipelineState(VulkanPipelineState);
}

void FVulkanCommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    ContextState.SetPushConstants(reinterpret_cast<const uint32*>(ShaderConstants), NumShaderConstants);
}

void FVulkanCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    FVulkanShaderResourceView* VulkanShaderResourceView = FVulkanRHI::ResourceCast(ShaderResourceView);
    ContextState.SetSRV(VulkanShaderResourceView, VulkanShader->GetShaderVisibility(), RegisterIndex);
}

void FVulkanCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex + InShaderResourceViews.Size() <= VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        FVulkanShaderResourceView* VulkanShaderResourceView = FVulkanRHI::ResourceCast(InShaderResourceViews[Index]);
        ContextState.SetSRV(VulkanShaderResourceView, VulkanShader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FVulkanCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    FVulkanUnorderedAccessView* VulkanUnorderedAccessView = FVulkanRHI::ResourceCast(UnorderedAccessView);
    ContextState.SetUAV(VulkanUnorderedAccessView, VulkanShader->GetShaderVisibility(), RegisterIndex);
}

void FVulkanCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex + InUnorderedAccessViews.Size() <= VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        FVulkanUnorderedAccessView* VulkanUnorderedAccessView = FVulkanRHI::ResourceCast(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(VulkanUnorderedAccessView, VulkanShader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FVulkanCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT);

    FVulkanBuffer* VulkanConstantBuffer = FVulkanRHI::ResourceCast(ConstantBuffer);
    ContextState.SetUniformBuffer(VulkanConstantBuffer, VulkanShader->GetShaderVisibility(), RegisterIndex);
}

void FVulkanCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex + InConstantBuffers.Size() <= VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT);

    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        FVulkanBuffer* VulkanConstantBuffer = FVulkanRHI::ResourceCast(InConstantBuffers[Index]);
        ContextState.SetUniformBuffer(VulkanConstantBuffer, VulkanShader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FVulkanCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex < VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    FVulkanSamplerState* VulkanSamplerState = FVulkanRHI::ResourceCast(SamplerState);
    ContextState.SetSampler(VulkanSamplerState, VulkanShader->GetShaderVisibility(), RegisterIndex);
}

void FVulkanCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex + InSamplerStates.Size() <= VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        FVulkanSamplerState* VulkanSamplerState = FVulkanRHI::ResourceCast(InSamplerStates[Index]);
        ContextState.SetSampler(VulkanSamplerState, VulkanShader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FVulkanCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)     
{
    FVulkanBuffer* VulkanBuffer = FVulkanRHI::ResourceCast(Dst);
    CHECK(VulkanBuffer != nullptr);

    if (VulkanBuffer->GetInfo().IsTransient())
    {
        FVulkanMemoryStorage NewStorage(GetDevice());
        void* MappedMemory = GetDevice()->GetMemoryManager().AllocateConstants(BufferRegion.Size, 0, NewStorage);
        CHECK(MappedMemory != nullptr);

        FMemory::Memcpy(MappedMemory, SrcData, BufferRegion.Size);
        VulkanBuffer->GetMemoryStorage().Swap(NewStorage);
        VulkanBuffer->ResourceRelocated(&VulkanBuffer->GetMemoryStorage());
    }
    else if (VulkanBuffer->GetInfo().IsDynamic())
    {
        void* BufferData = VulkanBuffer->Map(BufferRegion.Offset, BufferRegion.Size);
        if (!BufferData)
        {
            VULKAN_ERROR_CRITICAL("Failed to map buffer memory");
            return;
        }

        FMemory::Memcpy(BufferData, SrcData, BufferRegion.Size);
        VulkanBuffer->Unmap(BufferRegion.Offset, BufferRegion.Size);
    }
    else
    {
        FVulkanMemoryStorage UploadStorage(GetDevice());
        void* MappedMemory = GetDevice()->GetMemoryManager().AllocateUploadMemory(BufferRegion.Size, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, UploadStorage);
        CHECK(MappedMemory != nullptr);

        FMemory::Memcpy(MappedMemory, SrcData, BufferRegion.Size);
        
        VkBufferCopy BufferCopy = {};
        BufferCopy.srcOffset = UploadStorage.GetBufferOffset();
        BufferCopy.dstOffset = VulkanBuffer->GetBindOffset() + BufferRegion.Offset;
        BufferCopy.size      = BufferRegion.Size;
        
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        GetCommandBuffer()->CopyBuffer(UploadStorage.GetBackingBuffer(), VulkanBuffer->GetBindVkBuffer(), 1, &BufferCopy);
    }
}

void FVulkanCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) 
{
    FVulkanTexture* VulkanTexture = FVulkanRHI::ResourceCast(this, Dst);
    CHECK(VulkanTexture != nullptr);

    const VkFormat Format       = VulkanTexture->GetVkFormat();
    const uint64   RequiredSize = VkCalculateTextureUploadSize(Format, TextureRegion.Width, TextureRegion.Height);
    const uint64   Alignment    = GetDevice()->GetPhysicalDevice()->GetProperties().limits.optimalBufferCopyOffsetAlignment;

    FVulkanMemoryStorage UploadStorage(GetDevice());
    uint8* UploadMemory = static_cast<uint8*>(GetDevice()->GetMemoryManager().AllocateUploadMemory(RequiredSize, Alignment, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, UploadStorage));
    CHECK(UploadMemory != nullptr);

    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);
    CHECK(Source != nullptr);
    
    const uint32 RowPitch = VkCalculateTextureRowPitch(Format, TextureRegion.Width);
    const uint32 NumRows  = VkCalculateTextureNumRows(Format, TextureRegion.Height);

    for (uint64 y = 0; y < NumRows; y++)
    {
        FMemory::Memcpy(UploadMemory, Source, RowPitch);
        Source       += SrcRowPitch;
        UploadMemory += RowPitch;
    }

    VkBufferImageCopy BufferImageCopy = {};
    BufferImageCopy.bufferOffset                    = UploadStorage.GetBufferOffset();
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = GetImageAspectFlagsFromFormat(Format);
    BufferImageCopy.imageSubresource.mipLevel       = MipLevel;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { static_cast<int32>(TextureRegion.PositionX), static_cast<int32>(TextureRegion.PositionY), 0 };
    BufferImageCopy.imageExtent                     = { TextureRegion.Width, TextureRegion.Height, 1 };

    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyBufferToImage(UploadStorage.GetBackingBuffer(), VulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);
}

void FVulkanCommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
    FVulkanTexture* VulkanTexture = FVulkanRHI::ResourceCast(this, Dst);
    CHECK(VulkanTexture != nullptr);

    const VkFormat Format       = VulkanTexture->GetVkFormat();
    const uint32   RowPitch     = VkCalculateTextureRowPitch(Format, TextureRegion.Width);
    const uint32   NumRows      = VkCalculateTextureNumRows(Format, TextureRegion.Height);
    const uint64   SliceSize    = static_cast<uint64>(RowPitch) * NumRows;
    const uint64   RequiredSize = SliceSize * TextureRegion.Depth;
    const uint64   Alignment    = GetDevice()->GetPhysicalDevice()->GetProperties().limits.optimalBufferCopyOffsetAlignment;

    FVulkanMemoryStorage UploadStorage(GetDevice());
    uint8* UploadMemory = static_cast<uint8*>(GetDevice()->GetMemoryManager().AllocateUploadMemory(RequiredSize, Alignment, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, UploadStorage));
    CHECK(UploadMemory != nullptr);

    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);
    CHECK(Source != nullptr);

    for (uint32 z = 0; z < TextureRegion.Depth; z++)
    {
        const uint8* SliceSource = Source + z * SrcDepthPitch;
        for (uint32 y = 0; y < NumRows; y++)
        {
            FMemory::Memcpy(UploadMemory, SliceSource, RowPitch);
            SliceSource  += SrcRowPitch;
            UploadMemory += RowPitch;
        }
    }

    VkBufferImageCopy BufferImageCopy = {};
    BufferImageCopy.bufferOffset                    = UploadStorage.GetBufferOffset();
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = GetImageAspectFlagsFromFormat(Format);
    BufferImageCopy.imageSubresource.mipLevel       = MipLevel;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { static_cast<int32>(TextureRegion.PositionX), static_cast<int32>(TextureRegion.PositionY), static_cast<int32>(TextureRegion.PositionZ) };
    BufferImageCopy.imageExtent                     = { TextureRegion.Width, TextureRegion.Height, TextureRegion.Depth };

    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyBufferToImage(UploadStorage.GetBackingBuffer(), VulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);
}

void FVulkanCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FVulkanTexture* SrcVulkanTexture = FVulkanRHI::ResourceCast(this, Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTexture* DstVulkanTexture = FVulkanRHI::ResourceCast(this, Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    CHECK(SrcVulkanTexture->GetWidth()  == DstVulkanTexture->GetWidth());
    CHECK(SrcVulkanTexture->GetHeight() == DstVulkanTexture->GetHeight());
    CHECK(SrcVulkanTexture->GetDepth()  == DstVulkanTexture->GetDepth());
    
    VkImageResolve ImageResolve = {};
    ImageResolve.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
    ImageResolve.srcSubresource.mipLevel       = 0;
    ImageResolve.srcSubresource.baseArrayLayer = 0;
    ImageResolve.srcSubresource.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageResolve.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
    ImageResolve.dstSubresource.mipLevel       = 0;
    ImageResolve.dstSubresource.baseArrayLayer = 0;
    ImageResolve.dstSubresource.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageResolve.extent.width                  = DstVulkanTexture->GetWidth();
    ImageResolve.extent.height                 = DstVulkanTexture->GetHeight();
    ImageResolve.extent.depth                  = DstVulkanTexture->GetDepth();
    
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->ResolveImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageResolve);
}

void FVulkanCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc)
{
    FVulkanBuffer* SrcVulkanBuffer = FVulkanRHI::ResourceCast(Src);
    CHECK(SrcVulkanBuffer != nullptr);
    
    FVulkanBuffer* DstVulkanBuffer = FVulkanRHI::ResourceCast(Dst);
    CHECK(DstVulkanBuffer != nullptr);

    VkBufferCopy BufferCopy = {};
    BufferCopy.srcOffset = SrcVulkanBuffer->GetBindOffset() + CopyDesc.SrcOffset;
    BufferCopy.dstOffset = DstVulkanBuffer->GetBindOffset() + CopyDesc.DstOffset;
    BufferCopy.size      = CopyDesc.Size;
    
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyBuffer(SrcVulkanBuffer->GetBindVkBuffer(), DstVulkanBuffer->GetBindVkBuffer(), 1, &BufferCopy);
}

void FVulkanCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FVulkanTexture* SrcVulkanTexture = FVulkanRHI::ResourceCast(this, Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTexture* DstVulkanTexture = FVulkanRHI::ResourceCast(this, Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    CHECK(SrcVulkanTexture->GetWidth()        == DstVulkanTexture->GetWidth());
    CHECK(SrcVulkanTexture->GetHeight()       == DstVulkanTexture->GetHeight());
    CHECK(SrcVulkanTexture->GetDepth()        == DstVulkanTexture->GetDepth());
    CHECK(SrcVulkanTexture->GetNumMipLevels() == DstVulkanTexture->GetNumMipLevels());
    CHECK(SrcVulkanTexture->GetDimension()    == DstVulkanTexture->GetDimension());
    
    constexpr uint32 MaxCopies = 15;
    VkImageCopy ImageCopies[MaxCopies];
    
    const FRHITextureInfo TextureInfo = DstVulkanTexture->GetInfo();
    for (uint32 MipLevel = 0; MipLevel < TextureInfo.NumMipLevels; MipLevel++)
    {
        VkImageCopy& ImageCopy = ImageCopies[MipLevel];
        FMemory::Memzero(&ImageCopy, sizeof(ImageCopy));
    
        ImageCopy.extent.width                  = Math::Max<uint32>(TextureInfo.Extent.X >> MipLevel, 1u);
        ImageCopy.extent.height                 = Math::Max<uint32>(TextureInfo.Extent.Y >> MipLevel, 1u);
        ImageCopy.extent.depth                  = Math::Max<uint32>(TextureInfo.Extent.Z >> MipLevel, 1u);
        ImageCopy.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
        ImageCopy.srcSubresource.mipLevel       = MipLevel;
        ImageCopy.srcSubresource.baseArrayLayer = 0;
        ImageCopy.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
        ImageCopy.dstSubresource.mipLevel       = MipLevel;
        ImageCopy.dstSubresource.baseArrayLayer = 0;

        // NOTE: We want to copy the full function
        if (IsTextureCube(DstVulkanTexture->GetDimension()))
        {
            ImageCopy.srcSubresource.layerCount = SrcVulkanTexture->GetNumArraySlices() * RHI_NUM_CUBE_FACES;
            ImageCopy.dstSubresource.layerCount = DstVulkanTexture->GetNumArraySlices() * RHI_NUM_CUBE_FACES;
        }
        else
        {
            ImageCopy.srcSubresource.layerCount = SrcVulkanTexture->GetNumArraySlices();
            ImageCopy.dstSubresource.layerCount = DstVulkanTexture->GetNumArraySlices();
        }
    }

    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, TextureInfo.NumMipLevels, ImageCopies);
}

void FVulkanCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc)
{
    FVulkanTexture* SrcVulkanTexture = FVulkanRHI::ResourceCast(this, Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTexture* DstVulkanTexture = FVulkanRHI::ResourceCast(this, Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    constexpr uint32 MaxCopies = 15;
    VkImageCopy ImageCopy[MaxCopies];
    
    uint32 NumArrayLayers    = 0;
    uint32 DstBaseArrayLayer = 0;
    uint32 SrcBaseArrayLayer = 0;

    if (IsTextureCube(SrcVulkanTexture->GetDimension()))
    {
        SrcBaseArrayLayer = CopyDesc.SrcArraySlice  * RHI_NUM_CUBE_FACES;
        NumArrayLayers    = CopyDesc.NumArraySlices * RHI_NUM_CUBE_FACES;
    }
    else
    {
        SrcBaseArrayLayer = CopyDesc.SrcArraySlice;
        NumArrayLayers    = CopyDesc.NumArraySlices;
    }
    
    if (IsTextureCube(DstVulkanTexture->GetDimension()))
    {
        DstBaseArrayLayer = CopyDesc.DstArraySlice * RHI_NUM_CUBE_FACES;
        NumArrayLayers    = Math::Max(CopyDesc.NumArraySlices * RHI_NUM_CUBE_FACES, NumArrayLayers);
    }
    else
    {
        DstBaseArrayLayer = CopyDesc.DstArraySlice;
        NumArrayLayers    = Math::Max(CopyDesc.NumArraySlices, NumArrayLayers);
    }

    // Flush barriers
    BarrierBatcher.FlushBarriers(GetCommandBuffer());
    
    // We copy each layer separately due to MoltenVK seems to be acting weird when doing all layers separately
    for (uint32 ArrayLayer = 0; ArrayLayer < NumArrayLayers; ArrayLayer++)
    {
        for (uint32 MipLevel = 0; MipLevel < CopyDesc.NumMipLevels; MipLevel++)
        {
            VkImageCopy& CopyInfo = ImageCopy[MipLevel];
            FMemory::Memzero(&CopyInfo, sizeof(CopyInfo));
            
            // Describe the source subresource
            CopyInfo.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
            CopyInfo.srcSubresource.mipLevel       = CopyDesc.SrcMipSlice + MipLevel;
            CopyInfo.srcOffset.x                   = CopyDesc.SrcPosition.X >> MipLevel;
            CopyInfo.srcOffset.y                   = CopyDesc.SrcPosition.Y >> MipLevel;
            CopyInfo.srcOffset.z                   = CopyDesc.SrcPosition.Z >> MipLevel;
            CopyInfo.srcSubresource.baseArrayLayer = SrcBaseArrayLayer + ArrayLayer;
            CopyInfo.srcSubresource.layerCount     = 1;
            
            // Describe the destination subresource
            CopyInfo.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
            CopyInfo.dstSubresource.mipLevel       = CopyDesc.DstMipSlice + MipLevel;
            CopyInfo.dstOffset.x                   = CopyDesc.DstPosition.X >> MipLevel;
            CopyInfo.dstOffset.y                   = CopyDesc.DstPosition.Y >> MipLevel;
            CopyInfo.dstOffset.z                   = CopyDesc.DstPosition.Z >> MipLevel;
            CopyInfo.dstSubresource.baseArrayLayer = DstBaseArrayLayer + ArrayLayer;
            CopyInfo.dstSubresource.layerCount     = 1;
            
            // Size of this mip-slice
            CopyInfo.extent.width  = Math::Max(CopyDesc.Size.X >> MipLevel, 1);
            CopyInfo.extent.height = Math::Max(CopyDesc.Size.Y >> MipLevel, 1);
            CopyInfo.extent.depth  = Math::Max(CopyDesc.Size.Z >> MipLevel, 1);
        }
        
        GetCommandBuffer()->CopyImage(SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, CopyDesc.NumMipLevels, ImageCopy);
    }
}

void FVulkanCommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FVulkanTexture* SrcVulkanTexture = FVulkanRHI::ResourceCast(this, Src);
    CHECK(SrcVulkanTexture != nullptr);

    FVulkanBuffer* DstVulkanBuffer = FVulkanRHI::ResourceCast(Dst);
    CHECK(DstVulkanBuffer != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    const uint32 MipX = SrcRegion.PositionX >> SrcMipLevel;
    const uint32 MipY = SrcRegion.PositionY >> SrcMipLevel;

    VkBufferImageCopy Copy = {};
    Copy.bufferOffset                    = DstVulkanBuffer->GetBindOffset() + DstOffset;
    Copy.bufferRowLength                 = 0;
    Copy.bufferImageHeight               = 0;
    Copy.imageSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
    Copy.imageSubresource.mipLevel       = SrcMipLevel;
    Copy.imageSubresource.baseArrayLayer = 0;
    Copy.imageSubresource.layerCount     = 1;
    Copy.imageOffset.x                   = static_cast<int32>(MipX);
    Copy.imageOffset.y                   = static_cast<int32>(MipY);
    Copy.imageOffset.z                   = 0;
    Copy.imageExtent.width               = Math::Max(SrcRegion.Width >> SrcMipLevel, 1u);
    Copy.imageExtent.height              = Math::Max(SrcRegion.Height >> SrcMipLevel, 1u);
    Copy.imageExtent.depth               = 1;

    vkCmdCopyImageToBuffer(GetCommandBuffer().GetVkCommandBuffer(), SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanBuffer->GetBindVkBuffer(), 1, &Copy);
}

void FVulkanCommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FVulkanTexture* SrcVulkanTexture = FVulkanRHI::ResourceCast(this, Src);
    CHECK(SrcVulkanTexture != nullptr);

    FVulkanBuffer* DstVulkanBuffer = FVulkanRHI::ResourceCast(Dst);
    CHECK(DstVulkanBuffer != nullptr);

    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    VkBufferImageCopy Copy = {};
    Copy.bufferOffset                    = DstVulkanBuffer->GetBindOffset() + DstOffset;
    Copy.bufferRowLength                 = 0;
    Copy.bufferImageHeight               = 0;
    Copy.imageSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
    Copy.imageSubresource.mipLevel       = SrcMipLevel;
    Copy.imageSubresource.baseArrayLayer = SrcArraySlice;
    Copy.imageSubresource.layerCount     = 1;
    Copy.imageOffset.x                   = static_cast<int32>(SrcRegion.PositionX);
    Copy.imageOffset.y                   = static_cast<int32>(SrcRegion.PositionY);
    Copy.imageOffset.z                   = static_cast<int32>(SrcRegion.PositionZ);
    Copy.imageExtent.width               = SrcRegion.Width;
    Copy.imageExtent.height              = Math::Max(SrcRegion.Height, 1u);
    Copy.imageExtent.depth               = Math::Max(SrcRegion.Depth, 1u);

    vkCmdCopyImageToBuffer(GetCommandBuffer().GetVkCommandBuffer(), SrcVulkanTexture->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DstVulkanBuffer->GetBindVkBuffer(), 1, &Copy);
}

void FVulkanCommandContext::WriteFence(FRHIGpuFence* Fence)
{
    CHECK(Fence != nullptr);
    FVulkanGpuFence* VulkanFence = FVulkanRHI::ResourceCast(Fence);

    if (!CommandBuffer)
    {
        ObtainCommandBuffer();
    }

    if (CommandBuffer && CommandBuffer->GetNumCommands() > 0)
    {
        if (VulkanFence->UsesTimeline())
        {
            VulkanFence->EnqueueSignal(GetCommandQueue());
        }

        FVulkanFence* SubmittedFence = nullptr;
        FinishCommandBuffer(true, true, &SubmittedFence);

        if (!VulkanFence->UsesTimeline())
        {
            VulkanFence->SetSubmissionFence(SubmittedFence);
        }

        ObtainCommandBuffer();
    }
}

void FVulkanCommandContext::DiscardContents(FRHITexture* Resource)
{
    FVulkanTexture* VulkanTexture = FVulkanRHI::ResourceCast(this, Resource);
    if (!VulkanTexture)
    {
        return;
    }

    FVulkanImageLayoutState& LocalState = RetrievePendingImageState(VulkanTexture);

    VkImageLayout CurrentLayout = LocalState.GetImageLayout();
    if (CurrentLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
    {
        CurrentLayout = VulkanTexture->GetImageLayoutState().GetImageLayout();
    }

    if (CurrentLayout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        return;
    }

    const VkImageCreateInfo& CreateInfo = VulkanTexture->GetVkImageCreateInfo();

    VkImageMemoryBarrier2 ImageBarrier = {};
    ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageBarrier.newLayout                       = CurrentLayout;
    ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.image                           = VulkanTexture->GetVkImage();
    ImageBarrier.srcAccessMask                   = 0;
    ImageBarrier.dstAccessMask                   = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(CreateInfo.format);
    ImageBarrier.subresourceRange.baseArrayLayer = 0;
    ImageBarrier.subresourceRange.baseMipLevel   = 0;
    ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
}

void FVulkanCommandContext::BuildRayTracingScene(FRHIRayTracingScene* InRayTracingScene, const FRayTracingSceneBuildInfo& InBuildInfo)
{
    UNREFERENCED_VARIABLE(InRayTracingScene);
    UNREFERENCED_VARIABLE(InBuildInfo);
}

void FVulkanCommandContext::BuildRayTracingGeometry(FRHIRayTracingGeometry* InRayTracingGeometry, const FRayTracingGeometryBuildInfo& InBuildInfo)
{
    UNREFERENCED_VARIABLE(InRayTracingGeometry);
    UNREFERENCED_VARIABLE(InBuildInfo);
}

void FVulkanCommandContext::SetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources)
{
    UNREFERENCED_VARIABLE(RayTracingScene);
    UNREFERENCED_VARIABLE(PipelineState);
    UNREFERENCED_VARIABLE(GlobalResource);
    UNREFERENCED_VARIABLE(RayGenLocalResources);
    UNREFERENCED_VARIABLE(MissLocalResources);
    UNREFERENCED_VARIABLE(HitGroupResources);
    UNREFERENCED_VARIABLE(NumHitGroupResources);
}

void FVulkanCommandContext::TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
    FVulkanTexture* VulkanTexture = FVulkanRHI::ResourceCast(this, Texture);
    CHECK(VulkanTexture != nullptr);

    FVulkanImageLayoutState& LocalState = RetrievePendingImageState(VulkanTexture);

    const VkImageLayout NewLayout      = FVulkanRHI::ResourceStateToImageLayout(TextureTransition.AfterState);
    const VkImageLayout PreviousLayout = FVulkanRHI::ResourceStateToImageLayout(TextureTransition.BeforeState);

    if (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS && TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES)
    {
        const VkImageLayout CurrentLayout = LocalState.GetImageLayout();
        if (CurrentLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
        {
            FVulkanPendingImageBarrier PendingBarrier;
            PendingBarrier.Texture       = VulkanTexture;
            PendingBarrier.DesiredLayout = PreviousLayout;
            PendingBarrier.Subresource   = RHI_ALL_MIP_LEVELS;
            PendingImageBarriers.Add(PendingBarrier);
        }
        else if (LocalState.AreAllSubresourcesSameLayout())
        {
            CHECK(CurrentLayout == PreviousLayout);
        }
    }
    else
    {
        const VkImageCreateInfo& CreateInfo = VulkanTexture->GetVkImageCreateInfo();

        const uint32 BaseMip    = (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS)     ? 0 : TextureTransition.MipLevel;
        const uint32 MipCount   = (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS)     ? CreateInfo.mipLevels : 1;
        const uint32 BaseLayer  = (TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES) ? 0 : TextureTransition.ArraySlice;
        const uint32 LayerCount = (TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES) ? CreateInfo.arrayLayers : 1;

        for (uint32 Layer = BaseLayer; Layer < BaseLayer + LayerCount; Layer++)
        {
            for (uint32 Mip = BaseMip; Mip < BaseMip + MipCount; Mip++)
            {
                const uint32 SubresourceIndex = Layer * CreateInfo.mipLevels + Mip;

                const VkImageLayout SubLayout = LocalState.GetSubresourceLayout(SubresourceIndex);
                if (SubLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
                {
                    FVulkanPendingImageBarrier PendingBarrier;
                    PendingBarrier.Texture       = VulkanTexture;
                    PendingBarrier.DesiredLayout = PreviousLayout;
                    PendingBarrier.Subresource   = SubresourceIndex;
                    PendingImageBarriers.Add(PendingBarrier);
                }
                else
                {
                    CHECK(SubLayout == PreviousLayout);
                }
            }
        }
    }

    if (NewLayout != PreviousLayout)
    {
        VkImageMemoryBarrier2 ImageBarrier = {};
        ImageBarrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        ImageBarrier.newLayout                   = NewLayout;
        ImageBarrier.oldLayout                   = PreviousLayout;
        ImageBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                       = VulkanTexture->GetVkImage();
        ImageBarrier.srcAccessMask               = FVulkanRHI::ResourceStateToAccessFlags(TextureTransition.BeforeState);
        ImageBarrier.dstAccessMask               = FVulkanRHI::ResourceStateToAccessFlags(TextureTransition.AfterState);
        ImageBarrier.srcStageMask                = FVulkanRHI::ResourceStateToPipelineStageFlags(TextureTransition.BeforeState);
        ImageBarrier.dstStageMask                = FVulkanRHI::ResourceStateToPipelineStageFlags(TextureTransition.AfterState);
        ImageBarrier.subresourceRange.aspectMask = GetImageAspectFlagsFromFormat(VulkanTexture->GetVkFormat());

        if (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS)
        {
            ImageBarrier.subresourceRange.baseMipLevel = 0;
            ImageBarrier.subresourceRange.levelCount   = VK_REMAINING_MIP_LEVELS;
        }
        else
        {
            ImageBarrier.subresourceRange.baseMipLevel = TextureTransition.MipLevel;
            ImageBarrier.subresourceRange.levelCount   = 1;
        }

        if (TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES)
        {
            ImageBarrier.subresourceRange.baseArrayLayer = 0;
            ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        }
        else
        {
            uint32 LayerCount;
            uint32 BaseArrayLayer;

            if (IsTextureCube(VulkanTexture->GetDimension()))
            {
                LayerCount     = RHI_NUM_CUBE_FACES;
                BaseArrayLayer = TextureTransition.ArraySlice * RHI_NUM_CUBE_FACES;
            }
            else
            {
                LayerCount     = 1u;
                BaseArrayLayer = TextureTransition.ArraySlice;
            }

            ImageBarrier.subresourceRange.baseArrayLayer = BaseArrayLayer;
            ImageBarrier.subresourceRange.layerCount     = LayerCount;
        }

        CHECK(!IsInsideRenderPass());
        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
    }

    if (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS && TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES)
    {
        LocalState.SetImageLayout(NewLayout);
    }
    else
    {
        const VkImageCreateInfo& CreateInfo = VulkanTexture->GetVkImageCreateInfo();

        const uint32 BaseMip    = (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS)     ? 0 : TextureTransition.MipLevel;
        const uint32 MipCount   = (TextureTransition.MipLevel == RHI_ALL_MIP_LEVELS)     ? CreateInfo.mipLevels : 1;
        const uint32 BaseLayer  = (TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES) ? 0 : TextureTransition.ArraySlice;
        const uint32 LayerCount = (TextureTransition.ArraySlice == RHI_ALL_ARRAY_SLICES) ? CreateInfo.arrayLayers : 1;

        for (uint32 Layer = BaseLayer; Layer < BaseLayer + LayerCount; Layer++)
        {
            for (uint32 Mip = BaseMip; Mip < BaseMip + MipCount; Mip++)
            {
                const uint32 SubresourceIndex = Layer * CreateInfo.mipLevels + Mip;
                LocalState.SetSubresourceLayout(SubresourceIndex, NewLayout);
            }
        }
    }
}

void FVulkanCommandContext::TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)   
{
    FVulkanBuffer* VulkanBuffer = FVulkanRHI::ResourceCast(Buffer);
    CHECK(VulkanBuffer != nullptr);

    FVulkanBufferState& LocalState = RetrievePendingBufferState(VulkanBuffer);

    const VkAccessFlags2        BeforeAccess = FVulkanRHI::ResourceStateToAccessFlags(BeforeState);
    const VkPipelineStageFlags2 BeforeStage  = FVulkanRHI::ResourceStateToPipelineStageFlags(BeforeState);

    if (LocalState.GetAccess() == VK_ACCESS_FLAGS_2_TO_BE_DETERMINED)
    {
        FVulkanPendingBufferBarrier PendingBarrier;
        PendingBarrier.Buffer        = VulkanBuffer;
        PendingBarrier.DesiredAccess = BeforeAccess;
        PendingBarrier.DesiredStage  = BeforeStage;
        PendingBufferBarriers.Add(PendingBarrier);
    }
    else
    {
        CHECK(LocalState.GetAccess() == BeforeAccess);
    }

    VkBufferMemoryBarrier2 BufferBarrier = {};
    BufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    BufferBarrier.srcAccessMask       = BeforeAccess;
    BufferBarrier.dstAccessMask       = FVulkanRHI::ResourceStateToAccessFlags(AfterState);
    BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufferBarrier.srcStageMask        = BeforeStage;
    BufferBarrier.dstStageMask        = FVulkanRHI::ResourceStateToPipelineStageFlags(AfterState);
    BufferBarrier.buffer              = VulkanBuffer->GetVkBuffer();
    BufferBarrier.offset              = 0;
    BufferBarrier.size                = VK_WHOLE_SIZE;

    CHECK(!IsInsideRenderPass());
    BarrierBatcher.AddBufferMemoryBarrier(0, BufferBarrier);

    LocalState.SetState(FVulkanRHI::ResourceStateToAccessFlags(AfterState), FVulkanRHI::ResourceStateToPipelineStageFlags(AfterState));
}

void FVulkanCommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
    FVulkanTexture* VulkanTexture = FVulkanRHI::ResourceCast(this, Texture);
    CHECK(VulkanTexture != nullptr);

    if (VulkanTexture->GetImageLayoutState().HasDefaultLayout())
    {
        const VkImageLayout RequiredLayout = FVulkanRHI::ResourceStateToImageLayout(RequiredState.State);
        if (RequiredLayout == VulkanTexture->GetImageLayoutState().GetDefaultLayout())
        {
            return;
        }
    }

    FVulkanImageLayoutState& LocalState = RetrievePendingImageState(VulkanTexture);

    const VkAccessFlags2        DstAccess     = FVulkanRHI::ResourceStateToAccessFlags(RequiredState.State);
    const VkPipelineStageFlags2 DstStage      = FVulkanRHI::ResourceStateToPipelineStageFlags(RequiredState.State);
    const VkImageLayout         DesiredLayout = FVulkanRHI::ResourceStateToImageLayout(RequiredState.State);
    const VkImageCreateInfo&    CreateInfo    = VulkanTexture->GetVkImageCreateInfo();
    const VkImageAspectFlags    AspectMask    = GetImageAspectFlagsFromFormat(CreateInfo.format);

    if (RequiredState.MipLevel == RHI_ALL_MIP_LEVELS && RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES)
    {
        if (LocalState.AreAllSubresourcesSameLayout())
        {
            const VkImageLayout CurrentLayout = LocalState.GetImageLayout();
            if (CurrentLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
            {
                FVulkanPendingImageBarrier PendingBarrier;
                PendingBarrier.Texture       = VulkanTexture;
                PendingBarrier.DesiredLayout = DesiredLayout;
                PendingBarrier.Subresource   = RHI_ALL_MIP_LEVELS;
                PendingImageBarriers.Add(PendingBarrier);
            }
            else if (CurrentLayout != DesiredLayout)
            {
                VkImageMemoryBarrier2 ImageBarrier = {};
                ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                ImageBarrier.oldLayout                       = CurrentLayout;
                ImageBarrier.newLayout                       = DesiredLayout;
                ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.image                           = VulkanTexture->GetVkImage();
                ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask                   = DstAccess;
                ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.dstStageMask                    = DstStage;
                ImageBarrier.subresourceRange.aspectMask     = AspectMask;
                ImageBarrier.subresourceRange.baseArrayLayer = 0;
                ImageBarrier.subresourceRange.baseMipLevel   = 0;
                ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
                ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

                CHECK(!IsInsideRenderPass());
                BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
            }
        }
        else
        {
            for (uint32 i = 0; i < LocalState.GetNumSubresources(); i++)
            {
                const VkImageLayout CurrentLayout = LocalState.GetSubresourceLayout(i);
                if (CurrentLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
                {
                    FVulkanPendingImageBarrier PendingBarrier;
                    PendingBarrier.Texture       = VulkanTexture;
                    PendingBarrier.DesiredLayout = DesiredLayout;
                    PendingBarrier.Subresource   = i;
                    PendingImageBarriers.Add(PendingBarrier);
                }
                else if (CurrentLayout != DesiredLayout)
                {
                    const uint32 MipLevel   = i % CreateInfo.mipLevels;
                    const uint32 ArrayLayer = i / CreateInfo.mipLevels;

                    VkImageMemoryBarrier2 ImageBarrier = {};
                    ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                    ImageBarrier.oldLayout                       = CurrentLayout;
                    ImageBarrier.newLayout                       = DesiredLayout;
                    ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                    ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                    ImageBarrier.image                           = VulkanTexture->GetVkImage();
                    ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
                    ImageBarrier.dstAccessMask                   = DstAccess;
                    ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                    ImageBarrier.dstStageMask                    = DstStage;
                    ImageBarrier.subresourceRange.aspectMask     = AspectMask;
                    ImageBarrier.subresourceRange.baseArrayLayer = ArrayLayer;
                    ImageBarrier.subresourceRange.baseMipLevel   = MipLevel;
                    ImageBarrier.subresourceRange.layerCount     = 1;
                    ImageBarrier.subresourceRange.levelCount     = 1;

                    CHECK(!IsInsideRenderPass());
                    BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
                }
            }
        }

        LocalState.SetImageLayout(DesiredLayout);
    }
    else
    {
        const uint32 BaseMip    = (RequiredState.MipLevel == RHI_ALL_MIP_LEVELS)     ? 0 : RequiredState.MipLevel;
        const uint32 MipCount   = (RequiredState.MipLevel == RHI_ALL_MIP_LEVELS)     ? CreateInfo.mipLevels : 1;
        const uint32 BaseLayer  = (RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES) ? 0 : RequiredState.ArraySlice;
        const uint32 LayerCount = (RequiredState.ArraySlice == RHI_ALL_ARRAY_SLICES) ? CreateInfo.arrayLayers : 1;

        for (uint32 Layer = BaseLayer; Layer < BaseLayer + LayerCount; Layer++)
        {
            for (uint32 Mip = BaseMip; Mip < BaseMip + MipCount; Mip++)
            {
                const uint32        SubresourceIndex = Layer * CreateInfo.mipLevels + Mip;
                const VkImageLayout CurrentLayout    = LocalState.GetSubresourceLayout(SubresourceIndex);
                
                if (CurrentLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
                {
                    FVulkanPendingImageBarrier PendingBarrier;
                    PendingBarrier.Texture       = VulkanTexture;
                    PendingBarrier.DesiredLayout = DesiredLayout;
                    PendingBarrier.Subresource   = SubresourceIndex;
                    PendingImageBarriers.Add(PendingBarrier);
                }
                else if (CurrentLayout != DesiredLayout)
                {
                    VkImageMemoryBarrier2 ImageBarrier = {};
                    ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                    ImageBarrier.oldLayout                       = CurrentLayout;
                    ImageBarrier.newLayout                       = DesiredLayout;
                    ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                    ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                    ImageBarrier.image                           = VulkanTexture->GetVkImage();
                    ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
                    ImageBarrier.dstAccessMask                   = DstAccess;
                    ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                    ImageBarrier.dstStageMask                    = DstStage;
                    ImageBarrier.subresourceRange.aspectMask     = AspectMask;
                    ImageBarrier.subresourceRange.baseArrayLayer = Layer;
                    ImageBarrier.subresourceRange.baseMipLevel   = Mip;
                    ImageBarrier.subresourceRange.layerCount     = 1;
                    ImageBarrier.subresourceRange.levelCount     = 1;

                    CHECK(!IsInsideRenderPass());
                    BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
                }

                LocalState.SetSubresourceLayout(SubresourceIndex, DesiredLayout);
            }
        }
    }
}

void FVulkanCommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState)
{
    FVulkanBuffer* VulkanBuffer = FVulkanRHI::ResourceCast(Buffer);
    CHECK(VulkanBuffer != nullptr);

    FVulkanBufferState& LocalState = RetrievePendingBufferState(VulkanBuffer);
    
    const VkAccessFlags2        DesiredAccess = FVulkanRHI::ResourceStateToAccessFlags(RequiredState);
    const VkPipelineStageFlags2 DesiredStage  = FVulkanRHI::ResourceStateToPipelineStageFlags(RequiredState);

    if (LocalState.GetAccess() == VK_ACCESS_FLAGS_2_TO_BE_DETERMINED)
    {
        FVulkanPendingBufferBarrier PendingBarrier;
        PendingBarrier.Buffer        = VulkanBuffer;
        PendingBarrier.DesiredAccess = DesiredAccess;
        PendingBarrier.DesiredStage  = DesiredStage;

        PendingBufferBarriers.Add(PendingBarrier);
        LocalState.SetState(DesiredAccess, DesiredStage);
    }
    else if (LocalState.GetAccess() != DesiredAccess || LocalState.GetStage() != DesiredStage)
    {
        VkBufferMemoryBarrier2 BufferBarrier = {};
        BufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        BufferBarrier.srcAccessMask       = LocalState.GetAccess();
        BufferBarrier.dstAccessMask       = DesiredAccess;
        BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        BufferBarrier.srcStageMask        = LocalState.GetStage();
        BufferBarrier.dstStageMask        = DesiredStage;
        BufferBarrier.buffer              = VulkanBuffer->GetVkBuffer();
        BufferBarrier.offset              = 0;
        BufferBarrier.size                = VK_WHOLE_SIZE;

        CHECK(!IsInsideRenderPass());

        BarrierBatcher.AddBufferMemoryBarrier(0, BufferBarrier);
        LocalState.SetState(DesiredAccess, DesiredStage);
    }
}

void FVulkanCommandContext::TransitionImageLayout(FVulkanTexture* Texture, VkImageLayout AfterLayout)
{
    CHECK(Texture != nullptr);

    if (Texture->GetImageLayoutState().HasDefaultLayout())
    {
        if (AfterLayout == Texture->GetImageLayoutState().GetDefaultLayout())
        {
            return;
        }
    }

    FVulkanImageLayoutState& LocalState = RetrievePendingImageState(Texture);
    const VkImageCreateInfo& CreateInfo = Texture->GetVkImageCreateInfo();
    const VkImageAspectFlags AspectMask = GetImageAspectFlagsFromFormat(CreateInfo.format);

    if (LocalState.AreAllSubresourcesSameLayout())
    {
        const VkImageLayout CurrentLayout = LocalState.GetImageLayout();
        if (CurrentLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
        {
            FVulkanPendingImageBarrier PendingBarrier;
            PendingBarrier.Texture       = Texture;
            PendingBarrier.DesiredLayout = AfterLayout;
            PendingBarrier.Subresource   = RHI_ALL_MIP_LEVELS;
            PendingImageBarriers.Add(PendingBarrier);

            VkImageMemoryBarrier2 ImageBarrier = {};
            ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE;
            ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            ImageBarrier.oldLayout           = AfterLayout;
            ImageBarrier.newLayout           = AfterLayout;
            ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.image               = Texture->GetVkImage();
            ImageBarrier.subresourceRange    = { AspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
            BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        }
        else if (CurrentLayout != AfterLayout)
        {
            VkImageMemoryBarrier2 ImageBarrier = {};
            ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            ImageBarrier.oldLayout           = CurrentLayout;
            ImageBarrier.newLayout           = AfterLayout;
            ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.image               = Texture->GetVkImage();
            ImageBarrier.subresourceRange    = { AspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
            BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        }

        LocalState.SetImageLayout(AfterLayout);
    }
    else
    {
        const uint32 NumSubresources = CreateInfo.mipLevels * CreateInfo.arrayLayers;
        for (uint32 i = 0; i < NumSubresources; i++)
        {
            const uint32        MipLevel   = i % CreateInfo.mipLevels;
            const uint32        ArrayLayer = i / CreateInfo.mipLevels;
            const VkImageLayout SubLayout  = LocalState.GetSubresourceLayout(i);

            if (SubLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
            {
                FVulkanPendingImageBarrier PendingBarrier;
                PendingBarrier.Texture       = Texture;
                PendingBarrier.DesiredLayout = AfterLayout;
                PendingBarrier.Subresource   = i;
                PendingImageBarriers.Add(PendingBarrier);

                VkImageMemoryBarrier2 ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.oldLayout           = AfterLayout;
                ImageBarrier.newLayout           = AfterLayout;
                ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.image               = Texture->GetVkImage();
                ImageBarrier.subresourceRange    = { AspectMask, MipLevel, 1, ArrayLayer, 1 };
                BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
            }
            else if (SubLayout != AfterLayout)
            {
                VkImageMemoryBarrier2 ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.oldLayout           = SubLayout;
                ImageBarrier.newLayout           = AfterLayout;
                ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.image               = Texture->GetVkImage();
                ImageBarrier.subresourceRange    = { AspectMask, MipLevel, 1, ArrayLayer, 1 };
                BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
            }

            LocalState.SetSubresourceLayout(i, AfterLayout);
        }
    }
}

void FVulkanCommandContext::TransitionImageLayout(FVulkanTexture* Texture, VkImageLayout BeforeLayout, VkImageLayout AfterLayout)
{
    CHECK(Texture != nullptr);

    if (Texture->GetImageLayoutState().HasDefaultLayout())
    {
        if (AfterLayout == Texture->GetImageLayoutState().GetDefaultLayout())
        {
            return;
        }
    }

    FVulkanImageLayoutState& LocalState = RetrievePendingImageState(Texture);
    const VkImageCreateInfo& CreateInfo = Texture->GetVkImageCreateInfo();
    const VkImageAspectFlags AspectMask = GetImageAspectFlagsFromFormat(CreateInfo.format);

    if (LocalState.AreAllSubresourcesSameLayout())
    {
        const VkImageLayout CurrentLayout = LocalState.GetImageLayout();
        if (CurrentLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
        {
            const VkImageLayout SeedLayout = (BeforeLayout != VK_IMAGE_LAYOUT_TO_BE_DETERMINED) ? BeforeLayout : AfterLayout;

            FVulkanPendingImageBarrier PendingBarrier;
            PendingBarrier.Texture       = Texture;
            PendingBarrier.DesiredLayout = SeedLayout;
            PendingBarrier.Subresource   = RHI_ALL_MIP_LEVELS;
            PendingImageBarriers.Add(PendingBarrier);

            VkImageMemoryBarrier2 ImageBarrier = {};
            ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE;
            ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            ImageBarrier.oldLayout           = SeedLayout;
            ImageBarrier.newLayout           = AfterLayout;
            ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.image               = Texture->GetVkImage();
            ImageBarrier.subresourceRange    = { AspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
            BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        }
        else
        {
            CHECK(CurrentLayout == BeforeLayout);

            VkImageMemoryBarrier2 ImageBarrier = {};
            ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            ImageBarrier.oldLayout           = CurrentLayout;
            ImageBarrier.newLayout           = AfterLayout;
            ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.image               = Texture->GetVkImage();
            ImageBarrier.subresourceRange    = { AspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
            BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        }

        LocalState.SetImageLayout(AfterLayout);
    }
    else
    {
        const uint32 NumSubresources = CreateInfo.mipLevels * CreateInfo.arrayLayers;
        for (uint32 i = 0; i < NumSubresources; i++)
        {
            const uint32        MipLevel   = i % CreateInfo.mipLevels;
            const uint32        ArrayLayer = i / CreateInfo.mipLevels;
            const VkImageLayout SubLayout  = LocalState.GetSubresourceLayout(i);

            if (SubLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
            {
                const VkImageLayout SeedLayout = (BeforeLayout != VK_IMAGE_LAYOUT_TO_BE_DETERMINED) ? BeforeLayout : AfterLayout;

                FVulkanPendingImageBarrier PendingBarrier;
                PendingBarrier.Texture       = Texture;
                PendingBarrier.DesiredLayout = SeedLayout;
                PendingBarrier.Subresource   = i;
                PendingImageBarriers.Add(PendingBarrier);

                VkImageMemoryBarrier2 ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.oldLayout           = SeedLayout;
                ImageBarrier.newLayout           = AfterLayout;
                ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.image               = Texture->GetVkImage();
                ImageBarrier.subresourceRange    = { AspectMask, MipLevel, 1, ArrayLayer, 1 };
                BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
            }
            else
            {
                CHECK(SubLayout == BeforeLayout);

                VkImageMemoryBarrier2 ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.oldLayout           = SubLayout;
                ImageBarrier.newLayout           = AfterLayout;
                ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.image               = Texture->GetVkImage();
                ImageBarrier.subresourceRange    = { AspectMask, MipLevel, 1, ArrayLayer, 1 };
                BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
            }

            LocalState.SetSubresourceLayout(i, AfterLayout);
        }
    }
}

void FVulkanCommandContext::TransitionImageLayout(FVulkanTexture* Texture, VkImageLayout AfterLayout, uint32 FirstMip, uint32 NumMips, uint32 FirstArraySlice, uint32 NumArraySlices)
{
    CHECK(Texture != nullptr);

    FVulkanImageLayoutState& LocalState = RetrievePendingImageState(Texture);
    const VkImageCreateInfo& CreateInfo = Texture->GetVkImageCreateInfo();
    const VkImageAspectFlags AspectMask = GetImageAspectFlagsFromFormat(CreateInfo.format);

    for (uint32 ArraySlice = FirstArraySlice; ArraySlice < FirstArraySlice + NumArraySlices; ArraySlice++)
    {
        for (uint32 Mip = FirstMip; Mip < FirstMip + NumMips; Mip++)
        {
            const uint32 SubresourceIndex = ArraySlice * CreateInfo.mipLevels + Mip;
            
            const VkImageLayout SubLayout = LocalState.AreAllSubresourcesSameLayout()
                ? LocalState.GetImageLayout()
                : LocalState.GetSubresourceLayout(SubresourceIndex);

            if (SubLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
            {
                FVulkanPendingImageBarrier PendingBarrier;
                PendingBarrier.Texture       = Texture;
                PendingBarrier.DesiredLayout = AfterLayout;
                PendingBarrier.Subresource   = SubresourceIndex;
                PendingImageBarriers.Add(PendingBarrier);

                VkImageMemoryBarrier2 ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.oldLayout           = AfterLayout;
                ImageBarrier.newLayout           = AfterLayout;
                ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.image               = Texture->GetVkImage();
                ImageBarrier.subresourceRange    = { AspectMask, Mip, 1, ArraySlice, 1 };
                BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
            }
            else if (SubLayout != AfterLayout)
            {
                VkImageMemoryBarrier2 ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                ImageBarrier.oldLayout           = SubLayout;
                ImageBarrier.newLayout           = AfterLayout;
                ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                ImageBarrier.image               = Texture->GetVkImage();
                ImageBarrier.subresourceRange    = { AspectMask, Mip, 1, ArraySlice, 1 };
                BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
            }

            LocalState.SetSubresourceLayout(SubresourceIndex, AfterLayout);
        }
    }
}

void FVulkanCommandContext::TransitionImageLayout(FVulkanUnorderedAccessView* View)
{
    CHECK(View != nullptr);

    if (View->GetType() != FVulkanResourceView::EType::ImageView)
    {
        return;
    }

    FVulkanTexture* Texture = FVulkanRHI::ResourceCast(this, static_cast<FRHITexture*>(View->GetResource()));
    if (!Texture)
    {
        return;
    }

    const VkImageSubresourceRange& Range =  View->GetImageViewInfo().SubresourceRange;
    TransitionImageLayout(Texture, VK_IMAGE_LAYOUT_GENERAL, Range.baseMipLevel, Range.levelCount, Range.baseArrayLayer, Range.layerCount);
}

void FVulkanCommandContext::TransitionImageLayout(FVulkanShaderResourceView* View, VkImageLayout Layout)
{
    CHECK(View != nullptr);

    if (View->GetType() != FVulkanResourceView::EType::ImageView)
    {
        return;
    }

    FVulkanTexture* Texture = FVulkanRHI::ResourceCast(this, static_cast<FRHITexture*>(View->GetResource()));
    if (!Texture)
    {
        return;
    }

    const VkImageSubresourceRange& Range = View->GetImageViewInfo().SubresourceRange;
    TransitionImageLayout(Texture, Layout, Range.baseMipLevel, Range.levelCount, Range.baseArrayLayer, Range.layerCount);
}

void FVulkanCommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
    FVulkanTexture* VulkanTexture = FVulkanRHI::ResourceCast(this, Texture);
    CHECK(VulkanTexture != nullptr);

    VkImageMemoryBarrier2 ImageBarrier = {};
    ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.image                           = VulkanTexture->GetVkImage();
    ImageBarrier.srcAccessMask                   = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    ImageBarrier.dstAccessMask                   = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VulkanTexture->GetVkFormat());
    ImageBarrier.subresourceRange.baseArrayLayer = 0;
    ImageBarrier.subresourceRange.baseMipLevel   = 0;
    ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    CHECK(!IsInsideRenderPass());
    BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
}

void FVulkanCommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)   
{
    FVulkanBuffer* VulkanBuffer = FVulkanRHI::ResourceCast(Buffer);
    CHECK(VulkanBuffer != nullptr);

    VkBufferMemoryBarrier2 BufferBarrier = {};
    BufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    BufferBarrier.srcAccessMask       = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    BufferBarrier.dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufferBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    BufferBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    BufferBarrier.buffer              = VulkanBuffer->GetVkBuffer();
    BufferBarrier.offset              = 0;
    BufferBarrier.size                = VK_WHOLE_SIZE;

    CHECK(!IsInsideRenderPass());
    BarrierBatcher.AddBufferMemoryBarrier(0, BufferBarrier);
}

void FVulkanCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    ConditionalSplitCommandBuffer();
    ContextState.PrepareGraphicsState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindGraphicsState();
    GetCommandBuffer()->Draw(VertexCount, 1, StartVertexLocation, 0);

#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "Draw");
    }
#endif
}

void FVulkanCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    ConditionalSplitCommandBuffer();
    ContextState.PrepareGraphicsState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindGraphicsState();
    GetCommandBuffer()->DrawIndexed(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);

#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "DrawIndexed");
    }
#endif
}

void FVulkanCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    ConditionalSplitCommandBuffer();
    ContextState.PrepareGraphicsState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindGraphicsState();
    GetCommandBuffer()->Draw(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);

#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "DrawInstanced");
    }
#endif
}

void FVulkanCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    ConditionalSplitCommandBuffer();
    ContextState.PrepareGraphicsState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindGraphicsState();
    GetCommandBuffer()->DrawIndexed(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "DrawIndexedInstanced");
    }
#endif
}

void FVulkanCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    if (WorkGroupsX == 0 || WorkGroupsY == 0 || WorkGroupsZ == 0)
    {
        return;
    }

    ConditionalSplitCommandBuffer();

    ContextState.PrepareComputeState();
    ContextState.BindComputeState();

    GetCommandBuffer()->Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);

#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "Dispatch");
    }
#endif
}

void FVulkanCommandContext::DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
    // TODO: Implement Vulkan RT
    UNREFERENCED_VARIABLE(InScene);
    UNREFERENCED_VARIABLE(InPipelineState);
    UNREFERENCED_VARIABLE(InWidth);
    UNREFERENCED_VARIABLE(InHeight);
    UNREFERENCED_VARIABLE(InDepth);
}

void FVulkanCommandContext::PresentSwapChain(FRHISwapChain* InSwapChain, bool bVerticalSync)
{
    // -------------------------------------------------------------------------------------------
    // We intentionally do not retire or reset the command pool here. The goal is to maintain 
    // a single command pool per command context, per thread, per frame-in-flight. This helps 
    // avoid unnecessary command pool allocations or resets between multiple Present() calls
    // in the same frame.
    //
    // The command pool will instead be explicitly retired at the end of FinishContext(), 
    // ensuring proper lifecycle management without leaks.
    // -------------------------------------------------------------------------------------------

    FinishCommandBuffer(false);

    FVulkanSwapChain* VulkanSwapChain = FVulkanRHI::ResourceCast(InSwapChain);
    VulkanSwapChain->Present(this, bVerticalSync);

    // -------------------------------------------------------------------------------------------
    // Acquire or allocate a fresh command buffer so that subsequent GPU work can continue 
    // recording immediately after presenting.
    // -------------------------------------------------------------------------------------------

    ObtainCommandBuffer();
}

void FVulkanCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height)
{
    FVulkanSwapChain* VulkanSwapChain = FVulkanRHI::ResourceCast(SwapChain);
    VulkanSwapChain->Resize(this, Width, Height);
}

void FVulkanCommandContext::ClearState()
{
    SCOPED_LOCK(CommandContextCS);

    if (IsRecording())
    {
        if (CommandBuffer)
        {
            FinishCommandBuffer(true);
            ObtainCommandBuffer();
        }
    }
    
    Queue.WaitForCompletion();
    
    // After we know that all work on the Queue is finished we can clear the state
    ContextState.ResetState();
}

void FVulkanCommandContext::Flush()
{
    SCOPED_LOCK(CommandContextCS);
    
    if (IsRecording())
    {
        if (CommandBuffer)
        {
            FinishCommandBuffer(true);
            ObtainCommandBuffer();
        }
    }

    Queue.WaitForCompletion();
}

void FVulkanCommandContext::PushEvent(const FStringView& Name)
{
    EventStack.Emplace(Name.Data());

#if VK_EXT_debug_utils
    if (GVulkanSupportsDebugUtils)
    {
        VkDebugUtilsLabelEXT DebugUtilsLabel = {};
        DebugUtilsLabel.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        DebugUtilsLabel.pLabelName = Name.Data();
        DebugUtilsLabel.color[0]   = 0.0f;
        DebugUtilsLabel.color[1]   = 0.0f;
        DebugUtilsLabel.color[2]   = 0.0f;
        DebugUtilsLabel.color[3]   = 1.0f;
        
        GetCommandBuffer()->BeginDebugUtilsLabel(&DebugUtilsLabel);
    }
#endif

#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 1)
    {
        FVulkanRHI::Get()->GetCrashMarkers()->WriteMarker(GetCommandBuffer(), Name);
    }
#endif
}

void FVulkanCommandContext::PopEvent()
{
#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 1)
    {
        FVulkanRHI::Get()->GetCrashMarkers()->WriteEndMarker(GetCommandBuffer());
    }
#endif

    if (!EventStack.IsEmpty())
    {
        EventStack.Pop();
    }

#if VK_EXT_debug_utils
    if (GVulkanSupportsDebugUtils)
    {
        GetCommandBuffer()->EndDebugUtilsLabel();
    }
#endif
}

void FVulkanCommandContext::CloseEventStack()
{
#if VK_EXT_debug_utils
    if (GVulkanSupportsDebugUtils)
    {
        for (int32 i = EventStack.Size() - 1; i >= 0; --i)
        {
            GetCommandBuffer()->EndDebugUtilsLabel();
        }
    }
#endif
}

void FVulkanCommandContext::ReopenEventStack()
{
#if VK_EXT_debug_utils
    if (GVulkanSupportsDebugUtils)
    {
        for (int32 i = 0; i < EventStack.Size(); ++i)
        {
            VkDebugUtilsLabelEXT DebugUtilsLabel = {};
            DebugUtilsLabel.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            DebugUtilsLabel.pLabelName = *EventStack[i];
            DebugUtilsLabel.color[0]   = 0.0f;
            DebugUtilsLabel.color[1]   = 0.0f;
            DebugUtilsLabel.color[2]   = 0.0f;
            DebugUtilsLabel.color[3]   = 1.0f;

            GetCommandBuffer()->BeginDebugUtilsLabel(&DebugUtilsLabel);
        }
    }
#endif
}
