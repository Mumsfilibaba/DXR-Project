#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"
#include "RHI/RHIQuery.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanCore.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanQuery.h"
#include "VulkanRHI/VulkanDeviceLimits.h"

static TAutoConsoleVariable<int32> CVarMaxPendingSubmissions(
    "VulkanRHI.MaxPendingSubmissions",
    "Maximum number of pending GPU submissions before the CPU waits for the GPU to catch up",
    32);

static TAutoConsoleVariable<int32> CVarCommandContextMaxIdleFrames(
    "VulkanRHI.CommandContextPool.MaxIdleFrames",
    "Number of frames a pooled CommandContext may sit unused before it is destroyed",
    16);

static TAutoConsoleVariable<int32> CVarCommandContextMinRetained(
    "VulkanRHI.CommandContextPool.MinRetained",
    "Number of CommandContexts the pool keeps alive regardless of how long they have been idle",
    2);

#if VULKAN_VALIDATE_IMAGE_LAYOUTS
static TAutoConsoleVariable<int32> CVarBreakOnImageLayoutDesync(
    "VulkanRHI.BreakOnImageLayoutDesync",
    "Break into the debugger when a submission disagrees with the tracked image layout, instead of only logging it",
    0);
#endif

static uint64 ToNanoseconds(uint64 Timestamp)
{
    return static_cast<uint64>(static_cast<double>(Timestamp) * static_cast<double>(VulkanDeviceLimits::TimestampPeriod));
}

#if VULKAN_VALIDATE_IMAGE_LAYOUTS
static void ReportImageLayoutDesync(FVulkanTextureRHI* Texture, const CHAR* Phase, const CHAR* RecordingSite, VkImageLayout CommandBufferLayout, VkImageLayout TrackedLayout)
{
    // Only the log message consumes these, and logging compiles out in Release
    UNREFERENCED_VARIABLE(Phase);
    UNREFERENCED_VARIABLE(RecordingSite);
    UNREFERENCED_VARIABLE(CommandBufferLayout);
    UNREFERENCED_VARIABLE(TrackedLayout);

    String TextureName;
    Texture->GetDebugName(TextureName);

    VULKAN_ERROR("Image layout desync (%s) on '%s' recorded by %s: the submission says %s but the tracker says %s",
        Phase, *TextureName, RecordingSite ? RecordingSite : "<unknown>", ToString(CommandBufferLayout), ToString(TrackedLayout));

    if (CVarBreakOnImageLayoutDesync.GetValue() != 0)
    {
        DEBUG_BREAK();
    }
}
#endif

FVulkanQueue::FVulkanQueue(FVulkanDevice* InDevice, EVulkanCommandQueueType InQueueType)
    : FVulkanDeviceChild(InDevice)
    , Queue(VK_NULL_HANDLE)
    , QueueFamilyIndex()
    , QueueType(InQueueType)
{
}

FVulkanQueue::~FVulkanQueue()
{
    WaitForCompletion();
    ProcessCommandQueue();

    CommandContextPool.DestroyAll();
    CommandPoolPool.DestroyAll();

    Queue = VK_NULL_HANDLE;
}

bool FVulkanQueue::Initialize()
{
    TOptional<FVulkanQueueFamilyIndices> QueueIndices = GetDevice()->GetQueueIndicies();
    VULKAN_ERROR_COND(QueueIndices.HasValue(), "Queue Families is not initialized correctly");

    QueueFamilyIndex = GetDevice()->GetQueueIndexFromType(QueueType);
    vkGetDeviceQueue(GetDevice()->GetVkDevice(), QueueFamilyIndex, 0, &Queue);
    return true;
}

FVulkanCommandPool* FVulkanQueue::ObtainCommandPool()
{
    FVulkanCommandPool* CommandPool = CommandPoolPool.Acquire([this](int32) -> FVulkanCommandPool*
    {
        const VkCommandPoolCreateFlags CommandPoolFlags = GVulkanAllowResetCommandBuffers ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0;

        FVulkanCommandPool* NewCommandPool = new FVulkanCommandPool(GetDevice(), QueueType);
        if (!NewCommandPool->Initialize(CommandPoolFlags))
        {
            DEBUG_BREAK();
            delete NewCommandPool;
            return nullptr;
        }

        return NewCommandPool;
    });

    if (CommandPool)
    {
        CommandPool->Reset(0);
    }

    return CommandPool;
}

void FVulkanQueue::RecycleCommandPool(FVulkanCommandPool* InCommandPool)
{
    CommandPoolPool.Release(InCommandPool);
}

void FVulkanQueue::RetireCommandPoolDeferred(FVulkanCommandPool* InCommandPool)
{
    CHECK(InCommandPool != nullptr);

    SCOPED_LOCK(DeferredCommandPoolsCS);
    DeferredCommandPools.Add(InCommandPool);
}

