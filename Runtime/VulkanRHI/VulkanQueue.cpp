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

static uint64 ToNanoseconds(uint64 Timestamp)
{
    return static_cast<uint64>(static_cast<double>(Timestamp) * static_cast<double>(VulkanDeviceLimits::TimestampPeriod));
}

FVulkanQueue::FVulkanQueue(FVulkanDevice* InDevice, EVulkanCommandQueueType InQueueType)
    : FVulkanDeviceChild(InDevice)
    , Queue(VK_NULL_HANDLE)
    , QueueFamilyIndex()
    , QueueType(InQueueType)
    , WaitSemaphores()
    , WaitStages()
    , WaitSemaphoreValues()
    , SignalSemaphores()
    , SignalSemaphoreValues()
    , AvailableCommandPools()
    , CommandPools()
    , CommandPoolsCS()
{
}

FVulkanQueue::~FVulkanQueue()
{
    SCOPED_LOCK(CommandPoolsCS);

    for (FVulkanCommandPool* CommandPool : CommandPools)
    {
        delete CommandPool;
    }

    CommandPools.Clear();
    AvailableCommandPools.Clear();
    
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
    SCOPED_LOCK(CommandPoolsCS);

    if (!AvailableCommandPools.IsEmpty())
    {
        FVulkanCommandPool* CommandPool;
        if (AvailableCommandPools.Dequeue(CommandPool))
        {
            CommandPool->Reset(0);
            return CommandPool;
        }
    }

    const VkCommandPoolCreateFlags CommandPoolFlags = GVulkanAllowResetCommandBuffers ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0;
    FVulkanCommandPool* CommandPool = new FVulkanCommandPool(GetDevice(), QueueType);
    if (!CommandPool->Initialize(CommandPoolFlags))
    {
        DEBUG_BREAK();
        delete CommandPool;
        return nullptr;
    }

    CommandPools.Add(CommandPool);
    return CommandPool;
}

void FVulkanQueue::RecycleCommandPool(FVulkanCommandPool* InCommandPool)
{
    if (InCommandPool)
    {
        SCOPED_LOCK(CommandPoolsCS);
        AvailableCommandPools.Enqueue(InCommandPool);
    }
    else
    {
        LOG_WARNING("Trying to Recycle an invalid CommandPool");
    }
}

bool FVulkanQueue::ExecuteCommandBuffer(FVulkanCommandBuffer* const* CommandBuffers, uint32 NumCommandBuffers, FVulkanFence* Fence)
{
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

    TArray<VkCommandBuffer, TInlineArrayAllocator<VkCommandBuffer, 8>> CommandBuffersArray;
    CommandBuffersArray.Resize(NumCommandBuffers);

    if (!CommandBuffersArray.IsEmpty())
    {
        VULKAN_ERROR_COND(CommandBuffers != nullptr, "CommandBuffers cannot be nullptr");

        for (uint32 Index = 0; Index < NumCommandBuffers; ++Index)
        {
            VULKAN_ERROR_COND(CommandBuffers[Index] != nullptr, "CommandBuffer[%d] cannot be nullptr", Index);
            CommandBuffersArray[Index] = CommandBuffers[Index]->GetVkCommandBuffer();
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

    VkFence SignalFence = Fence ?  Fence->GetVkFence() : VK_NULL_HANDLE;
    VkResult Result = vkQueueSubmit(Queue, 1, &SubmitInfo, SignalFence);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkQueueSubmit failed with %s", ToString(Result));
        return false;
    }

    WaitSemaphores.Clear();
    WaitStages.Clear();
    WaitSemaphoreValues.Clear();

    SignalSemaphores.Clear();
    SignalSemaphoreValues.Clear();
    return true;
}

void FVulkanQueue::AddWaitSemaphore(VkSemaphore Semaphore, VkPipelineStageFlags WaitStage)
{
    WaitSemaphores.Add(Semaphore);
    WaitStages.Add(WaitStage);
    WaitSemaphoreValues.Add(0);

    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitStages.Size(), "WaitSemaphores and WaitStages must be the same size");
    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitSemaphoreValues.Size(), "WaitSemaphores and WaitSemaphoreValues must be the same size");
}

