#include "VulkanDeletionQueue.h"
#include "VulkanDevice.h"
#include "VulkanRHI.h"

void FVulkanDeferredObject::ProcessItems(const TArray<FVulkanDeferredObject>& Items)
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
        }
    }
}
