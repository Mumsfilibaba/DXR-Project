#include "VulkanSemaphore.h"
#include "VulkanDevice.h"

FVulkanSemaphore::FVulkanSemaphore(FVulkanDevice* InDevice)
    : FVulkanDeviceObject(InDevice)
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
    VULKAN_CHECK_RESULT(Result, "Failed to create Semaphore");
    return true;
}

bool FVulkanSemaphore::SetName(const FString& Name)
{
    VkResult Result = FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), Name.GetCString(), Semaphore, VK_OBJECT_TYPE_SEMAPHORE);
    VULKAN_CHECK_RESULT(Result, "vkSetDebugUtilsObjectNameEXT failed");
    return true;
}