FVulkanCommandContext* FVulkanQueue::ObtainCommandContext()
{
    FVulkanCommandContext* CommandContext = CommandContextPool.Acquire([this](int32) -> FVulkanCommandContext*
    {
        FVulkanCommandContext* NewCommandContext = new FVulkanCommandContext(GetDevice(), *this);
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
        VULKAN_ERROR_CRITICAL("Failed to Obtain CommandContext");
    }

    return CommandContext;
}

void FVulkanQueue::ReleaseCommandContext(FVulkanCommandContext* InContext)
{
    CHECK(InContext != nullptr);
    CHECK(!InContext->IsRecording());

    InContext->RetireTransientObjects();

    InContext->SetLastUsedFrame(CurrentFrame.Load());
    CommandContextPool.Release(InContext);
}

void FVulkanQueue::PruneCommandContexts(uint64 InCurrentFrame)
{
    CurrentFrame.Store(InCurrentFrame);

    const uint64 MaxIdleFrames = static_cast<uint64>(Math::Max<int32>(0, CVarCommandContextMaxIdleFrames.GetValue()));
    const int32  MinRetained   = Math::Max<int32>(0, CVarCommandContextMinRetained.GetValue());

    CommandContextPool.PruneFree(MinRetained, [InCurrentFrame, MaxIdleFrames](FVulkanCommandContext* Context)
    {
        return (InCurrentFrame - Context->GetLastUsedFrame()) > MaxIdleFrames;
    });
}

bool FVulkanQueue::ExecuteCommands(FVulkanCommands& InCommands)
{
    const TArray<VkSemaphore>&          WaitSemaphores        = InCommands.WaitSemaphores;
    const TArray<VkPipelineStageFlags>& WaitStages            = InCommands.WaitStages;
    const TArray<uint64>&               WaitSemaphoreValues   = InCommands.WaitSemaphoreValues;
    const TArray<VkSemaphore>&          SignalSemaphores      = InCommands.SignalSemaphores;
    const TArray<uint64>&               SignalSemaphoreValues = InCommands.SignalSemaphoreValues;

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = nullptr;

    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitStages.Size(), "The size of WaitSemaphores and WaitStages must be the same");
    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitSemaphoreValues.Size(), "The size of WaitSemaphores and WaitSemaphoreValues must be the same");
    VULKAN_ERROR_COND(SignalSemaphores.Size() == SignalSemaphoreValues.Size(), "The size of SignalSemaphores and SignalSemaphoreValues must be the same");

    if (!WaitSemaphores.IsEmpty())
    {
        SubmitInfo.waitSemaphoreCount = WaitSemaphores.Size();
        SubmitInfo.pWaitSemaphores    = WaitSemaphores.Data();
        SubmitInfo.pWaitDstStageMask  = WaitStages.Data();
    }
    else
    {
        SubmitInfo.waitSemaphoreCount = 0;
        SubmitInfo.pWaitSemaphores    = nullptr;
        SubmitInfo.pWaitDstStageMask  = nullptr;
    }

    if (!SignalSemaphores.IsEmpty())
    {
        SubmitInfo.signalSemaphoreCount = SignalSemaphores.Size();
        SubmitInfo.pSignalSemaphores    = SignalSemaphores.Data();
    }
    else
    {
        SubmitInfo.signalSemaphoreCount = 0;
        SubmitInfo.pSignalSemaphores    = nullptr;
    }

    const uint32 NumCommandBuffers = static_cast<uint32>(InCommands.CommandBuffers.Size());

    TArray<VkCommandBuffer, TInlineArrayAllocator<VkCommandBuffer, 8>> CommandBuffersArray;
    CommandBuffersArray.Resize(NumCommandBuffers);

    if (!CommandBuffersArray.IsEmpty())
    {
        for (uint32 Index = 0; Index < NumCommandBuffers; ++Index)
        {
            FVulkanCommandBuffer* CommandBuffer = InCommands.CommandBuffers[Index];
            VULKAN_ERROR_COND(CommandBuffer != nullptr, "CommandBuffer[%d] cannot be nullptr", Index);
            CommandBuffersArray[Index] = CommandBuffer->GetVkCommandBuffer();
        }

        SubmitInfo.commandBufferCount = CommandBuffersArray.Size();
        SubmitInfo.pCommandBuffers    = CommandBuffersArray.Data();
    }
    else
    {
        SubmitInfo.commandBufferCount = 0;
        SubmitInfo.pCommandBuffers    = nullptr;
    }

    bool bHasTimelineSemaphores = false;
    for (uint64 Value : WaitSemaphoreValues)
    {
        if (Value != 0)
        {
            bHasTimelineSemaphores = true;
            break;
        }
    }
    if (!bHasTimelineSemaphores)
    {
        for (uint64 Value : SignalSemaphoreValues)
        {
            if (Value != 0)
            {
                bHasTimelineSemaphores = true;
                break;
            }
        }
    }

    VkTimelineSemaphoreSubmitInfo TimelineSubmitInfo = {};
    if (bHasTimelineSemaphores)
    {
        TimelineSubmitInfo.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        TimelineSubmitInfo.pNext                     = nullptr;
        TimelineSubmitInfo.waitSemaphoreValueCount   = WaitSemaphoreValues.Size();
        TimelineSubmitInfo.pWaitSemaphoreValues      = WaitSemaphoreValues.IsEmpty() ? nullptr : WaitSemaphoreValues.Data();
        TimelineSubmitInfo.signalSemaphoreValueCount = SignalSemaphoreValues.Size();
        TimelineSubmitInfo.pSignalSemaphoreValues    = SignalSemaphoreValues.IsEmpty() ? nullptr : SignalSemaphoreValues.Data();

        SubmitInfo.pNext = &TimelineSubmitInfo;
    }

    VkFence SignalFence = InCommands.Fence ? InCommands.Fence->GetVkFence() : VK_NULL_HANDLE;

    VkResult Result = VK_SUCCESS;
    {
        SCOPED_LOCK(QueueCS);
        Result = vkQueueSubmit(Queue, 1, &SubmitInfo, SignalFence);
    }

    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkQueueSubmit failed with %s", ToString(Result));
        return false;
    }

    InCommands.WaitSemaphores.Clear();
    InCommands.WaitStages.Clear();
    InCommands.WaitSemaphoreValues.Clear();

    InCommands.SignalSemaphores.Clear();
    InCommands.SignalSemaphoreValues.Clear();
    return true;
}

