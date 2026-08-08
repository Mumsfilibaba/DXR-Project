#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHIShader.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanBufferClear.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/RayTracing/VulkanRayTracing.h"
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

struct FVulkanBarrierSubresourceRange
{
    uint32 BaseMip;
    uint32 MipCount;
    uint32 BaseLayer;
    uint32 LayerCount;
    bool   bWholeResource;
};

static FVulkanBarrierSubresourceRange VulkanResolveSubresourceRange(const VkImageCreateInfo& CreateInfo, const FRHITextureSubresourceRange& Subresources)
{
    const bool bAllMips   = (Subresources.NumMipLevels   == RHI_ALL_MIP_LEVELS);
    const bool bAllSlices = (Subresources.NumArraySlices == RHI_ALL_ARRAY_SLICES);

    FVulkanBarrierSubresourceRange Range;
    Range.BaseMip        = bAllMips   ? 0 : Subresources.FirstMipLevel;
    Range.MipCount       = bAllMips   ? CreateInfo.mipLevels : Subresources.NumMipLevels;
    Range.BaseLayer      = bAllSlices ? 0 : Subresources.FirstArraySlice;
    Range.LayerCount     = bAllSlices ? CreateInfo.arrayLayers : Subresources.NumArraySlices;
    Range.bWholeResource = bAllMips && bAllSlices;
    return Range;
}

static VkImageAspectFlags VulkanResolveAspectMask(VkFormat Format, const FRHITextureSubresourceRange& Subresources)
{
    const VkImageAspectFlags FullAspectMask = GetImageAspectFlagsFromFormat(Format);
    if (Subresources.NumPlaneSlices == RHI_ALL_PLANE_SLICES)
    {
        return FullAspectMask;
    }

    VkImageAspectFlags AspectMask = 0;
    for (uint32 Plane = Subresources.FirstPlaneSlice; Plane < Subresources.FirstPlaneSlice + Subresources.NumPlaneSlices; Plane++)
    {
        AspectMask |= (Plane == 0)
            ? (FullAspectMask & ~VkImageAspectFlags(VK_IMAGE_ASPECT_STENCIL_BIT))
            : (FullAspectMask &  VkImageAspectFlags(VK_IMAGE_ASPECT_STENCIL_BIT));
    }

    return (AspectMask != 0) ? AspectMask : FullAspectMask;
}

