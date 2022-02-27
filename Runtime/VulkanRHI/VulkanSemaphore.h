#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSemaphore

class CVulkanSemaphore : public CVulkanDeviceObject
{
public:

    inline CVulkanSemaphore(CVulkanDevice* InDevice)
        : CVulkanDeviceObject(InDevice)
        , Semaphore(VK_NULL_HANDLE)
    {
    }

    inline CVulkanSemaphore(CVulkanSemaphore&& Other)
        : CVulkanDeviceObject(Other.GetDevice())
        , Semaphore(Other.Semaphore)
    {
        Other.Semaphore = VK_NULL_HANDLE;
    }

    inline ~CVulkanSemaphore()
    {
        if (VULKAN_CHECK_HANDLE(Semaphore))
        {
            vkDestroySemaphore(GetDevice()->GetVkDevice(), Semaphore, nullptr);
            Semaphore = VK_NULL_HANDLE;
        }
    }

    inline bool Initialize()
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

    inline bool SetName(const String& Name)
    {
#if VK_EXT_debug_utils
        VkDebugUtilsObjectNameInfoEXT DebugUtilsObjectNameInfo;
        CMemory::Memzero(&DebugUtilsObjectNameInfo);

        DebugUtilsObjectNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugUtilsObjectNameInfo.pNext        = nullptr;
        DebugUtilsObjectNameInfo.pObjectName  = Name.CStr();
        DebugUtilsObjectNameInfo.objectHandle = reinterpret_cast<uint64>(Semaphore);
        DebugUtilsObjectNameInfo.objectType   = VK_OBJECT_TYPE_SEMAPHORE;

        VkResult Result = vkSetDebugUtilsObjectNameEXT(GetDevice()->GetVkDevice(), &DebugUtilsObjectNameInfo);
        VULKAN_CHECK_RESULT(Result, "vkSetDebugUtilsObjectNameEXT failed");

        return true;
#else
        return false;
#endif
    }

	FORCEINLINE VkSemaphore GetVkSemaphore() const
	{
		return Semaphore;
	}
	
private:
    VkSemaphore Semaphore;
};
