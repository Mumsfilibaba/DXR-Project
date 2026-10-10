#include "VulkanRHI/VulkanSemaphore.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanDeviceDebug.h"

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
    VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
    SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    SemaphoreCreateInfo.pNext = nullptr;
    SemaphoreCreateInfo.flags = 0;

    VkResult Result = vkCreateSemaphore(GetDevice()->GetVkDevice(), &SemaphoreCreateInfo, nullptr, &Semaphore);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Semaphore");
        return false;
    }

    return true;
}

bool FVulkanSemaphore::SetDebugName(const FString& Name)
{
    VkResult Result = VulkanSetObjectName(GetDevice()->GetVkDevice(), *Name, Semaphore, VK_OBJECT_TYPE_SEMAPHORE);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkSetDebugUtilsObjectNameEXT failed");
        return false;
    }

#if VULKAN_STORE_DEBUG_NAMES
    DebugName = Name;
#endif
    return true;
}

void FVulkanSemaphore::GetDebugName(FString& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}
