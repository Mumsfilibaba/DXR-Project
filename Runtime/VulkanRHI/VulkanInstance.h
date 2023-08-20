#pragma once
#include "VulkanRefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Platform/PlatformLibrary.h"

struct FVulkanInstanceDesc
{
    TArray<const CHAR*> RequiredExtensionNames;
    TArray<const CHAR*> RequiredLayerNames;
    TArray<const CHAR*> OptionalExtensionNames;
    TArray<const CHAR*> OptionalLayerNames;
};

class FVulkanInstance : public FVulkanRefCounted
{
public:
    FVulkanInstance();
    ~FVulkanInstance();

    bool Initialize(const FVulkanInstanceDesc& InstanceDesc);

    FORCEINLINE bool IsLayerEnabled(const FString& LayerName)
    {
        return LayerNames.find(LayerName) != LayerNames.end();
    }

    FORCEINLINE bool IsExtensionEnabled(const FString& ExtensionName)
    {
        return ExtensionNames.find(ExtensionName) != ExtensionNames.end();
    }

    FORCEINLINE VkInstance GetVkInstance() const noexcept
    {
        return Instance;
    }

private:
    void*      DriverHandle;  
    VkInstance Instance;

#if VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT DebugMessenger;
#endif

    TSet<FString> ExtensionNames;
    TSet<FString> LayerNames;
};