void FVulkanQueue::WaitForCompletion()
{
    SCOPED_LOCK(QueueCS);
    vkQueueWaitIdle(Queue);
}

VkResult FVulkanQueue::Present(const VkPresentInfoKHR& PresentInfo)
{
    SCOPED_LOCK(QueueCS);
    return vkQueuePresentKHR(Queue, &PresentInfo);
}

bool FVulkanQueue::SubmitSemaphoresOnly(VkSemaphore WaitSemaphore, VkPipelineStageFlags WaitStage, VkSemaphore SignalSemaphore)
{
    if (!VULKAN_CHECK_HANDLE(WaitSemaphore) && !VULKAN_CHECK_HANDLE(SignalSemaphore))
    {
        return true;
    }

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext              = nullptr;
    SubmitInfo.commandBufferCount = 0;
    SubmitInfo.pCommandBuffers    = nullptr;

    if (VULKAN_CHECK_HANDLE(WaitSemaphore))
    {
        SubmitInfo.waitSemaphoreCount = 1;
        SubmitInfo.pWaitSemaphores    = &WaitSemaphore;
        SubmitInfo.pWaitDstStageMask  = &WaitStage;
    }

    if (VULKAN_CHECK_HANDLE(SignalSemaphore))
    {
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores    = &SignalSemaphore;
    }

    VkResult Result = VK_SUCCESS;
    {
        SCOPED_LOCK(QueueCS);
        Result = vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE);
    }

    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkQueueSubmit failed with %s", ToString(Result));
        return false;
    }

    return true;
}

void FVulkanQueue::RetireDeferredObjects(TArray<FVulkanDeferredObject>&& InObjects)
{
    if (InObjects.IsEmpty())
    {
        return;
    }

    SCOPED_LOCK(DeferredObjectsCS);

    if (DeferredObjects.IsEmpty())
    {
        DeferredObjects = Move(InObjects);
    }
    else
    {
        DeferredObjects.Append(InObjects);
        InObjects.Clear();
    }
}

