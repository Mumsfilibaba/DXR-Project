#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Platform/PlatformLibrary.h"
#include "VulkanRHI/VulkanRefCounted.h"

struct FVulkanInstanceCreateInfo
{
    TArray<const CHAR*> RequiredExtensionNames;
    TArray<const CHAR*> RequiredLayerNames;
    TArray<const CHAR*> OptionalExtensionNames;
    TArray<const CHAR*> OptionalLayerNames;
};

class FVulkanInstance
{
public:
    FVulkanInstance();
    ~FVulkanInstance();

    bool Initialize(const FVulkanInstanceCreateInfo& InstanceDesc);
    void Release();

    bool IsLayerEnabled(const FString& LayerName)
    {
        return LayerNames.Find(LayerName) != nullptr;
    }

    bool IsExtensionEnabled(const FString& ExtensionName)
    {
        return ExtensionNames.Find(ExtensionName) != nullptr;
    }

    VkInstance GetVkInstance() const noexcept
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
