#pragma once
#include "VulkanCore.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Modules/Platform/PlatformLibrary.h"

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDriverInstance

class CVulkanDriverInstance : public CRefCounted
{
public:

    /* Create a new DriverInstance (i.e VkInstance) */
    static TSharedRef<CVulkanDriverInstance> CreateInstance() noexcept;

	bool Initialize(const TArray<const char*>& InstanceExtensionNames, const TArray<const char*>& InstanceLayerNames);
	
	FORCEINLINE VulkanVoidFunction LoadFunction(const char* Name) const noexcept
	{
		VULKAN_ERROR(GetInstanceProcAddrFunc != nullptr, "Vulkan Driver Instance is not initialized properly");
		return reinterpret_cast<VulkanVoidFunction>(GetInstanceProcAddrFunc(Instance, Name));
	}

    template<typename FunctionType>
    FORCEINLINE FunctionType LoadFunction(const char* Name) const noexcept
	{
		return reinterpret_cast<FunctionType>(LoadFunction(Name));
	}
	
    FORCEINLINE VkInstance GetInstance() const noexcept
    {
        return Instance;
    }

private:
    CVulkanDriverInstance();
    ~CVulkanDriverInstance();

    bool Load();

    PlatformLibrary::PlatformHandle DriverHandle;
	PFN_vkGetInstanceProcAddr       GetInstanceProcAddrFunc;
    
    VkInstance Instance;
};