void FVulkanBarrierBatcher::AddMemoryBarrier(VkDependencyFlags DependencyFlags, const VkMemoryBarrier2KHR& InBarrier)
{
    CHECK(InBarrier.sType == VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR);

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

void FVulkanBarrierBatcher::AddBufferMemoryBarrier(VkDependencyFlags DependencyFlags, const VkBufferMemoryBarrier2KHR& InBarrier)
{
    CHECK(InBarrier.sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR);
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

void FVulkanBarrierBatcher::AddImageMemoryBarrier(VkDependencyFlags DependencyFlags, const VkImageMemoryBarrier2KHR& InIncomingBarrier)
{
    CHECK(InIncomingBarrier.sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR);
    CHECK(InIncomingBarrier.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
    CHECK(InIncomingBarrier.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);

    VkImageMemoryBarrier2KHR InBarrier = InIncomingBarrier;

#if VK_EXT_sample_locations
    constexpr VkImageAspectFlags DepthStencilAspects = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    if (bHasCustomSampleLocations && (InBarrier.subresourceRange.aspectMask & DepthStencilAspects) != 0)
    {
        CHECK(InBarrier.pNext == nullptr);
        InBarrier.pNext = &SampleLocationsInfo;
    }
#endif

    for (FBatch& Batch : Batches)
    {
        if (Batch.DependencyFlags == DependencyFlags)
        {
            // Coalesce with existing barrier if same image + subresource range
            for (VkImageMemoryBarrier2KHR& Barrier : Batch.ImageMemoryBarriers)
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

#if VK_EXT_sample_locations
void FVulkanBarrierBatcher::SetCustomSampleLocations(const VkSampleLocationsInfoEXT* InSampleLocationsInfo)
{
    if (!InSampleLocationsInfo || GVulkanVariableSampleLocations)
    {
        bHasCustomSampleLocations = false;
        return;
    }

    CHECK(InSampleLocationsInfo->sampleLocationsCount <= RHI_MAX_SAMPLE_POSITIONS);
    Memory::Memcpy(SampleLocations, InSampleLocationsInfo->pSampleLocations, sizeof(VkSampleLocationEXT) * InSampleLocationsInfo->sampleLocationsCount);

    SampleLocationsInfo                  = *InSampleLocationsInfo;
    SampleLocationsInfo.pNext            = nullptr;
    SampleLocationsInfo.pSampleLocations = SampleLocations;

    bHasCustomSampleLocations = true;
}
#endif

void FVulkanBarrierBatcher::FlushBarriers(FVulkanCommandBuffer& CommandBuffer)
{
    VkDependencyInfoKHR DependencyInfo = {};
    DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;

    for (FBatch& Batch : Batches)
    {
    #if VULKAN_VALIDATE_IMAGE_LAYOUTS
        for (const VkImageMemoryBarrier2KHR& Barrier : Batch.ImageMemoryBarriers)
        {
            const VkImageSubresourceRange& Range = Barrier.subresourceRange;
            const bool bWholeImage = 
                Range.baseMipLevel == 0 && Range.levelCount == VK_REMAINING_MIP_LEVELS &&
                Range.baseArrayLayer == 0 && Range.layerCount == VK_REMAINING_ARRAY_LAYERS;

            CommandBuffer.AddImageLayoutForValidation(Barrier.image, Barrier.oldLayout, Barrier.newLayout, bWholeImage, "PipelineBarrier");
        }
    #endif

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
    , TransientDescriptorAllocator(nullptr)
    , ActiveQueryCount(0)
    , LastUsedFrame(0)
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

void* FVulkanCommandContext::GetRHINativeCommandList()
{
    return reinterpret_cast<void*>(CommandBuffer->GetVkCommandBuffer());
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
    FVulkanDeviceRHI::Get()->BeginFrame();

    if (NeedsCommandBuffer())
    {
        ObtainCommandBuffer();
    }

    GetDevice()->GetFrameFence().Signal(GetCommands());
}

void FVulkanCommandContext::EndFrame()
{
    FVulkanDeviceRHI::Get()->EndFrame();
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

        ContextState.BeginCommandBuffer();

        FVulkanDeviceRHI::Get()->NotifyCommandBufferOpened();
    }

    if (!Commands)
    {
        Commands = new FVulkanCommands(GetDevice(), Queue);
        Commands->AcquireFence();
    }
}

FVulkanImageLayoutState& FVulkanCommandContext::RetrievePendingImageState(FVulkanTextureRHI* Texture)
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

FVulkanBufferState& FVulkanCommandContext::RetrievePendingBufferState(FVulkanBufferRHI* Buffer)
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

    ContextState.EndCommandBuffer();

    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    CommandBuffer->InsertEndTimestamp(TimestampQueryAllocator);

    const bool bHasPendingState = 
        !PendingImageBarriers.IsEmpty() || !PendingBufferBarriers.IsEmpty() ||
        !PendingImageStates.IsEmpty() || !PendingBufferStates.IsEmpty();

    const uint32 NumCommands = CommandBuffer->GetNumCommands();

#if DEBUG_BUILD
    const uint32 CommandBudget = static_cast<uint32>(CVarMaxCommandsPerCommandBuffer.GetValue());
    if (NumCommands > (CommandBudget * 2))
    {
        VULKAN_WARNING("Command-buffer closed with %u commands against a budget of %u - a recording path is likely missing a ConditionalSplitCommandBuffer() call", NumCommands, CommandBudget);
    }
#endif

    if (NumCommands == 0 && !bHasPendingState && !Commands->HasPendingSemaphores())
    {
        CommandBuffer->End();
        CommandPool->RecycleBuffer(CommandBuffer);
        CommandBuffer = nullptr;

        FVulkanDeviceRHI::Get()->NotifyCommandBufferRetired(nullptr);

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
    UNREFERENCED_VARIABLE(bResolveQueries);

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

    if (FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
    {
        if (BindlessManager->IsEnabled())
        {
            BindlessManager->Flush();
        }
    }

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

    FVulkanDeviceRHI::Get()->NotifyCommandBufferRetired(Commands);
    Commands->Queue.SubmitCommands(Commands);
    Commands = nullptr;
}

void FVulkanCommandContext::SplitCommandBuffer(bool bFlushPool, bool bWaitForQueue)
{
    if (CommandBuffer)
    {
    #if VULKAN_ENABLE_CRASH_MARKERS
        if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 1)
        {
            FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteSplitMarker(GetCommandBuffer());
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
    VerifyOwnerThread();

    if (!CommandBuffer || ActiveQueryCount > 0)
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

void FVulkanCommandContext::RetireTransientObjects()
{
    CHECK(!IsRecording());

    if (CommandBuffer)
    {
        FinishCommandBuffer(true);
    }

    if (CommandPool)
    {
        Queue.RetireCommandPoolDeferred(CommandPool);
        CommandPool = nullptr;
    }

    TArray<FVulkanQueryRange> AbandonedRanges;
    TimestampQueryAllocator.Reset(AbandonedRanges);
    OcclusionQueryAllocator.Reset(AbandonedRanges);
    PipelineStatsQueryAllocator.Reset(AbandonedRanges);

    for (const FVulkanQueryRange& Range : AbandonedRanges)
    {
        GetDevice()->RecycleQueryPool(Range.Pool);
    }

    PendingImageBarriers.Clear();
    PendingBufferBarriers.Clear();
    PendingImageStates.Clear();
    PendingBufferStates.Clear();
    PendingQueries.Clear();
    EventStack.Clear();

    ContextState.ResetState();
    CHECK(Commands == nullptr);
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
    // A context is recorded by exactly one thread for the length of a session. Contexts are 
    // borrowed from the queue rather than shared, so this only stamps the owning thread for the 
    // asserts that catch a second thread wandering in.
    // -------------------------------------------------------------------------------------------

    AcquireOwnership();

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
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() > 0)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->ResetMarkers(GetCommandBuffer());
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
    // The session is over, so the context can be handed to another thread.
    // -------------------------------------------------------------------------------------------
    
    ReleaseOwnership();
}

#if VULKAN_VALIDATE_CONTEXT_THREAD_OWNERSHIP
void FVulkanCommandContext::AcquireOwnership()
{
    CHECK(OwnerThreadID.Load() == CORE_INVALID_THREAD_ID);
    OwnerThreadID.Store(FPlatformTLS::GetCurrentThreadID());
}

void FVulkanCommandContext::ReleaseOwnership()
{
    VerifyOwnerThread();
    OwnerThreadID.Store(CORE_INVALID_THREAD_ID);
}

void FVulkanCommandContext::VerifyOwnerThread() const
{
    CHECK(OwnerThreadID.Load() == FPlatformTLS::GetCurrentThreadID());
}

void FVulkanCommandContext::VerifyExclusiveAccess() const
{
    const uint32 CurrentOwner = OwnerThreadID.Load();
    CHECK(CurrentOwner == CORE_INVALID_THREAD_ID || CurrentOwner == FPlatformTLS::GetCurrentThreadID());
}
#endif

void FVulkanCommandContext::BeginQuery(FRHIQuery* Query)
{
    FVulkanQueryRHI* VulkanQuery = FVulkanDeviceRHI::ResourceCast(Query);
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
    FVulkanQueryRHI* VulkanQuery = FVulkanDeviceRHI::ResourceCast(Query);
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
    FVulkanQueryRHI* VulkanQuery = FVulkanDeviceRHI::ResourceCast(Query);
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

void FVulkanCommandContext::ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor)
{
    FVulkanRenderTargetViewRHI* VulkanRenderTargetView = FVulkanDeviceRHI::ResourceCast(RenderTargetView);
    CHECK(VulkanRenderTargetView != nullptr);

    ConditionalSplitCommandBuffer();

    const FVulkanResourceView::FImageView& ImageViewInfo = VulkanRenderTargetView->GetImageViewInfo();

    {
        // NOTE: Here the image is expected to be in a "RenderTargetState" so we need to transition
        // it to TransferDst, we then need to transition back when the clear is done.
        VkImageMemoryBarrier2KHR ImageBarrier = {};
        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.oldLayout                       = FVulkanDeviceRHI::ResourceStateToImageLayout(ERHIResourceState::RenderTarget);
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = ImageViewInfo.Image;
        ImageBarrier.srcAccessMask                   = FVulkanDeviceRHI::ResourceStateToAccessFlags(ERHIResourceState::RenderTarget);
        ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        ImageBarrier.subresourceRange.aspectMask     = ImageViewInfo.SubresourceRange.aspectMask;
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        VkClearColorValue VulkanClearColor;
        Memory::Memcpy(VulkanClearColor.float32, ClearColor.XYZW, sizeof(VulkanClearColor.float32));

        GetCommandBuffer()->ClearColorImage(
            ImageViewInfo.Image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            &VulkanClearColor, 
            1, 
            &ImageViewInfo.SubresourceRange);
        
        // .. And transition back into "RenderTargetState"
        ImageBarrier.newLayout           = FVulkanDeviceRHI::ResourceStateToImageLayout(ERHIResourceState::RenderTarget);
        ImageBarrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstAccessMask       = FVulkanDeviceRHI::ResourceStateToAccessFlags(ERHIResourceState::RenderTarget);
        ImageBarrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;

        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
    }
}

void FVulkanCommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
    FVulkanDepthStencilViewRHI* VulkanDepthStencilView = FVulkanDeviceRHI::ResourceCast(DepthStencilView);
    CHECK(VulkanDepthStencilView != nullptr);

    ConditionalSplitCommandBuffer();

    const FVulkanResourceView::FImageView& ImageViewInfo = VulkanDepthStencilView->GetImageViewInfo();

    {
        // NOTE: Here the image is expected to be in a "DepthStencilState" so we need to transition 
        // it to TransferDst, we then need to transition back when the clear is done.
        VkImageMemoryBarrier2KHR ImageBarrier = {};
        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.oldLayout                       = FVulkanDeviceRHI::ResourceStateToImageLayout(ERHIResourceState::DepthWrite);
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = ImageViewInfo.Image;
        ImageBarrier.srcAccessMask                   = FVulkanDeviceRHI::ResourceStateToAccessFlags(ERHIResourceState::DepthWrite);
        ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        ImageBarrier.subresourceRange.aspectMask     = ImageViewInfo.SubresourceRange.aspectMask;
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        VkClearDepthStencilValue DepthStencilValue;
        DepthStencilValue.depth   = Depth;
        DepthStencilValue.stencil = Stencil;

        GetCommandBuffer()->ClearDepthStencilImage(
            ImageViewInfo.Image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            &DepthStencilValue, 
            1, 
            &ImageViewInfo.SubresourceRange);

        // .. And transition back into "DepthStencilState"
        ImageBarrier.newLayout           = FVulkanDeviceRHI::ResourceStateToImageLayout(ERHIResourceState::DepthWrite);
        ImageBarrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstAccessMask       = FVulkanDeviceRHI::ResourceStateToAccessFlags(ERHIResourceState::DepthWrite);
        ImageBarrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;

        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
    }
}

bool FVulkanCommandContext::ClearBufferUnorderedAccessViewCompute(FVulkanUnorderedAccessViewRHI* View, const FVulkanBufferClearRegion& Region, const uint32 Values[4], bool bIsFloat)
{
    if (View->GetType() != FVulkanResourceView::EType::TypedBufferView)
    {
        return false;
    }

    const FRHIUnorderedAccessViewDesc::FBufferUAV& BufferUAV = View->GetDesc().Buffer;

    EVulkanBufferClearType ClearType;
    if (!VulkanClearBufferUAV::GetClearType(BufferUAV.Format, ClearType))
    {
        return false;
    }

    const bool bIsFloatFormat = (ClearType == EVulkanBufferClearType::Float);
    if (bIsFloat != bIsFloatFormat)
    {
        return false;
    }

    const uint32 ElementStride = GetByteStrideFromFormat(BufferUAV.Format);
    if (ElementStride == 0)
    {
        return false;
    }

    const uint32 NumElements = (BufferUAV.NumElements != 0) ? BufferUAV.NumElements : static_cast<uint32>(Region.Size / ElementStride);
    if (NumElements == 0)
    {
        return true;
    }

    FVulkanComputePipelineStateRHI* ClearPipeline = GetDevice()->GetBufferClearPipelines().GetOrCreatePipeline(*GetDevice(), ClearType);
    if (!ClearPipeline)
    {
        return false;
    }

    FVulkanPipelineLayout* Layout = ClearPipeline->GetPipelineLayout();
    if (!Layout)
    {
        return false;
    }

    uint32 DescriptorSetIndex;
    uint32 BindingIndex;
    if (!Layout->GetDescriptorBinding(EShaderVisibility::Compute, EResourceType::UAV, 0, DescriptorSetIndex, BindingIndex))
    {
        VULKAN_ERROR("The internal buffer-clear shader does not expose its output buffer");
        return false;
    }

    const VkBufferView BufferView = View->GetTypedBufferInfo().BufferView;

    VkWriteDescriptorSet DescriptorWrite = {};
    DescriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    DescriptorWrite.descriptorCount  = 1;
    DescriptorWrite.dstBinding       = BindingIndex;
    DescriptorWrite.pTexelBufferView = &BufferView;

    const VkDescriptorSetLayout SetLayout = Layout->GetVkDescriptorSetLayout(DescriptorSetIndex);

    FVulkanDescriptorSetBuilder DescriptorSetBuilder;
    DescriptorSetBuilder.SetupDescriptorWrites(SetLayout, &DescriptorWrite, 1);
    DescriptorSetBuilder.WriteStorageTexelBuffer(0, BufferView);

    FVulkanDescriptorPoolInfo PoolInfo;
    PoolInfo.DescriptorSetLayout = SetLayout;
    PoolInfo.DescriptorSizes.Emplace(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1);
    PoolInfo.GenerateHash();

    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

#if VULKAN_USE_DESCRIPTOR_CACHE
    DescriptorSetBuilder.UpdateHash();
    if (!GetDevice()->GetDescriptorSetCache().FindOrCreateDescriptorSet(PoolInfo, DescriptorSetBuilder, DescriptorSet))
#else
    if (!GetTransientDescriptorAllocator()->AllocateDescriptorSet(PoolInfo, DescriptorSetBuilder, DescriptorSet))
#endif
    {
        VULKAN_ERROR("Failed to allocate a DescriptorSet for the internal buffer-clear dispatch");
        return false;
    }

    RequireBufferState(View, ERHIResourceState::UnorderedAccess);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    struct FClearConstants
    {
        uint32 ClearValue[4];
        uint32 NumElements;
    } ClearConstants;

    Memory::Memcpy(ClearConstants.ClearValue, Values, sizeof(ClearConstants.ClearValue));
    ClearConstants.NumElements = NumElements;

    const FPushConstantsInfo& ConstantsInfo = Layout->GetConstantsInfo();
    const VkPipelineLayout    VulkanLayout  = Layout->GetVkPipelineLayout();

    const uint32 FirstSet = Layout->HasBindlessSet() ? (VULKAN_BINDLESS_RUNTIME_SET_INDEX + 1 + DescriptorSetIndex) : DescriptorSetIndex;

    GetCommandBuffer()->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, ClearPipeline->GetVkPipeline());
    GetCommandBuffer()->BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, VulkanLayout, FirstSet, 1, &DescriptorSet, 0, nullptr);
    GetCommandBuffer()->PushConstants(VulkanLayout, ConstantsInfo.StageFlags, 0,
        Math::Min<uint32>(ConstantsInfo.NumConstants * sizeof(uint32), sizeof(ClearConstants)), &ClearConstants);

    constexpr uint32 NumThreadsPerGroup = 64;
    GetCommandBuffer()->Dispatch(Math::DivideByMultiple(NumElements, NumThreadsPerGroup), 1, 1);

    ContextState.DirtyComputeBindings();
    return true;
}

void FVulkanCommandContext::ClearBufferUnorderedAccessView(FVulkanUnorderedAccessViewRHI* View, const uint32 Values[4], bool bIsFloat)
{
    const FVulkanBufferClearRegion Region = VulkanClearBufferUAV::ResolveRegion(View);
    if (!Region.bIsValid)
    {
        return;
    }

    const FRHIUnorderedAccessViewDesc::FBufferUAV& BufferUAV = View->GetDesc().Buffer;

    uint32 Pattern = 0;
    if (!VulkanClearBufferUAV::PackPattern(BufferUAV.Type, BufferUAV.Format, Values, bIsFloat, Pattern))
    {
        if (ClearBufferUnorderedAccessViewCompute(View, Region, Values, bIsFloat))
        {
            return;
        }

        VULKAN_WARNING("Clear of a '%s' buffer UAV cannot be expressed as a 32-bit fill; using the first component", ToString(BufferUAV.Format));
    }

    RequireBufferState(View, ERHIResourceState::CopyDest);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->FillBuffer(Region.Buffer, Region.Offset, Region.Size, Pattern);
}

void FVulkanCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
    FVulkanUnorderedAccessViewRHI* VulkanUnorderedAccessView = FVulkanDeviceRHI::ResourceCast(UnorderedAccessView);
    CHECK(VulkanUnorderedAccessView != nullptr);

    ConditionalSplitCommandBuffer();

    const FVulkanResourceView::EType Type = VulkanUnorderedAccessView->GetType();
    if (Type == FVulkanResourceView::EType::ImageView)
    {
        VkClearColorValue VulkanClearColor;
        Memory::Memcpy(VulkanClearColor.float32, ClearColor.XYZW, sizeof(VulkanClearColor.float32));

        TransitionImageLayout(VulkanUnorderedAccessView);
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        const FVulkanResourceView::FImageView& ImageViewInfo = VulkanUnorderedAccessView->GetImageViewInfo();
        GetCommandBuffer()->ClearColorImage(
            ImageViewInfo.Image, 
            VK_IMAGE_LAYOUT_GENERAL, 
            &VulkanClearColor, 
            1, 
            &ImageViewInfo.SubresourceRange);
    }
    else if (Type == FVulkanResourceView::EType::StructuredBufferView || Type == FVulkanResourceView::EType::TypedBufferView)
    {
        const uint32 Values[4] =
        {
            BitCast<uint32>(ClearColor.X),
            BitCast<uint32>(ClearColor.Y),
            BitCast<uint32>(ClearColor.Z),
            BitCast<uint32>(ClearColor.W),
        };

        ClearBufferUnorderedAccessView(VulkanUnorderedAccessView, Values, true);
    }
    else if (Type == FVulkanResourceView::EType::AccelerationStructureView)
    {
        // Only an SRV is ever initialized over an acceleration structure, so a UAV carrying this
        // type means the view's type field is corrupt.
        CHECKF(false, "ClearUnorderedAccessViewFloat: a UAV cannot view an acceleration structure");
    }
    else
    {
        VULKAN_ERROR("ClearUnorderedAccessViewFloat: the UAV was never successfully initialized");
    }
}

void FVulkanCommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
    FVulkanUnorderedAccessViewRHI* VulkanUnorderedAccessView = FVulkanDeviceRHI::ResourceCast(UnorderedAccessView);
    CHECK(VulkanUnorderedAccessView != nullptr);

    ConditionalSplitCommandBuffer();

    const FVulkanResourceView::EType Type = VulkanUnorderedAccessView->GetType();
    if (Type == FVulkanResourceView::EType::ImageView)
    {
        VkClearColorValue VulkanClearColor;
        Memory::Memcpy(VulkanClearColor.uint32, Values, sizeof(VulkanClearColor.uint32));

        TransitionImageLayout(VulkanUnorderedAccessView);
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        const FVulkanResourceView::FImageView& ImageViewInfo = VulkanUnorderedAccessView->GetImageViewInfo();
        GetCommandBuffer()->ClearColorImage(
            ImageViewInfo.Image, 
            VK_IMAGE_LAYOUT_GENERAL, 
            &VulkanClearColor, 
            1, 
            &ImageViewInfo.SubresourceRange);
    }
    else if (Type == FVulkanResourceView::EType::StructuredBufferView || Type == FVulkanResourceView::EType::TypedBufferView)
    {
        ClearBufferUnorderedAccessView(VulkanUnorderedAccessView, Values, false);
    }
    else if (Type == FVulkanResourceView::EType::AccelerationStructureView)
    {
        // Only an SRV is ever initialized over an acceleration structure, so a UAV carrying this
        // type means the view's type field is corrupt.
        CHECKF(false, "ClearUnorderedAccessViewUint: a UAV cannot view an acceleration structure");
    }
    else
    {
        VULKAN_ERROR("ClearUnorderedAccessViewUint: the UAV was never successfully initialized");
    }
}

void FVulkanCommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    CHECK(ContextState.IsRecording() && !ContextState.IsInsideRenderPass() && !ContextState.IsRenderPassPaused());
    ContextState.BeginRenderPass(BeginRenderPassDesc);
}

void FVulkanCommandContext::EndRenderPass()  
{
    CHECK(ContextState.IsInsideRenderPass());
    ContextState.EndRenderPass();
}

void FVulkanCommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    VkViewport Viewport = {};
#if VULKAN_ENABLE_NEGATIVE_VIEWPORT_HEIGHT
    Viewport.width    =  ViewportRegion.Width;
    Viewport.height   = -ViewportRegion.Height;
    Viewport.maxDepth =  ViewportRegion.MaxDepth;
    Viewport.minDepth =  ViewportRegion.MinDepth;
    Viewport.x        =  ViewportRegion.PositionX;
    Viewport.y        =  ViewportRegion.Height - ViewportRegion.PositionY;
#else
    Viewport.width    = ViewportRegion.Width;
    Viewport.height   = ViewportRegion.Height;
    Viewport.maxDepth = ViewportRegion.MaxDepth;
    Viewport.minDepth = ViewportRegion.MinDepth;
    Viewport.x        = ViewportRegion.PositionX;
    Viewport.y        = ViewportRegion.PositionY;
#endif

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

void FVulkanCommandContext::SetBlendFactor(const Vector4& Color)
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

void FVulkanCommandContext::SetDepthBounds(float MinDepth, float MaxDepth)
{
    ContextState.SetDepthBounds(MinDepth, MaxDepth);
}

void FVulkanCommandContext::SetSamplePositions(const FRHISamplePositionsDesc& SamplePositionsDesc)
{
    ContextState.SetSamplePositions(SamplePositionsDesc);

#if VK_EXT_sample_locations
    BarrierBatcher.SetCustomSampleLocations(ContextState.GetCustomSampleLocationsInfo());
#endif
}

void FVulkanCommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    ContextState.SetStreamOutputTargets(Buffers, Offsets);
}

void FVulkanCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 Index = 0; Index < InVertexBuffers.Size(); ++Index)
    {
        FVulkanBufferRHI* VulkanVertexBuffer = FVulkanDeviceRHI::ResourceCast(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(VulkanVertexBuffer, BufferSlot + Index);
    }
}

void FVulkanCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FVulkanBufferRHI* VulkanIndexBuffer = FVulkanDeviceRHI::ResourceCast(IndexBuffer);
    ContextState.SetIndexBuffer(VulkanIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FVulkanCommandContext::SetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
    FVulkanGraphicsPipelineStateRHI* VulkanPipelineState = FVulkanDeviceRHI::ResourceCast(PipelineState);
    ContextState.SetGraphicsPipelineState(VulkanPipelineState);
}

void FVulkanCommandContext::SetComputePipelineState(class FRHIComputePipelineState* PipelineState)  
{
    FVulkanComputePipelineStateRHI* VulkanPipelineState = FVulkanDeviceRHI::ResourceCast(PipelineState);
    ContextState.SetComputePipelineState(VulkanPipelineState);
}

void FVulkanCommandContext::SetMeshletPipelineState(class FRHIMeshletPipelineState* PipelineState)
{
    FVulkanMeshletPipelineStateRHI* VulkanPipelineState = FVulkanDeviceRHI::ResourceCast(PipelineState);
    ContextState.SetMeshletPipelineState(VulkanPipelineState);
}

void FVulkanCommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    MAYBE_UNUSED FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    ContextState.SetPushConstants(Shader->GetShaderStage(), reinterpret_cast<const uint32*>(ShaderConstants), NumShaderConstants);
}

void FVulkanCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    FVulkanShaderResourceViewRHI* VulkanShaderResourceView = FVulkanDeviceRHI::ResourceCast(ShaderResourceView);
    ContextState.SetSRV(VulkanShaderResourceView, VulkanShader->GetShaderVisibility(), RegisterIndex);
}

void FVulkanCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex + InShaderResourceViews.Size() <= VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);

    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        FVulkanShaderResourceViewRHI* VulkanShaderResourceView = FVulkanDeviceRHI::ResourceCast(InShaderResourceViews[Index]);
        ContextState.SetSRV(VulkanShaderResourceView, VulkanShader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FVulkanCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    FVulkanUnorderedAccessViewRHI* VulkanUnorderedAccessView = FVulkanDeviceRHI::ResourceCast(UnorderedAccessView);
    ContextState.SetUAV(VulkanUnorderedAccessView, VulkanShader->GetShaderVisibility(), RegisterIndex);
}

void FVulkanCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex + InUnorderedAccessViews.Size() <= VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);

    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        FVulkanUnorderedAccessViewRHI* VulkanUnorderedAccessView = FVulkanDeviceRHI::ResourceCast(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(VulkanUnorderedAccessView, VulkanShader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FVulkanCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT);

    FVulkanBufferRHI* VulkanConstantBuffer = FVulkanDeviceRHI::ResourceCast(ConstantBuffer);
    ContextState.SetUniformBuffer(VulkanConstantBuffer, VulkanShader->GetShaderVisibility(), RegisterIndex);
}

void FVulkanCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex + InConstantBuffers.Size() <= VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT);

    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        FVulkanBufferRHI* VulkanConstantBuffer = FVulkanDeviceRHI::ResourceCast(InConstantBuffers[Index]);
        ContextState.SetUniformBuffer(VulkanConstantBuffer, VulkanShader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FVulkanCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex < VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    FVulkanSamplerStateRHI* VulkanSamplerState = FVulkanDeviceRHI::ResourceCast(SamplerState);
    ContextState.SetSampler(VulkanSamplerState, VulkanShader->GetShaderVisibility(), RegisterIndex);
}

void FVulkanCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
    FVulkanShader* VulkanShader = GetVulkanShader(Shader);
    CHECK(VulkanShader != nullptr);
    CHECK(RegisterIndex + InSamplerStates.Size() <= VULKAN_DEFAULT_SAMPLER_STATE_COUNT);

    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        FVulkanSamplerStateRHI* VulkanSamplerState = FVulkanDeviceRHI::ResourceCast(InSamplerStates[Index]);
        ContextState.SetSampler(VulkanSamplerState, VulkanShader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FVulkanCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)     
{
    FVulkanBufferRHI* VulkanBuffer = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(VulkanBuffer != nullptr);

    if (VulkanBuffer->GetDesc().IsTransient())
    {
        FVulkanMemoryLocation NewLocation(GetDevice());
        void* MappedMemory = GetDevice()->GetMemoryManager().AllocateConstants(BufferRegion.Size, VulkanBuffer->GetRequiredAlignment(), NewLocation);
        CHECK(MappedMemory != nullptr);

        Memory::Memcpy(MappedMemory, SrcData, BufferRegion.Size);
        VulkanBuffer->GetMemoryLocation().Swap(NewLocation);
        VulkanBuffer->ResourceRelocated(&VulkanBuffer->GetMemoryLocation());
    }
    else if (VulkanBuffer->GetDesc().IsDynamic())
    {
        void* BufferData = VulkanBuffer->Map(BufferRegion.Offset, BufferRegion.Size);
        if (!BufferData)
        {
            VULKAN_ERROR_CRITICAL("Failed to map buffer memory");
            return;
        }

        Memory::Memcpy(BufferData, SrcData, BufferRegion.Size);
        VulkanBuffer->Unmap(BufferRegion.Offset, BufferRegion.Size);
    }
    else
    {
        ConditionalSplitCommandBuffer();

        FVulkanMemoryLocation UploadLocation(GetDevice());
        void* MappedMemory = GetDevice()->GetMemoryManager().AllocateUploadMemory(
            BufferRegion.Size, 
            1, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            UploadLocation);
        CHECK(MappedMemory != nullptr);

        Memory::Memcpy(MappedMemory, SrcData, BufferRegion.Size);
        
        VkBufferCopy BufferCopy = {};
        BufferCopy.srcOffset = UploadLocation.GetBufferOffset();
        BufferCopy.dstOffset = VulkanBuffer->GetBindOffset() + BufferRegion.Offset;
        BufferCopy.size      = BufferRegion.Size;

        RequireBufferState(VulkanBuffer, ERHIResourceState::CopyDest);
        BarrierBatcher.FlushBarriers(GetCommandBuffer());

        GetCommandBuffer()->CopyBuffer(
            UploadLocation.GetBackingBuffer(), 
            VulkanBuffer->GetBindVkBuffer(), 
            1, 
            &BufferCopy);
    }
}

void FVulkanCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) 
{
    FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(VulkanTexture != nullptr);

    ConditionalSplitCommandBuffer();

    const VkFormat Format       = VulkanTexture->GetVkFormat();
    const uint64   RequiredSize = VkCalculateTextureUploadSize(Format, TextureRegion.Width, TextureRegion.Height);
    const uint64   Alignment    = GetDevice()->GetPhysicalDevice()->GetProperties().limits.optimalBufferCopyOffsetAlignment;

    FVulkanMemoryLocation UploadLocation(GetDevice());
    uint8* UploadMemory = static_cast<uint8*>(GetDevice()->GetMemoryManager().AllocateUploadMemory(
        RequiredSize, 
        Alignment, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        UploadLocation));
    CHECK(UploadMemory != nullptr);

    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);
    CHECK(Source != nullptr);
    
    const uint32 RowPitch = VkCalculateTextureRowPitch(Format, TextureRegion.Width);
    const uint32 NumRows  = VkCalculateTextureNumRows(Format, TextureRegion.Height);

    for (uint64 y = 0; y < NumRows; y++)
    {
        Memory::Memcpy(UploadMemory, Source, RowPitch);
        Source       += SrcRowPitch;
        UploadMemory += RowPitch;
    }

    VkBufferImageCopy BufferImageCopy = {};
    BufferImageCopy.bufferOffset                    = UploadLocation.GetBufferOffset();
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = GetImageAspectFlagsFromFormat(Format);
    BufferImageCopy.imageSubresource.mipLevel       = MipLevel;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { static_cast<int32>(TextureRegion.PositionX), static_cast<int32>(TextureRegion.PositionY), 0 };
    BufferImageCopy.imageExtent                     = { TextureRegion.Width, TextureRegion.Height, 1 };

    TransitionImageLayout(VulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyBufferToImage(
        UploadLocation.GetBackingBuffer(), 
        VulkanTexture->GetVkImage(), 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        1, 
        &BufferImageCopy);
}

void FVulkanCommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
    FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(VulkanTexture != nullptr);

    ConditionalSplitCommandBuffer();

    const VkFormat Format       = VulkanTexture->GetVkFormat();
    const uint32   RowPitch     = VkCalculateTextureRowPitch(Format, TextureRegion.Width);
    const uint32   NumRows      = VkCalculateTextureNumRows(Format, TextureRegion.Height);
    const uint64   SliceSize    = static_cast<uint64>(RowPitch) * NumRows;
    const uint64   RequiredSize = SliceSize * TextureRegion.Depth;
    const uint64   Alignment    = GetDevice()->GetPhysicalDevice()->GetProperties().limits.optimalBufferCopyOffsetAlignment;

    FVulkanMemoryLocation UploadLocation(GetDevice());
    uint8* UploadMemory = static_cast<uint8*>(GetDevice()->GetMemoryManager().AllocateUploadMemory(
        RequiredSize, 
        Alignment, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        UploadLocation));
    CHECK(UploadMemory != nullptr);

    const uint8* Source = reinterpret_cast<const uint8*>(SrcData);
    CHECK(Source != nullptr);

    for (uint32 z = 0; z < TextureRegion.Depth; z++)
    {
        const uint8* SliceSource = Source + z * SrcDepthPitch;
        for (uint32 y = 0; y < NumRows; y++)
        {
            Memory::Memcpy(UploadMemory, SliceSource, RowPitch);
            SliceSource  += SrcRowPitch;
            UploadMemory += RowPitch;
        }
    }

    VkBufferImageCopy BufferImageCopy = {};
    BufferImageCopy.bufferOffset                    = UploadLocation.GetBufferOffset();
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = GetImageAspectFlagsFromFormat(Format);
    BufferImageCopy.imageSubresource.mipLevel       = MipLevel;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { static_cast<int32>(TextureRegion.PositionX), static_cast<int32>(TextureRegion.PositionY), static_cast<int32>(TextureRegion.PositionZ) };
    BufferImageCopy.imageExtent                     = { TextureRegion.Width, TextureRegion.Height, TextureRegion.Depth };

    TransitionImageLayout(VulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyBufferToImage(
        UploadLocation.GetBackingBuffer(), 
        VulkanTexture->GetVkImage(), 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        1, 
        &BufferImageCopy);
}

void FVulkanCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FVulkanTextureRHI* SrcVulkanTexture = FVulkanDeviceRHI::ResourceCast(Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTextureRHI* DstVulkanTexture = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    CHECK(SrcVulkanTexture->GetDesc().Extent.X == DstVulkanTexture->GetDesc().Extent.X);
    CHECK(SrcVulkanTexture->GetDesc().Extent.Y == DstVulkanTexture->GetDesc().Extent.Y);
    CHECK(SrcVulkanTexture->GetDesc().Extent.Z == DstVulkanTexture->GetDesc().Extent.Z);

    ConditionalSplitCommandBuffer();

    VkImageResolve ImageResolve = {};
    ImageResolve.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
    ImageResolve.srcSubresource.mipLevel       = 0;
    ImageResolve.srcSubresource.baseArrayLayer = 0;
    ImageResolve.srcSubresource.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageResolve.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
    ImageResolve.dstSubresource.mipLevel       = 0;
    ImageResolve.dstSubresource.baseArrayLayer = 0;
    ImageResolve.dstSubresource.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageResolve.extent.width                  = DstVulkanTexture->GetDesc().Extent.X;
    ImageResolve.extent.height                 = DstVulkanTexture->GetDesc().Extent.Y;
    ImageResolve.extent.depth                  = DstVulkanTexture->GetDesc().Extent.Z;

    TransitionImageLayout(SrcVulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    TransitionImageLayout(DstVulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->ResolveImage(
        SrcVulkanTexture->GetVkImage(), 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        DstVulkanTexture->GetVkImage(), 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        1, 
        &ImageResolve);
}

void FVulkanCommandContext::TranscodeSamplerFeedback(FRHITexture* /* Dst */, uint32 /* DstSubresource */, FRHITexture* /* Src */, uint32 /* SrcSubresource */, ESamplerFeedbackTranscodeMode /* Mode */)
{
    VULKAN_ERROR("TranscodeSamplerFeedback: sampler feedback is not supported by the Vulkan backend");
}

void FVulkanCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    FVulkanBufferRHI* SrcVulkanBuffer = FVulkanDeviceRHI::ResourceCast(Src);
    CHECK(SrcVulkanBuffer != nullptr);
    
    FVulkanBufferRHI* DstVulkanBuffer = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(DstVulkanBuffer != nullptr);

    ConditionalSplitCommandBuffer();

    VkBufferCopy BufferCopy = {};
    BufferCopy.srcOffset = SrcVulkanBuffer->GetBindOffset() + CopyDesc.SrcOffset;
    BufferCopy.dstOffset = DstVulkanBuffer->GetBindOffset() + CopyDesc.DstOffset;
    BufferCopy.size      = CopyDesc.Size;

    RequireBufferState(SrcVulkanBuffer, ERHIResourceState::CopySource);
    RequireBufferState(DstVulkanBuffer, ERHIResourceState::CopyDest);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyBuffer(
        SrcVulkanBuffer->GetBindVkBuffer(), 
        DstVulkanBuffer->GetBindVkBuffer(), 
        1, 
        &BufferCopy);
}

void FVulkanCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FVulkanTextureRHI* SrcVulkanTexture = FVulkanDeviceRHI::ResourceCast(Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTextureRHI* DstVulkanTexture = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(DstVulkanTexture != nullptr);
    
    CHECK(SrcVulkanTexture->GetDesc().Extent.X     == DstVulkanTexture->GetDesc().Extent.X);
    CHECK(SrcVulkanTexture->GetDesc().Extent.Y     == DstVulkanTexture->GetDesc().Extent.Y);
    CHECK(SrcVulkanTexture->GetDesc().Extent.Z     == DstVulkanTexture->GetDesc().Extent.Z);
    CHECK(SrcVulkanTexture->GetDesc().NumMipLevels == DstVulkanTexture->GetDesc().NumMipLevels);
    CHECK(SrcVulkanTexture->GetDesc().Dimension    == DstVulkanTexture->GetDesc().Dimension);

    ConditionalSplitCommandBuffer();

    constexpr uint32 MaxCopies = 15;
    VkImageCopy ImageCopies[MaxCopies];
    
    const FRHITextureDesc TextureDesc = DstVulkanTexture->GetDesc();
    for (uint32 MipLevel = 0; MipLevel < TextureDesc.NumMipLevels; MipLevel++)
    {
        VkImageCopy& ImageCopy = ImageCopies[MipLevel];
        Memory::Memzero(&ImageCopy, sizeof(ImageCopy));
    
        ImageCopy.extent.width                  = Math::Max<uint32>(TextureDesc.Extent.X >> MipLevel, 1u);
        ImageCopy.extent.height                 = Math::Max<uint32>(TextureDesc.Extent.Y >> MipLevel, 1u);
        ImageCopy.extent.depth                  = Math::Max<uint32>(TextureDesc.Extent.Z >> MipLevel, 1u);
        ImageCopy.srcSubresource.aspectMask     = GetImageAspectFlagsFromFormat(SrcVulkanTexture->GetVkFormat());
        ImageCopy.srcSubresource.mipLevel       = MipLevel;
        ImageCopy.srcSubresource.baseArrayLayer = 0;
        ImageCopy.dstSubresource.aspectMask     = GetImageAspectFlagsFromFormat(DstVulkanTexture->GetVkFormat());
        ImageCopy.dstSubresource.mipLevel       = MipLevel;
        ImageCopy.dstSubresource.baseArrayLayer = 0;

        // NOTE: We want to copy the full function
        const ETextureDimension SrcDimension = SrcVulkanTexture->GetDesc().Dimension;
        const ETextureDimension DstDimension = DstVulkanTexture->GetDesc().Dimension;
        ImageCopy.srcSubresource.layerCount = RHIDimensionArrayLayers(SrcDimension, SrcVulkanTexture->GetDesc().NumArraySlices);
        ImageCopy.dstSubresource.layerCount = RHIDimensionArrayLayers(DstDimension, DstVulkanTexture->GetDesc().NumArraySlices);
    }

    TransitionImageLayout(SrcVulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    TransitionImageLayout(DstVulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyImage(
        SrcVulkanTexture->GetVkImage(), 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        DstVulkanTexture->GetVkImage(), 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        TextureDesc.NumMipLevels, 
        ImageCopies);
}

void FVulkanCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
    FVulkanTextureRHI* SrcVulkanTexture = FVulkanDeviceRHI::ResourceCast(Src);
    CHECK(SrcVulkanTexture != nullptr);
    
    FVulkanTextureRHI* DstVulkanTexture = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(DstVulkanTexture != nullptr);

    {
        const ETextureDimension SrcDimension = Src->GetDesc().Dimension;
        const ETextureDimension DstDimension = Dst->GetDesc().Dimension;
        MAYBE_UNUSED const uint32 SrcNumArrayLayers = RHIDimensionArrayLayers(SrcDimension, Src->GetDesc().NumArraySlices);
        MAYBE_UNUSED const uint32 DstNumArrayLayers = RHIDimensionArrayLayers(DstDimension, Dst->GetDesc().NumArraySlices);
        CHECK(CopyDesc.SrcArraySlice + CopyDesc.NumArraySlices <= SrcNumArrayLayers);
        CHECK(CopyDesc.DstArraySlice + CopyDesc.NumArraySlices <= DstNumArrayLayers);
    }

    ConditionalSplitCommandBuffer();

    constexpr uint32 MaxCopies = 15;
    VkImageCopy ImageCopy[MaxCopies];
    
    const uint32 SrcBaseArrayLayer = CopyDesc.SrcArraySlice;
    const uint32 DstBaseArrayLayer = CopyDesc.DstArraySlice;
    const uint32 NumArrayLayers    = CopyDesc.NumArraySlices;

    TransitionImageLayout(SrcVulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    TransitionImageLayout(DstVulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Flush barriers
    BarrierBatcher.FlushBarriers(GetCommandBuffer());
    
    // We copy each layer separately due to MoltenVK seems to be acting weird when doing all layers separately
    for (uint32 ArrayLayer = 0; ArrayLayer < NumArrayLayers; ArrayLayer++)
    {
        for (uint32 MipLevel = 0; MipLevel < CopyDesc.NumMipLevels; MipLevel++)
        {
            VkImageCopy& CopyInfo = ImageCopy[MipLevel];
            Memory::Memzero(&CopyInfo, sizeof(CopyInfo));
            
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
        
        GetCommandBuffer()->CopyImage(
            SrcVulkanTexture->GetVkImage(), 
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
            DstVulkanTexture->GetVkImage(), 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            CopyDesc.NumMipLevels, 
            ImageCopy);
    }
}

void FVulkanCommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FVulkanTextureRHI* SrcVulkanTexture = FVulkanDeviceRHI::ResourceCast(Src);
    CHECK(SrcVulkanTexture != nullptr);

    FVulkanBufferRHI* DstVulkanBuffer = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(DstVulkanBuffer != nullptr);

    ConditionalSplitCommandBuffer();

    TransitionImageLayout(SrcVulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    RequireBufferState(DstVulkanBuffer, ERHIResourceState::CopyDest);
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

    GetCommandBuffer()->CopyImageToBuffer(
        SrcVulkanTexture->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        DstVulkanBuffer->GetBindVkBuffer(),
        1,
        &Copy);
}

void FVulkanCommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    {
        const ETextureDimension SrcDimension = Src->GetDesc().Dimension;
        MAYBE_UNUSED const uint32 SrcNumArrayLayers = RHIDimensionArrayLayers(SrcDimension, Src->GetDesc().NumArraySlices);
        CHECK(SrcArraySlice < SrcNumArrayLayers);
    }

    FVulkanTextureRHI* SrcVulkanTexture = FVulkanDeviceRHI::ResourceCast(Src);
    CHECK(SrcVulkanTexture != nullptr);

    FVulkanBufferRHI* DstVulkanBuffer = FVulkanDeviceRHI::ResourceCast(Dst);
    CHECK(DstVulkanBuffer != nullptr);

    ConditionalSplitCommandBuffer();

    TransitionImageLayout(SrcVulkanTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    RequireBufferState(DstVulkanBuffer, ERHIResourceState::CopyDest);
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

    GetCommandBuffer()->CopyImageToBuffer(
        SrcVulkanTexture->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        DstVulkanBuffer->GetBindVkBuffer(),
        1,
        &Copy);
}

void FVulkanCommandContext::WriteFence(FRHIFence* Fence)
{
    CHECK(Fence != nullptr);
    FVulkanFenceRHI* VulkanFence = FVulkanDeviceRHI::ResourceCast(Fence);

    if (!CommandBuffer)
    {
        ObtainCommandBuffer();
    }

    if (CommandBuffer && CommandBuffer->GetNumCommands() > 0)
    {
        if (VulkanFence->UsesTimeline())
        {
            VulkanFence->EnqueueSignal(GetCommands());
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
    FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(Resource);
    if (!VulkanTexture)
    {
        return;
    }

    ConditionalSplitCommandBuffer();

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

    VkImageMemoryBarrier2KHR ImageBarrier = {};
    ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
    ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageBarrier.newLayout                       = CurrentLayout;
    ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.image                           = VulkanTexture->GetVkImage();
    ImageBarrier.srcAccessMask                   = 0;
    ImageBarrier.dstAccessMask                   = VK_ACCESS_2_MEMORY_WRITE_BIT_KHR | VK_ACCESS_2_MEMORY_READ_BIT_KHR;
    ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR;
    ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
    ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(CreateInfo.format);
    ImageBarrier.subresourceRange.baseArrayLayer = 0;
    ImageBarrier.subresourceRange.baseMipLevel   = 0;
    ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);

    LocalState.SetImageLayout(CurrentLayout);
}

void FVulkanCommandContext::BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* InRayTracingScene, const FRHISceneAccelerationStructureBuildDesc& InBuildDesc)
{
    FVulkanSceneAccelerationStructureRHI* VulkanScene = FVulkanDeviceRHI::ResourceCast(InRayTracingScene);
    if (!VulkanScene)
    {
        return;
    }

    ConditionalSplitCommandBuffer();

    if (!VulkanScene->Build(*this, InBuildDesc))
    {
        VULKAN_ERROR("Failed to build scene acceleration-structure");
    }
}

void FVulkanCommandContext::BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* InRayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& InBuildDesc)
{
    FVulkanGeometryAccelerationStructureRHI* VulkanGeometry = FVulkanDeviceRHI::ResourceCast(InRayTracingGeometry);
    if (!VulkanGeometry)
    {
        return;
    }

    ConditionalSplitCommandBuffer();

    if (!VulkanGeometry->Build(*this, InBuildDesc))
    {
        VULKAN_ERROR("Failed to build geometry acceleration-structure");
    }
}

void FVulkanCommandContext::CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode)
{
#if VK_KHR_ray_tracing_pipeline
    if (!Destination || !Source || !vkCmdCopyAccelerationStructureKHR)
    {
        return;
    }

    VkCopyAccelerationStructureModeKHR VulkanCopyMode;
    switch (CopyMode)
    {
        case EAccelerationStructureCopyMode::Clone:
        {
            VulkanCopyMode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;
            break;
        }

        case EAccelerationStructureCopyMode::Compact:
        {
            VulkanCopyMode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
            break;
        }

        default:
        {
            VULKAN_WARNING("CopyAccelerationStructure: only Clone/Compact are supported through the AS->AS path on Vulkan");
            return;
        }
    }

    ConditionalSplitCommandBuffer();

    VkCopyAccelerationStructureInfoKHR CopyInfo = {};
    CopyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
    CopyInfo.src   = FVulkanDeviceRHI::ResourceCast(Source)->GetVkAccelerationStructure();
    CopyInfo.dst   = FVulkanDeviceRHI::ResourceCast(Destination)->GetVkAccelerationStructure();
    CopyInfo.mode  = VulkanCopyMode;

    AddAccelerationStructureMemoryBarrier();
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyAccelerationStructure(&CopyInfo);
#else
    UNREFERENCED_VARIABLE(Destination);
    UNREFERENCED_VARIABLE(Source);
    UNREFERENCED_VARIABLE(CopyMode);
#endif
}

void FVulkanCommandContext::CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes)
{
#if VK_KHR_acceleration_structure
    if (!AccelerationStructure || CompactedSizeInBytes == 0)
    {
        return;
    }

    if (AccelerationStructure->GetAccelerationStructureType() == ERayTracingAccelerationStructureType::Geometry)
    {
        FVulkanGeometryAccelerationStructureRHI* Geometry = FVulkanDeviceRHI::ResourceCast(static_cast<FRHIGeometryAccelerationStructure*>(AccelerationStructure));

        ConditionalSplitCommandBuffer();
        Geometry->CompactInPlace(*this, CompactedSizeInBytes);
    }
    else
    {
        VULKAN_WARNING("CompactAccelerationStructure: only geometry (BLAS) compaction is supported on Vulkan");
    }
#else
    UNREFERENCED_VARIABLE(AccelerationStructure);
    UNREFERENCED_VARIABLE(CompactedSizeInBytes);
#endif
}

void FVulkanCommandContext::SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Source, FRHIBuffer* DstBuffer, uint64 DstOffset)
{
#if VK_KHR_acceleration_structure
    FVulkanBufferRHI* VulkanDestination = FVulkanDeviceRHI::ResourceCast(DstBuffer);
    if (!VulkanDestination || !Source || !vkCmdCopyAccelerationStructureToMemoryKHR)
    {
        VULKAN_WARNING("SerializeAccelerationStructure: missing destination/source or vkCmdCopyAccelerationStructureToMemoryKHR not loaded");
        return;
    }

    ConditionalSplitCommandBuffer();

    VkCopyAccelerationStructureToMemoryInfoKHR CopyInfo = {};
    CopyInfo.sType              = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
    CopyInfo.src                = FVulkanDeviceRHI::ResourceCast(Source)->GetVkAccelerationStructure();
    CopyInfo.dst.deviceAddress  = VulkanDestination->GetDeviceAddress() + DstOffset;
    CopyInfo.mode               = VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR;

    RequireBufferState(VulkanDestination, ERHIResourceState::CopyDest | ERHIResourceState::RayTracingAccelerationStructure);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyAccelerationStructureToMemory(&CopyInfo);
#else
    UNREFERENCED_VARIABLE(DstBuffer);
    UNREFERENCED_VARIABLE(DstOffset);
    UNREFERENCED_VARIABLE(Source);
#endif
}

void FVulkanCommandContext::DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIBuffer* SourceBuffer, uint64 SourceOffset)
{
#if VK_KHR_acceleration_structure
    FVulkanBufferRHI* VulkanSource = FVulkanDeviceRHI::ResourceCast(SourceBuffer);
    if (!Destination || !VulkanSource || !vkCmdCopyMemoryToAccelerationStructureKHR)
    {
        VULKAN_WARNING("DeserializeAccelerationStructure: missing destination/source or vkCmdCopyMemoryToAccelerationStructureKHR not loaded");
        return;
    }

    ConditionalSplitCommandBuffer();

    VkCopyMemoryToAccelerationStructureInfoKHR CopyInfo = {};
    CopyInfo.sType             = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
    CopyInfo.src.deviceAddress = VulkanSource->GetDeviceAddress() + SourceOffset;
    CopyInfo.dst               = FVulkanDeviceRHI::ResourceCast(Destination)->GetVkAccelerationStructure();
    CopyInfo.mode              = VK_COPY_ACCELERATION_STRUCTURE_MODE_DESERIALIZE_KHR;

    RequireBufferState(VulkanSource, ERHIResourceState::CopySource | ERHIResourceState::RayTracingAccelerationStructure);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->CopyMemoryToAccelerationStructure(&CopyInfo);
#else
    UNREFERENCED_VARIABLE(Destination);
    UNREFERENCED_VARIABLE(SourceBuffer);
    UNREFERENCED_VARIABLE(SourceOffset);
#endif
}