void FVulkanQueue::SubmitCommands(FVulkanCommands* Commands)
{
    CHECK(Commands != nullptr);
    if (Commands->IsEmpty())
    {
        return;
    }

    {
        TScopedLock Lock(QueueCS);

        Commands->PreExecute();

    #if !VULKAN_USE_CPU_QUERY_RESOLVE
        const bool bResolveQueries = IsEnumFlagSet(Commands->Flags, EVulkanCommandsFlags::ResolveQueries);
        if (bResolveQueries)
        {
            if (!PendingTimestampQueries.IsEmpty())
            {
                Commands->TimestampQueries.Insert(0, PendingTimestampQueries);
                PendingTimestampQueries.Clear();
            }

            if (!PendingOcclusionQueries.IsEmpty())
            {
                Commands->OcclusionQueries.Insert(0, PendingOcclusionQueries);
                PendingOcclusionQueries.Clear();
            }

            if (!PendingPipelineStatsQueries.IsEmpty())
            {
                Commands->PipelineStatsQueries.Insert(0, PendingPipelineStatsQueries);
                PendingPipelineStatsQueries.Clear();
            }

            if (!PendingQueryRHIs.IsEmpty())
            {
                Commands->PendingQueries.Insert(0, PendingQueryRHIs);
                PendingQueryRHIs.Clear();
            }
        }
        else
        {
            PendingTimestampQueries.Append(Commands->TimestampQueries);
            PendingOcclusionQueries.Append(Commands->OcclusionQueries);
            PendingPipelineStatsQueries.Append(Commands->PipelineStatsQueries);
            PendingQueryRHIs.Append(Commands->PendingQueries);

            Commands->TimestampQueries.Clear();
            Commands->OcclusionQueries.Clear();
            Commands->PipelineStatsQueries.Clear();
            Commands->PendingQueries.Clear();
        }
    #endif

        Commands->Execute();

        PendingSubmissions.Enqueue(Commands);
    }

    const int32 MaxPending = CVarMaxPendingSubmissions.GetValue();
    {
        SCOPED_LOCK(ConsumerCS);

        while (PendingSubmissions.Size() > MaxPending)
        {
            FVulkanCommands* Oldest = nullptr;
            if (PendingSubmissions.Peek(Oldest) && Oldest)
            {
                Oldest->Fence->Wait(UINT64_MAX);
                PendingSubmissions.Dequeue();
                Oldest->PostExecute();
            }
            else
            {
                break;
            }
        }
    }
}

void FVulkanQueue::ProcessCommandQueue()
{
    SCOPED_LOCK(ConsumerCS);

    bool bProcess    = true;
    bool bFullyDrain = false;

    while (bProcess)
    {
        FVulkanCommands* Commands = nullptr;
        if (PendingSubmissions.Peek(Commands))
        {
            CHECK(Commands != nullptr);
            if (!Commands->IsExecutionFinished())
            {
                bProcess = false;
            }
            else
            {
                PendingSubmissions.Dequeue();
                Commands->PostExecute();
            }
        }
        else
        {
            bFullyDrain = true;
            bProcess    = false;
        }
    }

    if (bFullyDrain)
    {
        {
            SCOPED_LOCK(DeferredCommandPoolsCS);

            for (FVulkanCommandPool* CommandPool : DeferredCommandPools)
            {
                CommandPoolPool.Release(CommandPool);
            }

            DeferredCommandPools.Clear();
        }

        TArray<FVulkanDeferredObject> ObjectsToRetire;
        {
            SCOPED_LOCK(DeferredObjectsCS);
            ObjectsToRetire = Move(DeferredObjects);
        }

        FVulkanDeferredObject::ProcessItems(GetDevice(), ObjectsToRetire);
    }
}

