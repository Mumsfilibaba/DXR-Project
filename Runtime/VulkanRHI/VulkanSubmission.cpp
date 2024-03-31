#include "VulkanSubmission.h"

FVulkanCommandPacket::FVulkanCommandPacket(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : FVulkanDeviceChild(InDevice)
    , Resources()
    , Queue(InQueue)
    , Fence(nullptr)
    , CommandPools()
    , CommandBuffers()
{
}

FVulkanCommandPacket::~FVulkanCommandPacket()
{
    CHECK(Fence == nullptr);
}

void FVulkanCommandPacket::Submit()
{
    FVulkanFenceManager& FenceManager = GetDevice()->GetFenceManager();
    Fence = FenceManager.ObtainFence();
    CHECK(Fence != nullptr);
    CHECK(CommandBuffers.IsEmpty() == false);
    Queue.ExecuteCommandBuffer(CommandBuffers.Data(), CommandBuffers.Size(), Fence);
}

void FVulkanCommandPacket::HandleSubmitFinished()
{
    // Recycle the fence
    FVulkanFenceManager& FenceManager = GetDevice()->GetFenceManager();
    FenceManager.RecycleFence(Fence);
    Fence = nullptr;
    
    // Delete all the resources that has been queued up for destruction
    for (FVulkanDeletionQueue::FDeferredResource& Object : Resources)
    {
        Object.Release();
    }
    
    Resources.Clear();

    // Resolve queries
    for (FVulkanQueryPool* Query : QueryPools)
    {
        Query->ResolveQueries();
    }

    QueryPools.Clear();
    
    // Recycle all the CommandPool
    for (FVulkanCommandPool* CommandPool : CommandPools)
    {
        Queue.RecycleCommandPool(CommandPool);
    }
    
    CommandPools.Clear();
    
    // Recycle all the CommandBuffers
    for (FVulkanCommandBuffer* CommandBuffer : CommandBuffers)
    {
        FVulkanCommandPool* CommandPool = CommandBuffer->GetOwnerPool();
        CommandPool->RecycleBuffer(CommandBuffer);
    }

    CommandBuffers.Clear();
    
    // Destroy this instance after execution is finished
    delete this;
}