void FVulkanCommandContext::WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources)
{
#if VK_KHR_acceleration_structure
    FVulkanBufferRHI* VulkanDestination = FVulkanDeviceRHI::ResourceCast(DstBuffer);
    if (!VulkanDestination || !Sources || NumSources == 0 || !vkCmdWriteAccelerationStructuresPropertiesKHR)
    {
        return;
    }

    VkQueryType QueryType;
    switch (InfoType)
    {
        case EAccelerationStructurePostBuildInfoType::CompactedSize:
        {
            QueryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
            break;
        }

        case EAccelerationStructurePostBuildInfoType::Serialization:
        {
            QueryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR;
            break;
        }

        default:
        {
            VULKAN_WARNING("WriteAccelerationStructurePostBuildInfo: unsupported post-build info type on Vulkan");
            return;
        }
    }

    TArray<VkAccelerationStructureKHR> SourceHandles;
    SourceHandles.Reserve(NumSources);

    for (uint32 Index = 0; Index < NumSources; ++Index)
    {
        if (Sources[Index])
        {
            SourceHandles.Add(FVulkanDeviceRHI::ResourceCast(Sources[Index])->GetVkAccelerationStructure());
        }
    }

    if (SourceHandles.IsEmpty())
    {
        return;
    }

    ConditionalSplitCommandBuffer();

    VkQueryPoolCreateInfo QueryPoolCreateInfo = {};
    QueryPoolCreateInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    QueryPoolCreateInfo.queryType  = QueryType;
    QueryPoolCreateInfo.queryCount = static_cast<uint32>(SourceHandles.Size());

    VkQueryPool QueryPool = VK_NULL_HANDLE;
    if (VULKAN_FAILED(vkCreateQueryPool(GetDevice()->GetVkDevice(), &QueryPoolCreateInfo, nullptr, &QueryPool)))
    {
        VULKAN_ERROR("WriteAccelerationStructurePostBuildInfo: failed to create query pool");
        return;
    }

    AddAccelerationStructureMemoryBarrier();
    RequireBufferState(VulkanDestination, ERHIResourceState::CopyDest);
    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    GetCommandBuffer()->ResetQueryPool(QueryPool, 0, static_cast<uint32>(SourceHandles.Size()));
    GetCommandBuffer()->WriteAccelerationStructuresProperties(static_cast<uint32>(SourceHandles.Size()), SourceHandles.Data(), QueryType, QueryPool, 0);
    GetCommandBuffer()->CopyQueryPoolResults(
        QueryPool,
        0,
        static_cast<uint32>(SourceHandles.Size()),
        VulkanDestination->GetVkBuffer(),
        VulkanDestination->GetBindOffset() + DstOffset,
        sizeof(uint64),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    FVulkanDeviceRHI::DeferDeletion(QueryPool);
#else
    UNREFERENCED_VARIABLE(DstBuffer);
    UNREFERENCED_VARIABLE(DstOffset);
    UNREFERENCED_VARIABLE(InfoType);
    UNREFERENCED_VARIABLE(Sources);
    UNREFERENCED_VARIABLE(NumSources);
#endif
}

void FVulkanCommandContext::SetHitRecordLocalShaderBindings(FRHIShaderBindingTable* ShaderBindingTable, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings)
{
    if (FVulkanShaderBindingTable* VulkanShaderBindingTable = FVulkanDeviceRHI::ResourceCast(ShaderBindingTable))
    {
        VulkanShaderBindingTable->SetBindings(RecordKind, RecordIndex, Bindings, NumBindings);
    }
}

void FVulkanCommandContext::BuildShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable)
{
    if (FVulkanShaderBindingTable* VulkanShaderBindingTable = FVulkanDeviceRHI::ResourceCast(ShaderBindingTable))
    {
        VulkanShaderBindingTable->Build();
    }
}

void FVulkanCommandContext::ResetShaderBindingTable(FRHIShaderBindingTable* ShaderBindingTable)
{
    if (FVulkanShaderBindingTable* VulkanShaderBindingTable = FVulkanDeviceRHI::ResourceCast(ShaderBindingTable))
    {
        VulkanShaderBindingTable->ClearTableRecords();
    }
}

void FVulkanCommandContext::BuildOpacityMicromap(FRHIOpacityMicromap* OpacityMicromap, const FRHIOpacityMicromapBuildDesc& BuildDesc)
{
    if (FVulkanOpacityMicromap* VulkanMicromap = FVulkanDeviceRHI::ResourceCast(OpacityMicromap))
    {
        ConditionalSplitCommandBuffer();
        VulkanMicromap->Build(*this, BuildDesc);
    }
}

void FVulkanCommandContext::SetRayTracingPipelineState(FRHIRayTracingPipelineState* PipelineState)
{
    ContextState.SetRayTracingPipelineState(FVulkanDeviceRHI::ResourceCast(PipelineState));
}

void FVulkanCommandContext::DispatchRays(FRHIShaderBindingTable* ShaderBindingTable, uint32 Width, uint32 Height, uint32 Depth)
{
#if VK_KHR_ray_tracing_pipeline
    FVulkanShaderBindingTable* VulkanShaderBindingTable = FVulkanDeviceRHI::ResourceCast(ShaderBindingTable);
    if (!ContextState.GetRayTracingPipelineState() || !VulkanShaderBindingTable || !vkCmdTraceRaysKHR)
    {
        return;
    }

    CHECK(VulkanShaderBindingTable->GetPipeline() == ContextState.GetRayTracingPipelineState());

    ConditionalSplitCommandBuffer();

    ContextState.PrepareRayTracingState();
    ContextState.BindRayTracingState();

    const VkStridedDeviceAddressRegionKHR RayGenRegion   = VulkanShaderBindingTable->GetRayGenRegion();
    const VkStridedDeviceAddressRegionKHR MissRegion     = VulkanShaderBindingTable->GetMissRegion();
    const VkStridedDeviceAddressRegionKHR HitGroupRegion = VulkanShaderBindingTable->GetHitGroupRegion();
    const VkStridedDeviceAddressRegionKHR CallableRegion = VulkanShaderBindingTable->GetCallableRegion();

    GetCommandBuffer()->TraceRays(&RayGenRegion, &MissRegion, &HitGroupRegion, &CallableRegion, Width, Height, Depth);
#else
    UNREFERENCED_VARIABLE(ShaderBindingTable);
    UNREFERENCED_VARIABLE(Width);
    UNREFERENCED_VARIABLE(Height);
    UNREFERENCED_VARIABLE(Depth);
#endif
}

void FVulkanCommandContext::DispatchRaysIndirect(FRHIShaderBindingTable* ShaderBindingTable, FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
#if VK_KHR_ray_tracing_pipeline && VK_KHR_ray_tracing_maintenance1
    FVulkanBufferRHI*          VulkanArgumentBuffer     = FVulkanDeviceRHI::ResourceCast(ArgumentBuffer);
    FVulkanShaderBindingTable* VulkanShaderBindingTable = FVulkanDeviceRHI::ResourceCast(ShaderBindingTable);

    if (!ContextState.GetRayTracingPipelineState() || !VulkanShaderBindingTable || !VulkanArgumentBuffer || !vkCmdTraceRaysIndirect2KHR)
    {
        VULKAN_WARNING("DispatchRaysIndirect: missing pipeline/SBT/args or vkCmdTraceRaysIndirect2KHR not loaded");
        return;
    }

    CHECK(VulkanShaderBindingTable->GetPipeline() == ContextState.GetRayTracingPipelineState());

    ConditionalSplitCommandBuffer();

    RequireBufferState(VulkanArgumentBuffer, ERHIResourceState::IndirectArgument);

    ContextState.PrepareRayTracingState();
    ContextState.BindRayTracingState();

    const VkDeviceAddress IndirectDeviceAddress = VulkanArgumentBuffer->GetDeviceAddress() + ArgumentBufferOffset;
    if (IndirectDeviceAddress == 0 || (IndirectDeviceAddress & 3u) != 0)
    {
        VULKAN_WARNING("DispatchRaysIndirect: indirect device address must be non-zero and 4-byte aligned");
        return;
    }

    GetCommandBuffer()->TraceRaysIndirect2(IndirectDeviceAddress);
#else
    UNREFERENCED_VARIABLE(ShaderBindingTable);
    UNREFERENCED_VARIABLE(ArgumentBuffer);
    UNREFERENCED_VARIABLE(ArgumentBufferOffset);
#endif
}