void FVulkanQueue::AddWaitTimelineSemaphore(VkSemaphore Semaphore, uint64 Value, VkPipelineStageFlags WaitStage)
{
    WaitSemaphores.Add(Semaphore);
    WaitStages.Add(WaitStage);
    WaitSemaphoreValues.Add(Value);

    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitStages.Size(), "WaitSemaphores and WaitStages must be the same size");
    VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitSemaphoreValues.Size(), "WaitSemaphores and WaitSemaphoreValues must be the same size");
}

void FVulkanQueue::AddSignalSemaphore(VkSemaphore Semaphore)
{
    SignalSemaphores.Add(Semaphore);
    SignalSemaphoreValues.Add(0);

    VULKAN_ERROR_COND(SignalSemaphores.Size() == SignalSemaphoreValues.Size(), "SignalSemaphores and SignalSemaphoreValues must be the same size");
}

void FVulkanQueue::AddSignalTimelineSemaphore(VkSemaphore Semaphore, uint64 Value)
{
    SignalSemaphores.Add(Semaphore);
    SignalSemaphoreValues.Add(Value);

    VULKAN_ERROR_COND(SignalSemaphores.Size() == SignalSemaphoreValues.Size(), "SignalSemaphores and SignalSemaphoreValues must be the same size");
}

void FVulkanQueue::WaitForCompletion()
{
    vkQueueWaitIdle(Queue);
}

bool FVulkanQueue::FlushWaitSemaphoresAndWait()
{
    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext                = nullptr;
    SubmitInfo.commandBufferCount   = 0;
    SubmitInfo.pCommandBuffers      = nullptr;
    SubmitInfo.signalSemaphoreCount = 0;
    SubmitInfo.pSignalSemaphores    = nullptr;

    if (!WaitSemaphores.IsEmpty())
    {
        VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitStages.Size(), "The size of WaitSemaphores and WaitStages must be the same");
        VULKAN_ERROR_COND(WaitSemaphores.Size() == WaitSemaphoreValues.Size(), "The size of WaitSemaphores and WaitSemaphoreValues must be the same");
        
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

    bool bHasTimelineSemaphores = false;
    for (uint64 Value : WaitSemaphoreValues)
    {
        if (Value != 0)
        {
            bHasTimelineSemaphores = true;
            break;
        }
    }

    VkTimelineSemaphoreSubmitInfo TimelineSubmitInfo = {};
    if (bHasTimelineSemaphores)
    {
        TimelineSubmitInfo.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        TimelineSubmitInfo.pNext                     = nullptr;
        TimelineSubmitInfo.waitSemaphoreValueCount   = WaitSemaphoreValues.Size();
        TimelineSubmitInfo.pWaitSemaphoreValues      = WaitSemaphoreValues.IsEmpty() ? nullptr : WaitSemaphoreValues.Data();
        TimelineSubmitInfo.signalSemaphoreValueCount = 0;
        TimelineSubmitInfo.pSignalSemaphoreValues    = nullptr;

        SubmitInfo.pNext = &TimelineSubmitInfo;
    }

    VkResult Result = vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkQueueSubmit failed with %s", ToString(Result));
        return false;
    }

    WaitSemaphores.Clear();
    WaitStages.Clear();
    WaitSemaphoreValues.Clear();

    SignalSemaphores.Clear();
    SignalSemaphoreValues.Clear();

    WaitForCompletion();
    return true;
}

void FVulkanQueue::SubmitCommands(FVulkanCommands* Commands)
{
    CHECK(Commands != nullptr);
    if (Commands->IsEmpty())
    {
        return;
    }

    TScopedLock Lock(SubmissionCS);

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

    const int32 MaxPending = CVarMaxPendingSubmissions.GetValue();
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

void FVulkanQueue::ProcessCommandQueue()
{
    bool bProcess = true;
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
            bProcess = false;
        }
    }
}

