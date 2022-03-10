#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"

// Undefine Windows-macros
#ifdef CreateSemaphore
    #undef CreateSemaphore
#endif

typedef TSharedRef<class CVulkanSemaphore> CVulkanSemaphoreRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSemaphore

class CVulkanSemaphore : public CVulkanDeviceObject, public CRefCounted
{
public:

    /** Create a new semaphore */
    static CVulkanSemaphoreRef CreateSemaphore(CVulkanDevice* InDevice);

    bool SetName(const String& Name);

    FORCEINLINE VkSemaphore GetVkSemaphore() const
    {
        return Semaphore;
    }
    
private:
    CVulkanSemaphore(CVulkanDevice* InDevice);
    ~CVulkanSemaphore();

    bool Initialize();

    VkSemaphore Semaphore;
};