FVulkanCommands::FVulkanCommands(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : Queue(InQueue)
    , Device(InDevice)
    , Flags(EVulkanCommandsFlags::None)
    , Fence(nullptr)
    , CommandPools()
    , CommandBuffers()
    , WaitSemaphores()
    , WaitStages()
    , WaitSemaphoreValues()
    , SignalSemaphores()
    , SignalSemaphoreValues()
    , QueryRanges()
    , TimestampQueries()
    , OcclusionQueries()
    , PipelineStatsQueries()
    , PendingQueries()
    , DeferredObjects()
{
}

FVulkanCommands::~FVulkanCommands()
{
    CHECK(Fence == nullptr);
}

void FVulkanCommands::AcquireFence()
{
    FVulkanFenceManager& FenceManager = Device->GetFenceManager();
    Fence = FenceManager.ObtainFence();
    CHECK(Fence != nullptr);
}

void FVulkanCommands::PreExecute()
{
    for (int32 i = 0; i < CommandBuffers.Size(); i++)
    {
        FVulkanCommandBuffer* CommandBuffer = CommandBuffers[i];
        TimestampQueries.Append(CommandBuffer->TimestampQueries);
        OcclusionQueries.Append(CommandBuffer->OcclusionQueries);
        PipelineStatsQueries.Append(CommandBuffer->PipelineStatsQueries);

        CommandBuffer->TimestampQueries.Clear();
        CommandBuffer->OcclusionQueries.Clear();
        CommandBuffer->PipelineStatsQueries.Clear();
    }

#if VULKAN_VALIDATE_IMAGE_LAYOUTS
    ValidatePendingBarrierOrdering();
#endif

    FVulkanBarrierBatcher BarrierBatcher;

    const auto ResolveSourceLayout = [](VkImageLayout Layout)
    {
        return (Layout == VK_IMAGE_LAYOUT_TO_BE_DETERMINED) ? VK_IMAGE_LAYOUT_UNDEFINED : Layout;
    };

    for (const FVulkanPendingImageBarrier& Pending : PendingImageBarriers)
    {
        FVulkanImageLayoutState& GlobalState = Pending.Texture->GetImageLayoutState();

        const VkImageCreateInfo& CreateInfo = Pending.Texture->GetVkImageCreateInfo();
        const VkImageAspectFlags AspectMask = GetImageAspectFlagsFromFormat(CreateInfo.format);

        if (Pending.Subresource == RHI_ALL_MIP_LEVELS)
        {
            if (GlobalState.AreAllSubresourcesSameLayout())
            {
                const VkImageLayout GlobalLayout = ResolveSourceLayout(GlobalState.GetImageLayout());
                if (GlobalLayout != Pending.DesiredLayout)
                {
                    VkImageMemoryBarrier2KHR Barrier = {};
                    Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                    Barrier.oldLayout                       = GlobalLayout;
                    Barrier.newLayout                       = Pending.DesiredLayout;
                    Barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                    Barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                    Barrier.image                           = Pending.Texture->GetVkImage();
                    Barrier.srcAccessMask                   = VK_ACCESS_2_NONE_KHR;
                    Barrier.dstAccessMask                   = VK_ACCESS_2_NONE_KHR;
                    Barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                    Barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                    Barrier.subresourceRange.aspectMask     = AspectMask;
                    Barrier.subresourceRange.baseArrayLayer = 0;
                    Barrier.subresourceRange.baseMipLevel   = 0;
                    Barrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
                    Barrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
                    BarrierBatcher.AddImageMemoryBarrier(0, Barrier);
                }
            }
            else
            {
                const uint32 NumSubresources = GlobalState.GetNumSubresources();
                for (uint32 i = 0; i < NumSubresources; i++)
                {
                    const VkImageLayout GlobalLayout = ResolveSourceLayout(GlobalState.GetSubresourceLayout(i));
                    if (GlobalLayout != Pending.DesiredLayout)
                    {
                        const uint32 MipLevel   = i % CreateInfo.mipLevels;
                        const uint32 ArrayLayer = i / CreateInfo.mipLevels;

                        VkImageMemoryBarrier2KHR Barrier = {};
                        Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                        Barrier.oldLayout                       = GlobalLayout;
                        Barrier.newLayout                       = Pending.DesiredLayout;
                        Barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                        Barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                        Barrier.image                           = Pending.Texture->GetVkImage();
                        Barrier.srcAccessMask                   = VK_ACCESS_2_NONE_KHR;
                        Barrier.dstAccessMask                   = VK_ACCESS_2_NONE_KHR;
                        Barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                        Barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                        Barrier.subresourceRange.aspectMask     = AspectMask;
                        Barrier.subresourceRange.baseArrayLayer = ArrayLayer;
                        Barrier.subresourceRange.baseMipLevel   = MipLevel;
                        Barrier.subresourceRange.layerCount     = 1;
                        Barrier.subresourceRange.levelCount     = 1;
                        BarrierBatcher.AddImageMemoryBarrier(0, Barrier);
                    }
                }
            }
        }
        else
        {
            const VkImageLayout GlobalLayout = ResolveSourceLayout(GlobalState.GetSubresourceLayout(Pending.Subresource));
            if (GlobalLayout != Pending.DesiredLayout)
            {
                const uint32 MipLevel   = Pending.Subresource % CreateInfo.mipLevels;
                const uint32 ArrayLayer = Pending.Subresource / CreateInfo.mipLevels;

                VkImageMemoryBarrier2KHR Barrier = {};
                Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
                Barrier.oldLayout                       = GlobalLayout;
                Barrier.newLayout                       = Pending.DesiredLayout;
                Barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                Barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                Barrier.image                           = Pending.Texture->GetVkImage();
                Barrier.srcAccessMask                   = VK_ACCESS_2_NONE_KHR;
                Barrier.dstAccessMask                   = VK_ACCESS_2_NONE_KHR;
                Barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                Barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
                Barrier.subresourceRange.aspectMask     = AspectMask;
                Barrier.subresourceRange.baseArrayLayer = ArrayLayer;
                Barrier.subresourceRange.baseMipLevel   = MipLevel;
                Barrier.subresourceRange.layerCount     = 1;
                Barrier.subresourceRange.levelCount     = 1;
                BarrierBatcher.AddImageMemoryBarrier(0, Barrier);
            }
        }
    }

    for (const FVulkanPendingBufferBarrier& Pending : PendingBufferBarriers)
    {
        FVulkanBufferState& GlobalState = Pending.Buffer->GetBufferState();

        if (GlobalState.GetAccess() != Pending.DesiredAccess || GlobalState.GetStage() != Pending.DesiredStage)
        {
            VkBufferMemoryBarrier2KHR Barrier = {};
            Barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
            Barrier.srcAccessMask       = GlobalState.GetAccess();
            Barrier.dstAccessMask       = Pending.DesiredAccess;
            Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            Barrier.srcStageMask        = GlobalState.GetStage();
            Barrier.dstStageMask        = Pending.DesiredStage;
            Barrier.buffer              = Pending.Buffer->GetVkBuffer();
            Barrier.offset              = 0;
            Barrier.size                = VK_WHOLE_SIZE;
            BarrierBatcher.AddBufferMemoryBarrier(0, Barrier);
        }
    }

    if (BarrierBatcher.HasPendingBarriers())
    {
        FVulkanCommandPool*   FixupPool          = Queue.ObtainCommandPool();
        FVulkanCommandBuffer* FixupCommandBuffer = FixupPool->GetOrCreateBuffer();

        if (FixupCommandBuffer && FixupCommandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
        {
            BarrierBatcher.FlushBarriers(*FixupCommandBuffer);

            if (FixupCommandBuffer->End())
            {
                CommandBuffers.Insert(0, FixupCommandBuffer);
                AddCommandPool(FixupPool);
            }
        }
    }

#if VULKAN_VALIDATE_IMAGE_LAYOUTS
    ValidateImageLayouts();
#endif

    for (auto It = PendingImageStates.CreateIterator(); !It.IsEnd(); ++It)
    {
        FVulkanTextureRHI*       Texture    = It.GetKey();
        FVulkanImageLayoutState& LocalState = It.GetValue();
        Texture->GetImageLayoutState().AdoptTrackedState(LocalState);
    }

    for (auto It = PendingBufferStates.CreateIterator(); !It.IsEnd(); ++It)
    {
        FVulkanBufferRHI*   Buffer     = It.GetKey();
        FVulkanBufferState& LocalState = It.GetValue();
        Buffer->GetBufferState().AdoptTrackedState(LocalState);
    }

    PendingImageBarriers.Clear();
    PendingBufferBarriers.Clear();
    PendingImageStates.Clear();
    PendingBufferStates.Clear();
}

#if VULKAN_VALIDATE_IMAGE_LAYOUTS
void FVulkanCommands::ValidatePendingBarrierOrdering()
{
    TMap<VkImage, FVulkanImageLayoutValidationEntry> FirstEntries;
    for (FVulkanCommandBuffer* CommandBuffer : CommandBuffers)
    {
        TMap<VkImage, FVulkanImageLayoutValidationEntry>& Entries = CommandBuffer->GetImageLayoutValidationEntries();
        for (auto It = Entries.CreateIterator(); !It.IsEnd(); ++It)
        {
            const VkImage Image = It.GetKey();
            if (!FirstEntries.Contains(Image))
            {
                FirstEntries.FindOrAdd(Image) = It.GetValue();
            }
        }
    }

    for (const FVulkanPendingImageBarrier& Pending : PendingImageBarriers)
    {
        if (!Pending.Texture)
        {
            continue;
        }

        const VkImage Image = Pending.Texture->GetVkImage();
        if (!VULKAN_CHECK_HANDLE(Image))
        {
            continue;
        }

        const FVulkanImageLayoutValidationEntry* Entry = FirstEntries.Find(Image);
        if (!Entry || !Entry->bWholeImage || Entry->ExpectedEntryLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            continue;
        }

        if (Entry->ExpectedEntryLayout != Pending.DesiredLayout)
        {
            ReportImageLayoutDesync(Pending.Texture, "hoisted fixup", Entry->RecordingSite, Entry->ExpectedEntryLayout, Pending.DesiredLayout);
        }
    }
}

void FVulkanCommands::ValidateImageLayouts()
{
    TMap<VkImage, FVulkanTextureRHI*> ImageToTexture;
    for (auto It = PendingImageStates.CreateIterator(); !It.IsEnd(); ++It)
    {
        FVulkanTextureRHI* Texture = It.GetKey();
        if (Texture && VULKAN_CHECK_HANDLE(Texture->GetVkImage()))
        {
            ImageToTexture.FindOrAdd(Texture->GetVkImage()) = Texture;
        }
    }

    TMap<VkImage, VkImageLayout>                     SimulatedLayouts;
    TMap<VkImage, FVulkanImageLayoutValidationEntry> BatchEntries;

    for (FVulkanCommandBuffer* CommandBuffer : CommandBuffers)
    {
        TMap<VkImage, FVulkanImageLayoutValidationEntry>& Entries = CommandBuffer->GetImageLayoutValidationEntries();
        for (auto It = Entries.CreateIterator(); !It.IsEnd(); ++It)
        {
            const VkImage                            Image = It.GetKey();
            const FVulkanImageLayoutValidationEntry& Entry = It.GetValue();

            FVulkanTextureRHI** TexturePtr = ImageToTexture.Find(Image);
            FVulkanTextureRHI*  Texture    = TexturePtr ? *TexturePtr : nullptr;

            const bool bCheckable = Entry.bWholeImage && Texture != nullptr && Texture->GetImageLayoutState().AreAllSubresourcesSameLayout();
            if (bCheckable && Entry.ExpectedEntryLayout != VK_IMAGE_LAYOUT_UNDEFINED)
            {
                const VkImageLayout* Simulated = SimulatedLayouts.Find(Image);
                const VkImageLayout  Tracked   = Simulated ? *Simulated : Texture->GetImageLayoutState().GetImageLayout();

                if (Tracked != Entry.ExpectedEntryLayout && Tracked != VK_IMAGE_LAYOUT_TO_BE_DETERMINED)
                {
                    ReportImageLayoutDesync(Texture, "entry", Entry.RecordingSite, Entry.ExpectedEntryLayout, Tracked);
                }
            }

            if (Entry.bWholeImage)
            {
                SimulatedLayouts.FindOrAdd(Image) = Entry.FinalLayout;
                BatchEntries.FindOrAdd(Image)     = Entry;
            }
            else
            {
                SimulatedLayouts.Remove(Image);
                BatchEntries.Remove(Image);
            }
        }
    }

    for (auto It = BatchEntries.CreateIterator(); !It.IsEnd(); ++It)
    {
        const VkImage                            Image = It.GetKey();
        const FVulkanImageLayoutValidationEntry& Entry = It.GetValue();

        FVulkanTextureRHI** TexturePtr = ImageToTexture.Find(Image);
        FVulkanTextureRHI*  Texture    = TexturePtr ? *TexturePtr : nullptr;
        if (!Texture)
        {
            continue;
        }

        FVulkanImageLayoutState* LocalState = PendingImageStates.Find(Texture);
        if (!LocalState || !LocalState->AreAllSubresourcesSameLayout())
        {
            continue;
        }

        const VkImageLayout AdoptedLayout = LocalState->GetImageLayout();
        if (AdoptedLayout != VK_IMAGE_LAYOUT_TO_BE_DETERMINED && AdoptedLayout != Entry.FinalLayout)
        {
            ReportImageLayoutDesync(Texture, "exit", Entry.RecordingSite, Entry.FinalLayout, AdoptedLayout);
        }
    }
}
#endif

void FVulkanCommands::AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage)
{
    AddWaitTimelineSemaphore(Semaphore, 0, WaitStage);
}

void FVulkanCommands::AddWaitTimelineSemaphore(VkSemaphore Semaphore, uint64 Value, VkPipelineStageFlags WaitStage)
{
    WaitSemaphores.Add(Semaphore);
    WaitStages.Add(WaitStage);
    WaitSemaphoreValues.Add(Value);

    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitStages.Size(), "WaitSemaphores and WaitStages must be the same size");
    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitSemaphoreValues.Size(), "WaitSemaphores and WaitSemaphoreValues must be the same size");
}

