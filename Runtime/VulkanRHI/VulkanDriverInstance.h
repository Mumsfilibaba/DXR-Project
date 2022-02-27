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
	
    FORCEINLINE VkInstance GetVkInstance() const noexcept
    {
        return Instance;
    }

private:
     
    CVulkanDriverInstance();
    ~CVulkanDriverInstance();

	bool Initialize(const SVulkanDriverInstanceDesc& InstanceDesc);

    DynamicLibraryHandle DriverHandle;  
    VkInstance           Instance;

#if VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT DebugMessenger;
#endif

    TSet<String> ExtensionNames;
    TSet<String> LayerNames;
};
