#include "VulkanDeletionQueue.h"
#include "VulkanDevice.h"
#include "VulkanRHI.h"

void FVulkanDeletionQueue::FDeferredResource::Release()
{
    switch(Type)
    {
        case FVulkanDeletionQueue::EType::RHIResource:
        {
            CHECK(Resource != nullptr);
            Resource->Release();
            break;
        }
        case FVulkanDeletionQueue::EType::VulkanResource:
        {
            CHECK(VulkanResource != nullptr);
            VulkanResource->Release();
            break;
        }
        case FVulkanDeletionQueue::EType::DescriptorPool:
        {
            CHECK(DescriptorPool != nullptr);
            // TODO: Move this to the FVulkanRHI
            FVulkanRHI::GetRHI()->GetDevice()->GetDescriptorPoolManager().RecyclePool(DescriptorPool);
            break;
        }
    }
}
