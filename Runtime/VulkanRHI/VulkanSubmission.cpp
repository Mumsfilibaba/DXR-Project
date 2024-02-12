#include "VulkanSubmission.h"

FVulkanCommandPacket::FVulkanCommandPacket(FVulkanDevice* InDevice, FVulkanQueue& InQueue)
    : FVulkanDeviceChild(InDevice)
    , Queue(InQueue)
    , Fence(nullptr)
    , Resources()
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
    for (const FVulkanDeletionQueue::FDeferredResource& Object : Resources)
    {
        switch(Object.Type)
        {
            case FVulkanDeletionQueue::EType::RHIResource:
            {
                CHECK(Object.Resource != nullptr);
                Object.Resource->Release();
                break;
            }
            case FVulkanDeletionQueue::EType::VulkanResource:
            {
                CHECK(Object.VulkanResource != nullptr);
                Object.VulkanResource->Release();
                break;
            }
        }
    }
    
    Resources.Clear();
    
    // Recycle all the commandpool
    for (FVulkanCommandPool* CommandPool : CommandPools)
    {
        Queue.RecycleCommandPool(CommandPool);
    }
    
    CommandPools.Clear();
    
    // Recycle all the commandbuffers
    for (FVulkanCommandBuffer* CommandBuffer : CommandBuffers)
    {
        FVulkanCommandPool* CommandPool = CommandBuffer->GetOwnerPool();
        CommandPool->RecycleBuffer(CommandBuffer);
    }

    CommandBuffers.Clear();
    
    // Destroy this instance after execution is finished
    delete this;
}
