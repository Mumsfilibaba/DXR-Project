#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Platform/PlatformLibrary.h"
#include "VulkanRHI/VulkanExtensions.h"

struct FVulkanInstanceCreateInfo
{
    TArray<const ANSICHAR*>                      RequiredLayerNames;
    TArray<const ANSICHAR*>                      OptionalLayerNames;
    TArray<TUniquePtr<FVulkanInstanceExtension>> Extensions;
};

class FVulkanInstance
{
public:
    FVulkanInstance();
    ~FVulkanInstance();

    bool Initialize(FVulkanInstanceCreateInfo& CreateInfo);
    void Release();

    bool IsLayerEnabled(const FAnsiString& LayerName)
    {
        return LayerNames.Find(LayerName) != nullptr;
    }

    bool IsExtensionEnabled(const FAnsiString& ExtensionName)
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
    TSet<FAnsiString> ExtensionNames;
    TSet<FAnsiString> LayerNames;
};
