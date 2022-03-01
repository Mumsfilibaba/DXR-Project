#include "VulkanSemaphore.h"
#include "VulkanDevice.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSemaphore

CVulkanSemaphoreRef CVulkanSemaphore::CreateSemaphore(CVulkanDevice* InDevice)
{
    CVulkanSemaphoreRef NewSemaphore = dbg_new CVulkanSemaphore(InDevice);
    if (NewSemaphore && NewSemaphore->Initialize())
    {
        return NewSemaphore;
    }

    return nullptr;
}

CVulkanSemaphore::CVulkanSemaphore(CVulkanDevice* InDevice)
    : CVulkanDeviceObject(InDevice)
    , Semaphore(VK_NULL_HANDLE)
{
}

CVulkanSemaphore::~CVulkanSemaphore()
{
    if (VULKAN_CHECK_HANDLE(Semaphore))
    {
        vkDestroySemaphore(GetDevice()->GetVkDevice(), Semaphore, nullptr);
        Semaphore = VK_NULL_HANDLE;
    }
}

bool CVulkanSemaphore::Initialize()
{
    VkSemaphoreCreateInfo SemaphoreCreateInfo;
    CMemory::Memzero(&SemaphoreCreateInfo);

    SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    SemaphoreCreateInfo.pNext = nullptr;
    SemaphoreCreateInfo.flags = 0;

    VkResult Result = vkCreateSemaphore(GetDevice()->GetVkDevice(), &SemaphoreCreateInfo, nullptr, &Semaphore);
    VULKAN_CHECK_RESULT(Result, "Failed to create Semaphore");

    return true;
}

bool CVulkanSemaphore::SetName(const String& Name)
{
    VkResult Result = CVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), Name.CStr(), Semaphore, VK_OBJECT_TYPE_SEMAPHORE);
    VULKAN_CHECK_RESULT(Result, "vkSetDebugUtilsObjectNameEXT failed");

    return true;
}