void FVulkanCommandContext::ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* Operations, uint32 NumOperations)
{
#if VK_NV_cluster_acceleration_structure && VK_NV_partitioned_acceleration_structure
    if (!Operations || NumOperations == 0)
    {
        return;
    }

    ConditionalSplitCommandBuffer();

    BarrierBatcher.FlushBarriers(GetCommandBuffer());

    for (uint32 Index = 0; Index < NumOperations; ++Index)
    {
        const FRHIRayTracingAccelerationStructureOperationDesc& Operation = Operations[Index];

        FVulkanBufferRHI* ArgumentBuffer    = FVulkanDeviceRHI::ResourceCast(Operation.ArgumentBuffer);
        FVulkanBufferRHI* DestinationBuffer = FVulkanDeviceRHI::ResourceCast(Operation.DestinationBuffer);
        FVulkanBufferRHI* ScratchBuffer     = FVulkanDeviceRHI::ResourceCast(Operation.ScratchBuffer);

        const VkDeviceAddress ArgumentAddress    = ArgumentBuffer    ? (ArgumentBuffer->GetDeviceAddress()    + Operation.ArgumentBufferOffset) : 0;
        const VkDeviceAddress DestinationAddress = DestinationBuffer ? (DestinationBuffer->GetDeviceAddress() + Operation.DestinationOffset)    : 0;
        const VkDeviceAddress ScratchAddress     = ScratchBuffer     ? (ScratchBuffer->GetDeviceAddress()     + Operation.ScratchOffset)        : 0;

        if (Operation.OperationType == ERayTracingAccelerationStructureOperationType::PartitionedSceneAccelerationStructure)
        {
            if (!vkCmdBuildPartitionedAccelerationStructuresNV)
            {
                continue;
            }

            VkPartitionedAccelerationStructureInstancesInputNV InstancesInput = {};
            InstancesInput.sType                             = VK_STRUCTURE_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCES_INPUT_NV;
            InstancesInput.instanceCount                     = Operation.ArgumentCount;
            InstancesInput.maxInstancePerPartitionCount      = Operation.ArgumentCount;
            InstancesInput.partitionCount                    = 1;
            InstancesInput.maxInstanceInGlobalPartitionCount = Operation.ArgumentCount;

            VkBuildPartitionedAccelerationStructureInfoNV BuildInfo = {};
            BuildInfo.sType                        = VK_STRUCTURE_TYPE_BUILD_PARTITIONED_ACCELERATION_STRUCTURE_INFO_NV;
            BuildInfo.input                        = InstancesInput;
            BuildInfo.dstAccelerationStructureData = DestinationAddress;
            BuildInfo.scratchData                  = ScratchAddress;
            BuildInfo.srcInfos                     = ArgumentAddress;

            GetCommandBuffer()->BuildPartitionedAccelerationStructures(&BuildInfo);
        }
        else
        {
            if (!vkCmdBuildClusterAccelerationStructureIndirectNV)
            {
                continue;
            }

            FVulkanClusterInputScratch Scratch = {};
            Scratch.TriangleClusters.sType                         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_TRIANGLE_CLUSTER_INPUT_NV;
            Scratch.TriangleClusters.vertexFormat                  = VK_FORMAT_R32G32B32_SFLOAT;
            Scratch.TriangleClusters.maxGeometryIndexValue         = Operation.ClusterLimits.MaxGeometryIndex;
            Scratch.TriangleClusters.maxClusterUniqueGeometryCount = 1;
            Scratch.TriangleClusters.maxClusterTriangleCount       = Operation.ClusterLimits.MaxTrianglesPerCluster;
            Scratch.TriangleClusters.maxClusterVertexCount         = Operation.ClusterLimits.MaxVerticesPerCluster;
            Scratch.TriangleClusters.maxTotalTriangleCount         = Operation.ClusterLimits.MaxTrianglesPerCluster * Operation.ClusterLimits.MaxClusterCount;
            Scratch.TriangleClusters.maxTotalVertexCount           = Operation.ClusterLimits.MaxVerticesPerCluster * Operation.ClusterLimits.MaxClusterCount;
            Scratch.TriangleClusters.minPositionTruncateBitCount   = 0;

            Scratch.ClustersBottomLevel.sType                                   = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_CLUSTERS_BOTTOM_LEVEL_INPUT_NV;
            Scratch.ClustersBottomLevel.maxTotalClusterCount                    = Operation.ClusterLimits.MaxClusterCount;
            Scratch.ClustersBottomLevel.maxClusterCountPerAccelerationStructure = Operation.ClusterLimits.MaxClusterCount;

            Scratch.MoveObjects.sType         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_MOVE_OBJECTS_INPUT_NV;
            Scratch.MoveObjects.type          = VK_CLUSTER_ACCELERATION_STRUCTURE_TYPE_TRIANGLE_CLUSTER_NV;
            Scratch.MoveObjects.noMoveOverlap = VK_FALSE;
            Scratch.MoveObjects.maxMovedBytes = 0;

            VkClusterAccelerationStructureInputInfoNV InputInfo = {};
            InputInfo.sType                         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_INPUT_INFO_NV;
            InputInfo.maxAccelerationStructureCount = Operation.ArgumentCount;
            InputInfo.flags                         = ConvertAccelerationStructureBuildFlags(EAccelerationStructureBuildFlags::None);

            switch (Operation.OperationType)
            {
                case ERayTracingAccelerationStructureOperationType::BuildClusterTemplatesFromTriangles:
                    InputInfo.opType                    = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_TEMPLATE_NV;
                    InputInfo.opInput.pTriangleClusters = &Scratch.TriangleClusters;
                    break;

                case ERayTracingAccelerationStructureOperationType::InstantiateClusterTemplates:
                    InputInfo.opType                    = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_INSTANTIATE_TRIANGLE_CLUSTER_NV;
                    InputInfo.opInput.pTriangleClusters = &Scratch.TriangleClusters;
                    break;

                case ERayTracingAccelerationStructureOperationType::BuildGeometryAccelerationStructureFromClusters:
                    InputInfo.opType                       = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_CLUSTERS_BOTTOM_LEVEL_NV;
                    InputInfo.opInput.pClustersBottomLevel = &Scratch.ClustersBottomLevel;
                    break;

                case ERayTracingAccelerationStructureOperationType::MoveClusterObjects:
                    InputInfo.opType               = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_MOVE_OBJECTS_NV;
                    InputInfo.opInput.pMoveObjects = &Scratch.MoveObjects;
                    break;

                default:
                    InputInfo.opType                    = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_NV;
                    InputInfo.opInput.pTriangleClusters = &Scratch.TriangleClusters;
                    break;
            }

            VkClusterAccelerationStructureCommandsInfoNV CommandsInfo = {};
            CommandsInfo.sType                       = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_COMMANDS_INFO_NV;
            CommandsInfo.input                       = InputInfo;
            CommandsInfo.dstImplicitData             = DestinationAddress;
            CommandsInfo.scratchData                 = ScratchAddress;
            CommandsInfo.srcInfosArray.deviceAddress = ArgumentAddress;
            CommandsInfo.srcInfosArray.stride        = Operation.ArgumentStride;
            CommandsInfo.srcInfosArray.size          = uint64(Operation.ArgumentCount) * Operation.ArgumentStride;

            GetCommandBuffer()->BuildClusterAccelerationStructureIndirect(&CommandsInfo);
        }
    }
#else
    UNREFERENCED_VARIABLE(Operations);
    UNREFERENCED_VARIABLE(NumOperations);
#endif
}

void FVulkanCommandContext::TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs)
{
    for (const FRHITransitionBarrierDesc& Desc : TransitionDescs)
    {
        if (Desc.IsTexture())
        {
            TransitionBarrierTexture(Desc);
        }
        else
        {
            TransitionBarrierBuffer(Desc);
        }
    }
}

void FVulkanCommandContext::TransitionBarrierTexture(const FRHITransitionBarrierDesc& Desc)
{
    FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(Desc.Texture.Resource);
    CHECK(VulkanTexture != nullptr);

    if (Desc.IsSplitBegin())
    {
        return;
    }

    const FRHITextureDesc&             TextureDesc  = Desc.Texture.Resource->GetDesc();
    const FRHITextureSubresourceRange& Subresources = Desc.Texture.Subresources;

    {
        MAYBE_UNUSED const uint32 NumArrayLayers = RHIDimensionArrayLayers(TextureDesc.Dimension, TextureDesc.NumArraySlices);
        CHECK(Subresources.NumArraySlices == RHI_ALL_ARRAY_SLICES || (Subresources.FirstArraySlice + Subresources.NumArraySlices) <= NumArrayLayers);
        CHECK(Subresources.NumMipLevels   == RHI_ALL_MIP_LEVELS   || (Subresources.FirstMipLevel   + Subresources.NumMipLevels)   <= TextureDesc.NumMipLevels);
    }

    const VkImageLayout NewLayout = FVulkanDeviceRHI::ResourceStateToImageLayout(Desc.AfterState);

    const ERHIResourceStateTrackingMode TrackingMode = Desc.Texture.Resource->GetDesc().TrackingMode;
    if (TrackingMode == ERHIResourceStateTrackingMode::Static)
    {
        if (!Desc.IsTrackingModeChange())
        {
            const FVulkanImageLayoutState& GlobalState = VulkanTexture->GetImageLayoutState();
            VULKAN_WARNING_COND(!GlobalState.HasDefaultLayout() || GlobalState.GetDefaultLayout() == NewLayout,
                "Dropping a transition to layout %d on a Static texture that always rests in layout %d",
                int32(NewLayout), int32(GlobalState.GetDefaultLayout()));
        }

        ApplyTrackingModeChange(VulkanTexture, Desc);
        return;
    }

    const VkImageCreateInfo&             CreateInfo = VulkanTexture->GetVkImageCreateInfo();
    const FVulkanBarrierSubresourceRange Range      = VulkanResolveSubresourceRange(CreateInfo, Subresources);

    const bool bInferBeforeState = (Desc.BeforeState == Desc.AfterState);

    const VkImageLayout DeclaredLayout = Desc.IsDiscard()
        ? VK_IMAGE_LAYOUT_UNDEFINED
        : FVulkanDeviceRHI::ResourceStateToImageLayout(Desc.BeforeState);

    VkImageMemoryBarrier2KHR ImageBarrier = {};
    ImageBarrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
    ImageBarrier.newLayout                   = NewLayout;
    ImageBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.image                       = VulkanTexture->GetVkImage();
    ImageBarrier.dstAccessMask               = FVulkanDeviceRHI::ResourceStateToAccessFlags(Desc.AfterState);
    ImageBarrier.dstStageMask                = FVulkanDeviceRHI::ResourceStateToPipelineStageFlags(Desc.AfterState);
    ImageBarrier.subresourceRange.aspectMask = VulkanResolveAspectMask(CreateInfo.format, Subresources);

    // With no declared before-state there is no source scope to narrow to, so wait on everything
    ImageBarrier.srcAccessMask = bInferBeforeState ? VK_ACCESS_2_NONE_KHR : FVulkanDeviceRHI::ResourceStateToAccessFlags(Desc.BeforeState);
    ImageBarrier.srcStageMask  = bInferBeforeState ? VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR : FVulkanDeviceRHI::ResourceStateToPipelineStageFlags(Desc.BeforeState);

    if (TrackingMode == ERHIResourceStateTrackingMode::Manual)
    {
        ImageBarrier.oldLayout                       = DeclaredLayout;
        ImageBarrier.subresourceRange.baseMipLevel   = Range.BaseMip;
        ImageBarrier.subresourceRange.levelCount     = Range.MipCount;
        ImageBarrier.subresourceRange.baseArrayLayer = Range.BaseLayer;
        ImageBarrier.subresourceRange.layerCount     = Range.LayerCount;

        CHECK(!IsInsideRenderPass());
        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);

        ApplyTrackingModeChange(VulkanTexture, Desc);
        return;
    }

    FVulkanImageLayoutState& LocalState = RetrievePendingImageState(VulkanTexture);

    const auto EmitTransition = [&](VkImageLayout CurrentLayout, uint32 Subresource, uint32 BaseMip, uint32 MipCount, uint32 BaseLayer, uint32 LayerCount)
    {
        const bool bFirstTouch = (CurrentLayout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED);
        if (bFirstTouch)
        {
            FVulkanPendingImageBarrier PendingBarrier;
            PendingBarrier.Texture       = VulkanTexture;
            PendingBarrier.DesiredLayout = bInferBeforeState ? NewLayout : DeclaredLayout;
            PendingBarrier.Subresource   = Subresource;
            PendingImageBarriers.Add(PendingBarrier);

            if (bInferBeforeState)
            {
                return;
            }
        }

        const VkImageLayout OldLayout = bFirstTouch ? DeclaredLayout : CurrentLayout;
        if (OldLayout == NewLayout)
        {
            return;
        }

        ImageBarrier.oldLayout                       = OldLayout;
        ImageBarrier.subresourceRange.baseMipLevel   = BaseMip;
        ImageBarrier.subresourceRange.levelCount     = MipCount;
        ImageBarrier.subresourceRange.baseArrayLayer = BaseLayer;
        ImageBarrier.subresourceRange.layerCount     = LayerCount;

        CHECK(!IsInsideRenderPass());
        BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
    };

    if (Range.bWholeResource && LocalState.AreAllSubresourcesSameLayout())
    {
        EmitTransition(LocalState.GetImageLayout(), RHI_ALL_MIP_LEVELS, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS);
        LocalState.SetImageLayout(NewLayout);
    }
    else
    {
        for (uint32 Layer = Range.BaseLayer; Layer < Range.BaseLayer + Range.LayerCount; Layer++)
        {
            for (uint32 Mip = Range.BaseMip; Mip < Range.BaseMip + Range.MipCount; Mip++)
            {
                const uint32 SubresourceIndex = Layer * CreateInfo.mipLevels + Mip;
                EmitTransition(LocalState.GetSubresourceLayout(SubresourceIndex), SubresourceIndex, Mip, 1, Layer, 1);
                LocalState.SetSubresourceLayout(SubresourceIndex, NewLayout);
            }
        }
    }

    ApplyTrackingModeChange(VulkanTexture, Desc);
}

