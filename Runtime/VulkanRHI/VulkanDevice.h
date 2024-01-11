#pragma once
#include "VulkanCore.h"
#include "VulkanLoader.h"
#include "VulkanAllocators.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanMemory.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Optional.h"
#include "Core/Threading/AtomicInt.h"

class FVulkanInstance;
class FVulkanPhysicalDevice;

////////////////////////////////////////////////////
// Global variables that describe different features

extern VULKANRHI_API bool GVulkanForceBinding;
extern VULKANRHI_API bool GVulkanForceDedicatedAllocations;
extern VULKANRHI_API bool GVulkanAllowNullDescriptors;


enum class EVulkanCommandQueueType
{
    Unknown  = 0, 
    Graphics = 1, 
    Copy     = 2, 
    Compute  = 3, 
};

struct FVulkanDeviceDesc
{
    FVulkanDeviceDesc()
        : RequiredExtensionNames()
        , OptionalExtensionNames()
        , RequiredFeatures()
    {
        FMemory::Memzero(&RequiredFeatures);
        FMemory::Memzero(&RequiredFeatures11);
        FMemory::Memzero(&RequiredFeatures12);
    }

    TArray<const CHAR*> RequiredExtensionNames;
    TArray<const CHAR*> OptionalExtensionNames;

    VkPhysicalDeviceFeatures         RequiredFeatures;
    VkPhysicalDeviceVulkan11Features RequiredFeatures11;
    VkPhysicalDeviceVulkan12Features RequiredFeatures12;
};


class FVulkanDevice : public FVulkanRefCounted
{
public:
    FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter);
    ~FVulkanDevice();

    bool Initialize(const FVulkanDeviceDesc& DeviceDesc);

    uint32 GetCommandQueueIndexFromType(EVulkanCommandQueueType Type) const;
    
    FVulkanRenderPassCache&  GetRenderPassCache()  { return RenderPassCache; }
    FVulkanFramebufferCache& GetFramebufferCache() { return FramebufferCache; }

    FVulkanMemoryManager&       GetMemoryManager() { return MemoryManager; }
    FVulkanUploadHeapAllocator& GetUploadHeap()    { return UploadHeap; };

    bool IsDepthClipSupported()                 const { return bSupportsDepthClip; }
    bool IsConservativeRasterizationSupported() const { return bSupportsConservativeRasterization; }
    
    bool IsLayerEnabled(const FString& LayerName)
    {
        return LayerNames.find(LayerName) != LayerNames.end();
    }

    bool IsExtensionEnabled(const FString& ExtensionName)
    {
        return ExtensionNames.find(ExtensionName) != ExtensionNames.end();
    }

    FVulkanInstance* GetInstance() const
    {
        return Instance;
    }
    
    FVulkanPhysicalDevice* GetPhysicalDevice() const
    {
        return PhysicalDevice;
    }

    VkDevice GetVkDevice() const
    {
        return Device;
    }

    TOptional<FVulkanQueueFamilyIndices> GetQueueIndicies() const
    {
        return QueueIndicies;
    }

private:
    FVulkanInstance*       Instance;
    FVulkanPhysicalDevice* PhysicalDevice;
    VkDevice               Device;
    
    FVulkanRenderPassCache     RenderPassCache;
    FVulkanFramebufferCache    FramebufferCache;
    FVulkanUploadHeapAllocator UploadHeap;
    FVulkanMemoryManager       MemoryManager;

    TOptional<FVulkanQueueFamilyIndices> QueueIndicies;
    
    TSet<FString> ExtensionNames;
    TSet<FString> LayerNames;
    
    // One off boolean features
    bool bSupportsDepthClip                 : 1;
    bool bSupportsConservativeRasterization : 1;
};
