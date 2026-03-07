#pragma once
#include "VulkanRHI/VulkanCore.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/String.h"
#include "Core/Templates/Utility/NonCopyable.h"

class FVulkanInstance;
class FVulkanDevice;

class VULKANRHI_API FVulkanExtensionBase
{
public:
    virtual ~FVulkanExtensionBase() = default;

    virtual bool IsRequired()   const { return false; }
    virtual bool ShouldEnable() const { return true; }
    
    virtual const CHAR* GetName() const = 0;

    bool IsEnabled() const
    {
        return bEnabled;
    }

    void SetEnabled(bool bInEnabled)
    {
        bEnabled = bInEnabled;
    }

private:
    bool bEnabled = false;
};

struct VULKANRHI_API FVulkanInstanceExtension : public FVulkanExtensionBase
{
    virtual bool LoadFunctions(FVulkanInstance* Instance) { return true; }
};

struct VULKANRHI_API FVulkanDeviceExtension : public FVulkanExtensionBase
{
    virtual bool LoadFunctions(FVulkanDevice* Device) { return true; }
    
    virtual void AddToFeatureQueryChain(FVulkanStructChain& Chain)  { }
    virtual void AddToPropertyQueryChain(FVulkanStructChain& Chain) { }
    virtual void AddToFeatureEnableChain(FVulkanStructChain& Chain) { }

    virtual void ProcessQueriedFeatures() { }
};

class VULKANRHI_API FVulkanExtensionRegistry : public FNonCopyable
{
public:
    FVulkanExtensionRegistry();
    ~FVulkanExtensionRegistry();

    FVulkanExtensionRegistry& AddExtension(FVulkanInstanceExtension* Extension);
    FVulkanExtensionRegistry& AddExtension(FVulkanDeviceExtension* Extension);

    bool ResolveInstanceExtensions(const TArray<VkExtensionProperties>& Available, TArray<const CHAR*>& OutEnabledNames);
    bool ResolveDeviceExtensions(const TArray<VkExtensionProperties>& Available, TArray<const CHAR*>& OutEnabledNames);

    bool LoadInstanceFunctions(FVulkanInstance* Instance);
    bool LoadDeviceFunctions(FVulkanDevice* Device);

    void BuildFeatureQueryChain(FVulkanStructChain& Chain);
    void BuildPropertyQueryChain(FVulkanStructChain& Chain);
    void BuildFeatureEnableChain(FVulkanStructChain& Chain);
    void ProcessQueriedFeatures();

    bool IsDeviceExtensionEnabled(const FString& Name)   const;
    bool IsInstanceExtensionEnabled(const FString& Name) const;

private:
    TSet<FString>                     EnabledInstanceExtensionNames;
    TSet<FString>                     EnabledDeviceExtensionNames;
    TArray<FVulkanInstanceExtension*> InstanceExtensions;
    TArray<FVulkanDeviceExtension*>   DeviceExtensions;
};

