#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanDeletionQueue.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanDevice.h"

void FVulkanDeferredObject::ProcessItems(FVulkanDevice* Device, TArray<FVulkanDeferredObject>& Items)
{
    for (const FVulkanDeferredObject& Item : Items)
    {
        switch(Item.Type)
        {
            case FVulkanDeferredObject::EType::RHIResource:
            {
                CHECK(Item.RHIResource != nullptr);
                delete Item.RHIResource;
                break;
            }
            case FVulkanDeferredObject::EType::VulkanResource:
            {
                CHECK(Item.VulkanResource != nullptr);
                Item.VulkanResource->Release();
                break;
            }
            case FVulkanDeferredObject::EType::BuddyAllocatorBlock:
            {
                CHECK(Item.BuddyAllocatorBlock.Allocator != nullptr);
                Item.BuddyAllocatorBlock.Allocator->RecycleAllocation(Item.BuddyAllocatorBlock.AllocationData);
                break;
            }
            case FVulkanDeferredObject::EType::PoolAllocatorBlock:
            {
                CHECK(Item.PoolAllocatorBlock.Allocator != nullptr);
                Item.PoolAllocatorBlock.Allocator->RecycleAllocation(Item.PoolAllocatorBlock.AllocationData);
                break;
            }
            case FVulkanDeferredObject::EType::DedicatedAllocation:
            {
                if (Item.DedicatedAllocation.Buffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(Device->GetVkDevice(), Item.DedicatedAllocation.Buffer, nullptr);
                }

                if (Item.DedicatedAllocation.Memory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(Device->GetVkDevice(), Item.DedicatedAllocation.Memory, nullptr);
                }

                break;
            }
            case FVulkanDeferredObject::EType::LinearAllocatorPage:
            {
                CHECK(Item.LinearAllocatorPage.Allocator != nullptr);
                CHECK(Item.LinearAllocatorPage.Page != nullptr);
                Item.LinearAllocatorPage.Allocator->ReturnPage(Item.LinearAllocatorPage.Page);
                break;
            }
            case FVulkanDeferredObject::EType::DescriptorPool:
            {
                CHECK(Item.DescriptorPoolData.PoolManager != nullptr);
                CHECK(Item.DescriptorPoolData.Pool != nullptr);
                Item.DescriptorPoolData.PoolManager->ReleasePool(Item.DescriptorPoolData.Pool);
                break;
            }
        }
    }
}