FVulkanCommands::FVulkanCommands(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : Queue(InQueue)
    , Device(InDevice)
    , Flags(EVulkanCommandsFlags::None)
    , Fence(nullptr)
    , CommandPools()
    , CommandBuffers()
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

    FVulkanBarrierBatcher BarrierBatcher;

    for (const FVulkanPendingImageBarrier& Pending : PendingImageBarriers)
    {
        FVulkanImageLayoutState& GlobalState = Pending.Texture->GetImageLayoutState();

        const VkImageCreateInfo& CreateInfo = Pending.Texture->GetVkImageCreateInfo();
        const VkImageAspectFlags AspectMask = GetImageAspectFlagsFromFormat(CreateInfo.format);

        if (Pending.Subresource == RHI_ALL_MIP_LEVELS)
        {
            if (GlobalState.AreAllSubresourcesSameLayout())
            {
                const VkImageLayout GlobalLayout = GlobalState.GetImageLayout();
                if (GlobalLayout != Pending.DesiredLayout)
                {
                    VkImageMemoryBarrier2 Barrier = {};
                    Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                    Barrier.oldLayout                       = GlobalLayout;
                    Barrier.newLayout                       = Pending.DesiredLayout;
                    Barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                    Barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                    Barrier.image                           = Pending.Texture->GetVkImage();
                    Barrier.srcAccessMask                   = VK_ACCESS_2_NONE;
                    Barrier.dstAccessMask                   = VK_ACCESS_2_NONE;
                    Barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                    Barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
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
                    const VkImageLayout GlobalLayout = GlobalState.GetSubresourceLayout(i);
                    if (GlobalLayout != Pending.DesiredLayout)
                    {
                        const uint32 MipLevel   = i % CreateInfo.mipLevels;
                        const uint32 ArrayLayer = i / CreateInfo.mipLevels;

                        VkImageMemoryBarrier2 Barrier = {};
                        Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                        Barrier.oldLayout                       = GlobalLayout;
                        Barrier.newLayout                       = Pending.DesiredLayout;
                        Barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                        Barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                        Barrier.image                           = Pending.Texture->GetVkImage();
                        Barrier.srcAccessMask                   = VK_ACCESS_2_NONE;
                        Barrier.dstAccessMask                   = VK_ACCESS_2_NONE;
                        Barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                        Barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
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
            const VkImageLayout GlobalLayout = GlobalState.GetSubresourceLayout(Pending.Subresource);
            if (GlobalLayout != Pending.DesiredLayout)
            {
                const uint32 MipLevel   = Pending.Subresource % CreateInfo.mipLevels;
                const uint32 ArrayLayer = Pending.Subresource / CreateInfo.mipLevels;

                VkImageMemoryBarrier2 Barrier = {};
                Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                Barrier.oldLayout                       = GlobalLayout;
                Barrier.newLayout                       = Pending.DesiredLayout;
                Barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                Barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                Barrier.image                           = Pending.Texture->GetVkImage();
                Barrier.srcAccessMask                   = VK_ACCESS_2_NONE;
                Barrier.dstAccessMask                   = VK_ACCESS_2_NONE;
                Barrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                Barrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
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
            VkBufferMemoryBarrier2 Barrier = {};
            Barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
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

    for (auto It = PendingImageStates.CreateIterator(); !It.IsEnd(); ++It)
    {
        FVulkanTextureRHI*       Texture    = It.GetKey();
        FVulkanImageLayoutState& LocalState = It.GetValue();
        Texture->GetImageLayoutState() = LocalState;
    }

    for (auto It = PendingBufferStates.CreateIterator(); !It.IsEnd(); ++It)
    {
        FVulkanBufferRHI*   Buffer     = It.GetKey();
        FVulkanBufferState& LocalState = It.GetValue();
        Buffer->GetBufferState() = LocalState;
    }

    PendingImageBarriers.Clear();
    PendingBufferBarriers.Clear();
    PendingImageStates.Clear();
    PendingBufferStates.Clear();
}

void FVulkanCommands::Execute()
{
    CHECK(CommandBuffers.IsEmpty() == false);
    CHECK(Fence != nullptr);
    Queue.ExecuteCommandBuffer(CommandBuffers.Data(), CommandBuffers.Size(), Fence);

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
