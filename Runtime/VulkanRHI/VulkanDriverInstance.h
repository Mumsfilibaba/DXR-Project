#pragma once
#include "VulkanCore.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Modules/Platform/PlatformLibrary.h"

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanDriverInstanceDesc

struct SVulkanDriverInstanceDesc
{
    TArray<const char*> RequiredExtensionNames;
    TArray<const char*> RequiredLayerNames;
    TArray<const char*> OptionalExtensionNames;
    TArray<const char*> OptionalLayerNames;

    bool bEnableValidationLayer = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDriverInstance

class CVulkanDriverInstance : public CRefCounted
{
public:

    /* Create a new DriverInstance (i.e VkInstance) */
    static TSharedRef<CVulkanDriverInstance> CreateInstance(const SVulkanDriverInstanceDesc& InstanceDesc) noexcept;

    FORCEINLINE bool IsLayerEnabled(const String& LayerName)
    {
        return LayerNames.find(LayerName) != LayerNames.end();
    }

    FORCEINLINE bool IsExtensionEnabled(const String& ExtensionName)
    {
        return ExtensionNames.find(ExtensionName) != ExtensionNames.end();
    }

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

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugLayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT MessageType, 
        const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, 
        void* UserData);

	bool Initialize(const SVulkanDriverInstanceDesc& InstanceDesc);

    PlatformLibrary::PlatformHandle DriverHandle;
	PFN_vkGetInstanceProcAddr       GetInstanceProcAddrFunc;
    
    VkInstance Instance;

#if VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT DebugMessenger;
#endif

    TSet<String> ExtensionNames;
    TSet<String> LayerNames;
};
