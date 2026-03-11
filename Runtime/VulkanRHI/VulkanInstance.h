#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Platform/PlatformLibrary.h"
#include "VulkanRHI/VulkanRefCounted.h"
#include "VulkanRHI/VulkanExtensions.h"

struct FVulkanInstanceCreateInfo
{
    TArray<const CHAR*> RequiredLayerNames;
    TArray<const CHAR*> OptionalLayerNames;
    TArray<TUniquePtr<FVulkanInstanceExtension>> Extensions;
};

class FVulkanInstance
{
public:
    FVulkanInstance();
    ~FVulkanInstance();

    bool Initialize(FVulkanInstanceCreateInfo& CreateInfo);
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
    void*         DriverHandle;  
    VkInstance    Instance;
    TSet<FString> ExtensionNames;
    TSet<FString> LayerNames;
};