void FVulkanCommands::AddSignalSemaphore(VkSemaphore Semaphore)
{
    AddSignalTimelineSemaphore(Semaphore, 0);
}

void FVulkanCommands::AddSignalTimelineSemaphore(VkSemaphore Semaphore, uint64 Value)
{
    SignalSemaphores.Add(Semaphore);
    SignalSemaphoreValues.Add(Value);

    VULKAN_ERROR_COND(SignalSemaphores.Size() == SignalSemaphoreValues.Size(), "SignalSemaphores and SignalSemaphoreValues must be the same size");
}

void FVulkanCommands::Execute()
{
    CHECK(!CommandBuffers.IsEmpty() || HasPendingSemaphores());
    CHECK(Fence != nullptr);
    Queue.ExecuteCommands(*this);

    for (int32 i = 0; i < PendingQueries.Size(); i++)
    {
        PendingQueries[i]->SyncFence = MakeSharedRef<FVulkanFence>(Fence);
    }
    PendingQueries.Clear();
}

void FVulkanCommands::PostExecute()
{
    TArray<uint64> RawTicksArray;
    RawTicksArray.Resize(TimestampQueries.Size());

    for (int32 i = 0; i < TimestampQueries.Size(); i++)
    {
        RawTicksArray[i] = 0;
        
        const FVulkanQuery& Query = TimestampQueries[i];
        if (Query.QueryPool)
        {
            Query.CopyResult(&RawTicksArray[i], sizeof(uint64));
        }
    }

    uint64 AccumulatedIdleTicks      = 0;
    uint64 LastCommandBufferEndTicks = 0;
    
    bool bHaveLastCommandBufferEnd = false;
    for (int32 i = 0; i < TimestampQueries.Size(); i++)
    {
        const FVulkanQuery& Query = TimestampQueries[i];
        const uint64 Ticks = RawTicksArray[i];

        if (Query.Type == EVulkanQueryType::CommandListEnd)
        {
            LastCommandBufferEndTicks = Ticks;
            bHaveLastCommandBufferEnd = true;
        }
        else if (Query.Type == EVulkanQueryType::CommandListBegin && bHaveLastCommandBufferEnd)
        {
            if (Ticks > LastCommandBufferEndTicks)
            {
                AccumulatedIdleTicks += Ticks - LastCommandBufferEndTicks;
            }

            bHaveLastCommandBufferEnd = false;
        }

        if (Query.Type == EVulkanQueryType::Timestamp && Query.ResultTarget)
        {
            const uint64 AdjustedTicks = (Ticks > AccumulatedIdleTicks) ? (Ticks - AccumulatedIdleTicks) : 0;
            *Query.ResultTarget = ToNanoseconds(AdjustedTicks);
        }
    }

    TimestampQueries.Clear();

    for (int32 QueryIdx = 0; QueryIdx < OcclusionQueries.Size(); QueryIdx++)
    {
        const FVulkanQuery& Query = OcclusionQueries[QueryIdx];
        if (!Query.QueryPool || !Query.ResultTarget)
        {
            continue;
        }

        uint64 NumSamples = 0;
        if (Query.CopyResult(&NumSamples, sizeof(uint64)))
        {
            *Query.ResultTarget = NumSamples;
        }
    }

    OcclusionQueries.Clear();

    for (int32 QueryIdx = 0; QueryIdx < PipelineStatsQueries.Size(); QueryIdx++)
    {
        const FVulkanQuery& Query = PipelineStatsQueries[QueryIdx];
        if (!Query.QueryPool || !Query.ResultTarget)
        {
            continue;
        }

        struct FPipelineStats
        {
            uint64 IAVertices;
            uint64 IAPrimitives;
            uint64 VSInvocations;
            uint64 GSInvocations;
            uint64 GSPrimitives;
            uint64 CInvocations;
            uint64 CPrimitives;
            uint64 PSInvocations;
            uint64 HSInvocations;
            uint64 DSInvocations;
            uint64 CSInvocations;
        };

        FPipelineStats VulkanStats = {};
        if (Query.CopyResult(&VulkanStats, sizeof(VulkanStats)))
        {
            FRHIPipelineStatistics* Stats = reinterpret_cast<FRHIPipelineStatistics*>(Query.ResultTarget);
            Stats->IAVertices    = VulkanStats.IAVertices;
            Stats->IAPrimitives  = VulkanStats.IAPrimitives;
            Stats->VSInvocations = VulkanStats.VSInvocations;
            Stats->GSInvocations = VulkanStats.GSInvocations;
            Stats->GSPrimitives  = VulkanStats.GSPrimitives;
            Stats->CInvocations  = VulkanStats.CInvocations;
            Stats->CPrimitives   = VulkanStats.CPrimitives;
            Stats->PSInvocations = VulkanStats.PSInvocations;
            Stats->HSInvocations = VulkanStats.HSInvocations;
            Stats->DSInvocations = VulkanStats.DSInvocations;
            Stats->CSInvocations = VulkanStats.CSInvocations;
            Stats->ASInvocations = 0;
            Stats->MSInvocations = 0;
            Stats->MSPrimitives  = 0;
        }
    }

    PipelineStatsQueries.Clear();

    for (int32 RangeIdx = 0; RangeIdx < QueryRanges.Size(); RangeIdx++)
    {
        FVulkanQueryPool* Pool = QueryRanges[RangeIdx].Pool;
        Device->RecycleQueryPool(Pool);
    }

    QueryRanges.Clear();

    for (int32 CmdBufIdx = 0; CmdBufIdx < CommandBuffers.Size(); CmdBufIdx++)
    {
        FVulkanCommandPool* CommandPool = CommandBuffers[CmdBufIdx]->GetOwnerPool();
        CommandPool->RecycleBuffer(CommandBuffers[CmdBufIdx]);
    }

    CommandBuffers.Clear();

    for (int32 PoolIdx = 0; PoolIdx < CommandPools.Size(); PoolIdx++)
    {
        Queue.RecycleCommandPool(CommandPools[PoolIdx]);
    }

    CommandPools.Clear();

    FVulkanFenceManager& FenceManager = Device->GetFenceManager();
    FenceManager.RecycleFence(Fence);
    Fence = nullptr;

    FVulkanDeferredObject::ProcessItems(Device, DeferredObjects);
    DeferredObjects.Clear();

    PendingImageBarriers.Clear();
    PendingBufferBarriers.Clear();
    PendingImageStates.Clear();
    PendingBufferStates.Clear();

    delete this;
}
