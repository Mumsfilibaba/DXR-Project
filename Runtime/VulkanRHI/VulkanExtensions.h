#pragma once
#include "VulkanRHI/VulkanCore.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Templates/Utility/NonCopyable.h"

class VULKANRHI_API FVulkanExtension
{
public:
    FVulkanExtension(const CHAR* InExtensionName, bool bInRequired, bool bInShouldEnable)
        : ExtensionName(InExtensionName)
        , bRequired(bInRequired)
        , bEnabled(false)
        , bShouldEnable(bInShouldEnable)
    {
    }

    virtual ~FVulkanExtension() = default;

    FORCEINLINE const CHAR* GetExtensionName() const
    {
        return ExtensionName;
    }

    FORCEINLINE bool IsRequired() const
    {
        return bRequired;
    }

    FORCEINLINE bool ShouldEnable() const
    {
        return bShouldEnable;
    }

    FORCEINLINE bool IsEnabled() const
    {
        return bEnabled;
    }

    FORCEINLINE void SetEnabled(bool bInEnabled)
    {
        bEnabled = bInEnabled;
    }

private:
    const CHAR* ExtensionName;

    bool bRequired : 1;
    bool bEnabled : 1;
    bool bShouldEnable : 1;
};

struct VULKANRHI_API FVulkanInstanceExtension : public FVulkanExtension
{
public:
    static void RegisterExtensions(TArray<TUniquePtr<FVulkanInstanceExtension>>& OutExtensions);

public:
    FVulkanInstanceExtension(const CHAR* InName, bool bInRequired, bool bInShouldEnable)
        : FVulkanExtension(InName, bInRequired, bInShouldEnable)
    {
    }

    virtual void PrepareInstanceCreateInfo(VkInstanceCreateInfo&) { }

};

struct VULKANRHI_API FVulkanDeviceExtension : public FVulkanExtension
{
public:
    static void RegisterExtensions(TArray<TUniquePtr<FVulkanDeviceExtension>>& OutExtensions);
    
public:
    FVulkanDeviceExtension(const CHAR* InName, bool bInRequired, bool bInShouldEnable)
        : FVulkanExtension(InName, bInRequired, bInShouldEnable)
    {
    }

    virtual void PrepareDeviceFeatures(VkPhysicalDeviceFeatures2&)       { }
    virtual void PrepareDeviceProperties(VkPhysicalDeviceProperties2&) { }
    virtual void PrepareDeviceCreateInfo(VkDeviceCreateInfo&)    { }
    virtual void ProcessQueriedFeatures() { }
};

#if VK_KHR_portability_enumeration
class FVulkanKHRPortabilityEnumerationExtension : public FVulkanInstanceExtension
{
public:
    FVulkanKHRPortabilityEnumerationExtension(bool bInRequired, bool bInShouldEnable)
        : FVulkanInstanceExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, bInRequired, bInShouldEnable)
    {
    }

    virtual void PrepareInstanceCreateInfo(VkInstanceCreateInfo& OutInstanceCreateInfo) override final
    {
        OutInstanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
};
#endif

