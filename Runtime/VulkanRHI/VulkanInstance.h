#pragma once
#include "VulkanCore.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Modules/Platform/PlatformLibrary.h"

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanInstanceDesc

struct SVulkanInstanceDesc
{
    TArray<const char*> RequiredExtensionNames;
    TArray<const char*> RequiredLayerNames;
    TArray<const char*> OptionalExtensionNames;
    TArray<const char*> OptionalLayerNames;

    bool bEnableValidationLayer = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanInstance

class CVulkanInstance : public CRefCounted
{
public:

    /* Create a new instance */
    static TSharedRef<CVulkanInstance> CreateInstance(const SVulkanInstanceDesc& InstanceDesc) noexcept;

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
     
    CVulkanInstance();
    ~CVulkanInstance();

    bool Initialize(const SVulkanInstanceDesc& InstanceDesc);

    DynamicLibraryHandle DriverHandle;  
    VkInstance           Instance;

#if VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT DebugMessenger;
#endif

    TSet<String> ExtensionNames;
    TSet<String> LayerNames;
};