void FVulkanCommandContext::ApplyTrackingModeChange(FVulkanTextureRHI* Texture, const FRHITransitionBarrierDesc& Desc)
{
    if (!Desc.IsTrackingModeChange())
    {
        return;
    }

    const VkImageLayout      NewLayout   = FVulkanDeviceRHI::ResourceStateToImageLayout(Desc.AfterState);
    FVulkanImageLayoutState& GlobalState = Texture->GetImageLayoutState();

    switch (Desc.NewTrackingMode)
    {
    case ERHIResourceStateTrackingMode::Static:
        GlobalState.SetDefaultLayout(NewLayout);
        break;

    case ERHIResourceStateTrackingMode::Tracked:
        GlobalState.ClearDefaultLayout();
        RetrievePendingImageState(Texture).SetImageLayout(NewLayout);
        break;

    case ERHIResourceStateTrackingMode::Manual:
        GlobalState.ClearDefaultLayout();
        break;
    }

    Texture->SetResourceStateTrackingMode(Desc.NewTrackingMode);
}

void FVulkanCommandContext::TransitionBarrierBuffer(const FRHITransitionBarrierDesc& Desc)
{
    FVulkanBufferRHI* VulkanBuffer = FVulkanDeviceRHI::ResourceCast(Desc.Buffer.Resource);
    CHECK(VulkanBuffer != nullptr);

    if (Desc.IsSplitBegin())
    {
        return;
    }

    const VkAccessFlags2KHR        AfterAccess = FVulkanDeviceRHI::ResourceStateToAccessFlags(Desc.AfterState);
    const VkPipelineStageFlags2KHR AfterStage  = FVulkanDeviceRHI::ResourceStateToPipelineStageFlags(Desc.AfterState);

    const ERHIResourceStateTrackingMode TrackingMode = Desc.Buffer.Resource->GetDesc().TrackingMode;
    if (TrackingMode == ERHIResourceStateTrackingMode::Static)
    {
        const FVulkanBufferState& GlobalState = VulkanBuffer->GetBufferState();
        VULKAN_WARNING_COND(!GlobalState.HasDefaultState() || (AfterAccess & ~GlobalState.GetDefaultAccess()) == 0,
            "Dropping a transition to access 0x%llx on a Static buffer that always rests in access 0x%llx",
            uint64(AfterAccess), uint64(GlobalState.GetDefaultAccess()));

        return;
    }

    const FBufferRegion& Range = Desc.Buffer.Range;
    CHECK(Range.Size == RHI_WHOLE_SIZE || (Range.Offset + Range.Size) <= Desc.Buffer.Resource->GetDesc().Size);

    VkBufferMemoryBarrier2KHR BufferBarrier = {};
    BufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
    BufferBarrier.dstAccessMask       = AfterAccess;
    BufferBarrier.dstStageMask        = AfterStage;
    BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufferBarrier.buffer              = VulkanBuffer->GetVkBuffer();
    BufferBarrier.offset              = Range.Offset;
    BufferBarrier.size                = (Range.Size == RHI_WHOLE_SIZE) ? VK_WHOLE_SIZE : Range.Size;

    if (TrackingMode == ERHIResourceStateTrackingMode::Manual)
    {
        BufferBarrier.srcAccessMask = FVulkanDeviceRHI::ResourceStateToAccessFlags(Desc.BeforeState);
        BufferBarrier.srcStageMask  = FVulkanDeviceRHI::ResourceStateToPipelineStageFlags(Desc.BeforeState);

        CHECK(!IsInsideRenderPass());
        BarrierBatcher.AddBufferMemoryBarrier(0, BufferBarrier);
        return;
    }

    const bool bInferBeforeState = (Desc.BeforeState == Desc.AfterState);

    FVulkanBufferState& LocalState = RetrievePendingBufferState(VulkanBuffer);
    const bool          bFirstTouch = (LocalState.GetAccess() == VK_ACCESS_FLAGS_2_TO_BE_DETERMINED);

    const VkAccessFlags2KHR        DeclaredAccess = bInferBeforeState ? AfterAccess : FVulkanDeviceRHI::ResourceStateToAccessFlags(Desc.BeforeState);
    const VkPipelineStageFlags2KHR DeclaredStage  = bInferBeforeState ? AfterStage  : FVulkanDeviceRHI::ResourceStateToPipelineStageFlags(Desc.BeforeState);

    if (bFirstTouch)
    {
        FVulkanPendingBufferBarrier PendingBarrier;
        PendingBarrier.Buffer        = VulkanBuffer;
        PendingBarrier.DesiredAccess = DeclaredAccess;
        PendingBarrier.DesiredStage  = DeclaredStage;
        PendingBufferBarriers.Add(PendingBarrier);
    }

    const bool bNeedsBarrier = bFirstTouch
        ? !bInferBeforeState
        : (LocalState.GetAccess() != AfterAccess || LocalState.GetStage() != AfterStage);

    if (bNeedsBarrier)
    {
        BufferBarrier.srcAccessMask = bFirstTouch ? DeclaredAccess : LocalState.GetAccess();
        BufferBarrier.srcStageMask  = bFirstTouch ? DeclaredStage  : LocalState.GetStage();

        CHECK(!IsInsideRenderPass());
        BarrierBatcher.AddBufferMemoryBarrier(0, BufferBarrier);
    }

    LocalState.SetState(AfterAccess, AfterStage);
}

void FVulkanCommandContext::RequireBufferState(FVulkanBufferRHI* VulkanBuffer, ERHIResourceState RequiredState)
{
    CHECK(VulkanBuffer != nullptr);

    if (VulkanBuffer->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Tracked)
    {
        return;
    }

    FVulkanBufferState& LocalState = RetrievePendingBufferState(VulkanBuffer);
    
    const VkAccessFlags2KHR        DesiredAccess = FVulkanDeviceRHI::ResourceStateToAccessFlags(RequiredState);
    const VkPipelineStageFlags2KHR DesiredStage  = FVulkanDeviceRHI::ResourceStateToPipelineStageFlags(RequiredState);

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
        VkBufferMemoryBarrier2KHR BufferBarrier = {};
        BufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
        BufferBarrier.srcAccessMask       = LocalState.GetAccess();
        BufferBarrier.dstAccessMask       = DesiredAccess;
        BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        BufferBarrier.srcStageMask        = LocalState.GetStage();
        BufferBarrier.dstStageMask        = DesiredStage;
        BufferBarrier.buffer              = VulkanBuffer->GetVkBuffer();
        BufferBarrier.offset              = 0;
        BufferBarrier.size                = VK_WHOLE_SIZE;

        BarrierBatcher.AddBufferMemoryBarrier(0, BufferBarrier);
        LocalState.SetState(DesiredAccess, DesiredStage);
    }
}

void FVulkanCommandContext::TransitionImageLayout(FVulkanTextureRHI* Texture, VkImageLayout AfterLayout)
{
    CHECK(Texture != nullptr);

    if (Texture->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Tracked)
    {
        return;
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

            VkImageMemoryBarrier2KHR ImageBarrier = {};
            ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
            ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE_KHR;
            ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
            ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
            ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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
            VkImageMemoryBarrier2KHR ImageBarrier = {};
            ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
            ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
            ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
            ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
            ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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

                VkImageMemoryBarrier2KHR ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE_KHR;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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
                VkImageMemoryBarrier2KHR ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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

void FVulkanCommandContext::TransitionImageLayout(FVulkanTextureRHI* Texture, VkImageLayout BeforeLayout, VkImageLayout AfterLayout)
{
    CHECK(Texture != nullptr);

    if (Texture->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Tracked)
    {
        return;
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

            VkImageMemoryBarrier2KHR ImageBarrier = {};
            ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
            ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE_KHR;
            ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
            ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
            ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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

            VkImageMemoryBarrier2KHR ImageBarrier = {};
            ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
            ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
            ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
            ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
            ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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

                VkImageMemoryBarrier2KHR ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE_KHR;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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

                VkImageMemoryBarrier2KHR ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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

void FVulkanCommandContext::TransitionImageLayout(FVulkanTextureRHI* Texture, VkImageLayout AfterLayout, uint32 FirstMip, uint32 NumMips, uint32 FirstArraySlice, uint32 NumArraySlices)
{
    CHECK(Texture != nullptr);

    if (Texture->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Tracked)
    {
        return;
    }

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

                VkImageMemoryBarrier2KHR ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_NONE_KHR;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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
                VkImageMemoryBarrier2KHR ImageBarrier = {};
                ImageBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                ImageBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.dstAccessMask       = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
                ImageBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                ImageBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
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

void FVulkanCommandContext::TransitionImageLayout(FVulkanUnorderedAccessViewRHI* View)
{
    CHECK(View != nullptr);

    if (View->GetType() != FVulkanResourceView::EType::ImageView)
    {
        return;
    }

    FVulkanTextureRHI* Texture = FVulkanDeviceRHI::ResourceCast(static_cast<FRHITexture*>(View->GetResource()));
    if (!Texture)
    {
        return;
    }

    const VkImageSubresourceRange& Range =  View->GetImageViewInfo().SubresourceRange;
    TransitionImageLayout(Texture, VK_IMAGE_LAYOUT_GENERAL, Range.baseMipLevel, Range.levelCount, Range.baseArrayLayer, Range.layerCount);
}

void FVulkanCommandContext::AddAccelerationStructureMemoryBarrier()
{
    VkMemoryBarrier2KHR AccelerationStructureBarrier = {};
    AccelerationStructureBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR;
    AccelerationStructureBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_HOST_BIT_KHR | VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR | VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    AccelerationStructureBarrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT_KHR | VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    AccelerationStructureBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
    AccelerationStructureBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR | VK_ACCESS_2_TRANSFER_READ_BIT_KHR | VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

    BarrierBatcher.AddMemoryBarrier(0, AccelerationStructureBarrier);
}

void FVulkanCommandContext::RequireBufferState(FVulkanUnorderedAccessViewRHI* View, ERHIResourceState RequiredState)
{
    CHECK(View != nullptr);

    if (View->GetType() == FVulkanResourceView::EType::ImageView)
    {
        return;
    }

    if (FVulkanBufferRHI* Buffer = FVulkanDeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(View->GetResource())))
    {
        RequireBufferState(Buffer, RequiredState);
    }
}

void FVulkanCommandContext::TransitionImageLayout(FVulkanShaderResourceViewRHI* View, VkImageLayout Layout)
{
    CHECK(View != nullptr);

    if (View->GetType() != FVulkanResourceView::EType::ImageView)
    {
        return;
    }

    FVulkanTextureRHI* Texture = FVulkanDeviceRHI::ResourceCast(static_cast<FRHITexture*>(View->GetResource()));
    if (!Texture)
    {
        return;
    }

    const VkImageSubresourceRange& Range = View->GetImageViewInfo().SubresourceRange;
    TransitionImageLayout(Texture, Layout, Range.baseMipLevel, Range.levelCount, Range.baseArrayLayer, Range.layerCount);
}

void FVulkanCommandContext::UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs)
{
    CHECK(!IsInsideRenderPass());

    for (const FRHIUnorderedAccessBarrierDesc& Desc : BarrierDescs)
    {
        if (Desc.IsTexture())
        {
            FVulkanTextureRHI* VulkanTexture = FVulkanDeviceRHI::ResourceCast(Desc.Texture.Resource);
            CHECK(VulkanTexture != nullptr);

            const VkImageCreateInfo&             CreateInfo = VulkanTexture->GetVkImageCreateInfo();
            const FVulkanBarrierSubresourceRange Range      = VulkanResolveSubresourceRange(CreateInfo, Desc.Texture.Subresources);

            VkImageMemoryBarrier2KHR ImageBarrier = {};
            ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
            ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
            ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
            ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            ImageBarrier.image                           = VulkanTexture->GetVkImage();
            ImageBarrier.srcAccessMask                   = VK_ACCESS_2_SHADER_READ_BIT_KHR | VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
            ImageBarrier.dstAccessMask                   = VK_ACCESS_2_SHADER_READ_BIT_KHR | VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
            ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
            ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
            ImageBarrier.subresourceRange.aspectMask     = VulkanResolveAspectMask(CreateInfo.format, Desc.Texture.Subresources);
            ImageBarrier.subresourceRange.baseMipLevel   = Range.BaseMip;
            ImageBarrier.subresourceRange.levelCount     = Range.MipCount;
            ImageBarrier.subresourceRange.baseArrayLayer = Range.BaseLayer;
            ImageBarrier.subresourceRange.layerCount     = Range.LayerCount;

            BarrierBatcher.AddImageMemoryBarrier(0, ImageBarrier);
        }
        else
        {
            FVulkanBufferRHI* VulkanBuffer = FVulkanDeviceRHI::ResourceCast(Desc.Buffer.Resource);
            CHECK(VulkanBuffer != nullptr);

            const FBufferRegion& Range = Desc.Buffer.Range;
            CHECK(Range.Size == RHI_WHOLE_SIZE || (Range.Offset + Range.Size) <= Desc.Buffer.Resource->GetDesc().Size);

            VkBufferMemoryBarrier2KHR BufferBarrier = {};
            BufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
            BufferBarrier.srcAccessMask       = VK_ACCESS_2_SHADER_READ_BIT_KHR | VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
            BufferBarrier.dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT_KHR | VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
            BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            BufferBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
            BufferBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
            BufferBarrier.buffer              = VulkanBuffer->GetVkBuffer();
            BufferBarrier.offset              = Range.Offset;
            BufferBarrier.size                = (Range.Size == RHI_WHOLE_SIZE) ? VK_WHOLE_SIZE : Range.Size;

            BarrierBatcher.AddBufferMemoryBarrier(0, BufferBarrier);
        }
    }
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
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "Draw");
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
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "DrawIndexed");
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
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "DrawInstanced");
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
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "DrawIndexedInstanced");
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
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "Dispatch");
    }
