#include "VulkanRHI/VulkanSemaphore.h"
#include "VulkanRHI/VulkanDevice.h"

FVulkanSemaphore::FVulkanSemaphore(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Semaphore(VK_NULL_HANDLE)
{
}

FVulkanSemaphore::~FVulkanSemaphore()
{
    if (VULKAN_CHECK_HANDLE(Semaphore))
    {
        vkDestroySemaphore(GetDevice()->GetVkDevice(), Semaphore, nullptr);
        Semaphore = VK_NULL_HANDLE;
    }
}

bool FVulkanSemaphore::Initialize()
{
    VkSemaphoreCreateInfo SemaphoreCreateInfo;
    FMemory::Memzero(&SemaphoreCreateInfo);

    SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    SemaphoreCreateInfo.pNext = nullptr;
    SemaphoreCreateInfo.flags = 0;

    VkResult Result = vkCreateSemaphore(GetDevice()->GetVkDevice(), &SemaphoreCreateInfo, nullptr, &Semaphore);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Semaphore");
        return false;
    }

    return true;
}

bool FVulkanSemaphore::SetDebugName(const FString& Name)
{
    VkResult Result = FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *Name, Semaphore, VK_OBJECT_TYPE_SEMAPHORE);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkSetDebugUtilsObjectNameEXT failed");
        return false;
    }

    return true;
}
