#include "VulkanSubmission.h"

FVulkanCommandPayload::FVulkanCommandPayload(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : Queue(InQueue)
    , Device(InDevice)
    , Fence(nullptr)
    , DeletionQueue()
    , CommandPools()
    , CommandBuffers()
{
}

FVulkanCommandPayload::~FVulkanCommandPayload()
{
    CHECK(Fence == nullptr);
}

void FVulkanCommandPayload::Submit()
{
    FVulkanFenceManager& FenceManager = Device->GetFenceManager();
    Fence = FenceManager.ObtainFence();
    CHECK(Fence != nullptr);
    CHECK(CommandBuffers.IsEmpty() == false);
    Queue.ExecuteCommandBuffer(CommandBuffers.Data(), CommandBuffers.Size(), Fence);
}

void FVulkanCommandPayload::Finish()
{
    // Delete all the resources that has been queued up for destruction
    FVulkanDeferredObject::ProcessItems(DeletionQueue);
    DeletionQueue.Clear();

    // Resolve queries
    for (FVulkanQueryPool* Query : QueryPools)
        Query->ResolveQueries();

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
        Queue.RecycleCommandPool(CommandPool);
    
    CommandPools.Clear();

    // Recycle the fence
    FVulkanFenceManager& FenceManager = Device->GetFenceManager();
    FenceManager.RecycleFence(Fence);
    Fence = nullptr;
    
    // Destroy this instance after execution is finished
    delete this;
}
