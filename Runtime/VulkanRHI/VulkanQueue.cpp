#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanCore.h"
#include "VulkanRHI/VulkanCommandContext.h"

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
        TimelineSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        TimelineSubmitInfo.pNext = nullptr;

        TimelineSubmitInfo.waitSemaphoreValueCount   = WaitSemaphoreValues.Size();
        TimelineSubmitInfo.pWaitSemaphoreValues      = WaitSemaphoreValues.IsEmpty() ? nullptr : WaitSemaphoreValues.Data();
        TimelineSubmitInfo.signalSemaphoreValueCount = SignalSemaphoreValues.Size();
        TimelineSubmitInfo.pSignalSemaphoreValues    = SignalSemaphoreValues.IsEmpty() ? nullptr : SignalSemaphoreValues.Data();

        SubmitInfo.pNext = &TimelineSubmitInfo;
    }

    VkFence SignalFence = Fence ? Fence->GetVkFence() : VK_NULL_HANDLE;
    VkResult Result = vkQueueSubmit(Queue, 1, &SubmitInfo, SignalFence);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkQueueSubmit failed");
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
        TimelineSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        TimelineSubmitInfo.pNext = nullptr;
        TimelineSubmitInfo.waitSemaphoreValueCount   = WaitSemaphoreValues.Size();
        TimelineSubmitInfo.pWaitSemaphoreValues      = WaitSemaphoreValues.IsEmpty() ? nullptr : WaitSemaphoreValues.Data();
        TimelineSubmitInfo.signalSemaphoreValueCount = 0;
        TimelineSubmitInfo.pSignalSemaphoreValues    = nullptr;
        SubmitInfo.pNext = &TimelineSubmitInfo;
    }

    VkResult Result = vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkQueueSubmit failed");
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

FVulkanCommands::FVulkanCommands(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : Queue(InQueue)
    , Fence(nullptr)
    , Device(InDevice)
    , CommandPools()
    , CommandBuffers()
    , QueryPools()
    , DeletionQueue()
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
        FVulkanCommandPool* FixupPool = Queue.ObtainCommandPool();
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
        FVulkanTexture*    Texture    = It.GetKey();
        FVulkanImageLayoutState& LocalState = It.GetValue();
        Texture->GetImageLayoutState() = LocalState;
    }

    for (auto It = PendingBufferStates.CreateIterator(); !It.IsEnd(); ++It)
    {
        FVulkanBuffer*      Buffer     = It.GetKey();
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
}

void FVulkanCommands::Finish()
{
    // Resolve queries
    for (FVulkanQueryPool* QueryPool : QueryPools)
    {
        FVulkanQueryPoolManager* QueryPoolManager = QueryPool->GetQueryPoolManager();
        QueryPool->ResolveQueries();
        QueryPoolManager->RecycleQueryPool(QueryPool);
    }

    QueryPools.Clear();

    // Recycle all the CommandBuffers before CommandPools to avoid needing to lock the CommandPools
    for (FVulkanCommandBuffer* CommandBuffer : CommandBuffers)
    {
        FVulkanCommandPool* CommandPool = CommandBuffer->GetOwnerPool();
        CommandPool->RecycleBuffer(CommandBuffer);
    }

    CommandBuffers.Clear();

    // Recycle all the CommandPool
    for (FVulkanCommandPool* CommandPool : CommandPools)
    {
        Queue.RecycleCommandPool(CommandPool);
    }
    
    CommandPools.Clear();

    // Recycle the fence
    FVulkanFenceManager& FenceManager = Device->GetFenceManager();
    FenceManager.RecycleFence(Fence);
    Fence = nullptr;

    // Delete all the resources that has been queued up for destruction
    FVulkanDeferredObject::ProcessItems(Device, DeletionQueue);
    DeletionQueue.Clear();

    // Destroy this instance after execution is finished
    delete this;
}