#endif
}

void FVulkanCommandContext::DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
#if VK_EXT_mesh_shader
    if (!GVulkanSupportsMeshShaders)
    {
        VULKAN_WARNING("DispatchMesh called but mesh shaders are not supported on this device");
        return;
    }

    if (ThreadGroupCountX == 0 || ThreadGroupCountY == 0 || ThreadGroupCountZ == 0)
    {
        return;
    }

    ConditionalSplitCommandBuffer();
    ContextState.PrepareMeshletState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindMeshletState();
    GetCommandBuffer()->DrawMeshTasks(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 2)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteDrawMarker(GetCommandBuffer(), "DispatchMesh");
    }
#endif
#else
    UNREFERENCED_VARIABLE(ThreadGroupCountX);
    UNREFERENCED_VARIABLE(ThreadGroupCountY);
    UNREFERENCED_VARIABLE(ThreadGroupCountZ);
    VULKAN_WARNING("DispatchMesh called but VK_EXT_mesh_shader is not available in this build");
#endif // VK_EXT_mesh_shader
}

void FVulkanCommandContext::DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    FVulkanBufferRHI* Arguments = FVulkanDeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    ConditionalSplitCommandBuffer();

    RequireBufferState(Arguments, ERHIResourceState::IndirectArgument);
    ContextState.PrepareGraphicsState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindGraphicsState();

    GetCommandBuffer()->DrawIndirect(
        Arguments->GetBindVkBuffer(),
        Arguments->GetBindOffset() + ArgumentBufferOffset,
        CommandCount,
        sizeof(FRHIDrawIndirectParameters));
}

void FVulkanCommandContext::DrawIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
    FVulkanBufferRHI* Arguments = FVulkanDeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    FVulkanBufferRHI* Count = FVulkanDeviceRHI::ResourceCast(CountBuffer);
    CHECK(Count != nullptr);

    ConditionalSplitCommandBuffer();

    RequireBufferState(Arguments, ERHIResourceState::IndirectArgument);
    RequireBufferState(Count, ERHIResourceState::IndirectArgument);
    ContextState.PrepareGraphicsState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindGraphicsState();

    GetCommandBuffer()->DrawIndirectCount(
        Arguments->GetBindVkBuffer(),
        Arguments->GetBindOffset() + ArgumentBufferOffset,
        Count->GetBindVkBuffer(),
        Count->GetBindOffset() + CountBufferOffset,
        MaxCommandCount,
        sizeof(FRHIDrawIndirectParameters));
}

void FVulkanCommandContext::DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    FVulkanBufferRHI* Arguments = FVulkanDeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    ConditionalSplitCommandBuffer();

    RequireBufferState(Arguments, ERHIResourceState::IndirectArgument);
    ContextState.PrepareGraphicsState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindGraphicsState();

    GetCommandBuffer()->DrawIndexedIndirect(
        Arguments->GetBindVkBuffer(),
        Arguments->GetBindOffset() + ArgumentBufferOffset,
        CommandCount,
        sizeof(FRHIDrawIndexedIndirectParameters));
}

void FVulkanCommandContext::DrawIndexedIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
    FVulkanBufferRHI* Arguments = FVulkanDeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    FVulkanBufferRHI* Count = FVulkanDeviceRHI::ResourceCast(CountBuffer);
    CHECK(Count != nullptr);

    ConditionalSplitCommandBuffer();

    RequireBufferState(Arguments, ERHIResourceState::IndirectArgument);
    RequireBufferState(Count, ERHIResourceState::IndirectArgument);
    ContextState.PrepareGraphicsState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindGraphicsState();

    GetCommandBuffer()->DrawIndexedIndirectCount(
        Arguments->GetBindVkBuffer(),
        Arguments->GetBindOffset() + ArgumentBufferOffset,
        Count->GetBindVkBuffer(),
        Count->GetBindOffset() + CountBufferOffset,
        MaxCommandCount,
        sizeof(FRHIDrawIndexedIndirectParameters));
}

void FVulkanCommandContext::DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
    FVulkanBufferRHI* Arguments = FVulkanDeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    ConditionalSplitCommandBuffer();

    RequireBufferState(Arguments, ERHIResourceState::IndirectArgument);

    ContextState.PrepareComputeState();
    ContextState.BindComputeState();

    GetCommandBuffer()->DispatchIndirect(Arguments->GetBindVkBuffer(), Arguments->GetBindOffset() + ArgumentBufferOffset);
}

void FVulkanCommandContext::DispatchMeshIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
#if VK_EXT_mesh_shader
    if (!GVulkanSupportsMeshShaders || !vkCmdDrawMeshTasksIndirectEXT)
    {
        VULKAN_WARNING("DispatchMeshIndirect called but indirect mesh shaders are not supported on this device");
        return;
    }

    FVulkanBufferRHI* Arguments = FVulkanDeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    ConditionalSplitCommandBuffer();

    RequireBufferState(Arguments, ERHIResourceState::IndirectArgument);
    ContextState.PrepareMeshletState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindMeshletState();

    GetCommandBuffer()->DrawMeshTasksIndirect(
        Arguments->GetBindVkBuffer(),
        Arguments->GetBindOffset() + ArgumentBufferOffset,
        CommandCount,
        sizeof(FRHIDispatchMeshIndirectParameters));
#else
    UNREFERENCED_VARIABLE(ArgumentBuffer);
    UNREFERENCED_VARIABLE(ArgumentBufferOffset);
    UNREFERENCED_VARIABLE(CommandCount);
#endif
}

void FVulkanCommandContext::DispatchMeshIndirectCount(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, FRHIBuffer* CountBuffer, uint64 CountBufferOffset, uint32 MaxCommandCount)
{
#if VK_EXT_mesh_shader
    if (!GVulkanSupportsMeshShaders || !vkCmdDrawMeshTasksIndirectCountEXT)
    {
        VULKAN_WARNING("DispatchMeshIndirectCount called but count-buffer mesh shaders are not supported on this device");
        return;
    }

    FVulkanBufferRHI* Arguments = FVulkanDeviceRHI::ResourceCast(ArgumentBuffer);
    CHECK(Arguments != nullptr);

    FVulkanBufferRHI* Count = FVulkanDeviceRHI::ResourceCast(CountBuffer);
    CHECK(Count != nullptr);

    ConditionalSplitCommandBuffer();

    RequireBufferState(Arguments, ERHIResourceState::IndirectArgument);
    RequireBufferState(Count, ERHIResourceState::IndirectArgument);
    ContextState.PrepareMeshletState();

    CHECK(IsInsideRenderPass());
    CHECK(!BarrierBatcher.HasPendingBarriers());

    ContextState.BindMeshletState();

    GetCommandBuffer()->DrawMeshTasksIndirectCount(
        Arguments->GetBindVkBuffer(),
        Arguments->GetBindOffset() + ArgumentBufferOffset,
        Count->GetBindVkBuffer(),
        Count->GetBindOffset() + CountBufferOffset,
        MaxCommandCount,
        sizeof(FRHIDispatchMeshIndirectParameters));
#else
    UNREFERENCED_VARIABLE(ArgumentBuffer);
    UNREFERENCED_VARIABLE(ArgumentBufferOffset);
    UNREFERENCED_VARIABLE(CountBuffer);
    UNREFERENCED_VARIABLE(CountBufferOffset);
    UNREFERENCED_VARIABLE(MaxCommandCount);
#endif
}

void FVulkanCommandContext::PresentSwapChain(FRHISwapChain* InSwapChain, bool bVerticalSync)
{
    // -------------------------------------------------------------------------------------------
    // We intentionally do not retire or reset the command pool here. The goal is to maintain 
    // a single command pool per command context, per thread, per frame-in-flight. This helps 
    // avoid unnecessary command pool allocations or resets between multiple Present() calls
    // in the same frame.
    // The command pool will instead be explicitly retired at the end of FinishContext(), 
    // ensuring proper lifecycle management without leaks.
    // -------------------------------------------------------------------------------------------

    FVulkanSwapChainRHI* VulkanSwapChain = FVulkanDeviceRHI::ResourceCast(InSwapChain);
    VulkanSwapChain->ClaimPendingSemaphores(GetCommands());

    FinishCommandBuffer(false);

    VulkanSwapChain->Present(bVerticalSync);

    // -------------------------------------------------------------------------------------------
    // Acquire or allocate a fresh command buffer so that subsequent GPU work can continue 
    // recording immediately after presenting.
    // -------------------------------------------------------------------------------------------

    ObtainCommandBuffer();
}

void FVulkanCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    FVulkanSwapChainRHI* VulkanSwapChain = FVulkanDeviceRHI::ResourceCast(SwapChain);
    VulkanSwapChain->Resize(Width, Height, Format, ColorSpace);
}

void FVulkanCommandContext::ClearState()
{
    VerifyExclusiveAccess();

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
    VerifyExclusiveAccess();

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

void FVulkanCommandContext::PushEvent(const StringView& Name)
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
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 1)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteMarker(GetCommandBuffer(), Name);
    }
#endif
}

void FVulkanCommandContext::PopEvent()
{
#if VULKAN_ENABLE_CRASH_MARKERS
    if (FVulkanDeviceRHI::Get()->IsCrashMarkersEnabled() && GetCrashMarkerLevel() >= 1)
    {
        FVulkanDeviceRHI::Get()->GetCrashMarkers()->WriteEndMarker(GetCommandBuffer());
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
