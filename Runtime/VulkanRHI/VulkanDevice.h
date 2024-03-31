#pragma once
#include "VulkanCore.h"
#include "VulkanLoader.h"
#include "VulkanAllocators.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanMemory.h"
#include "VulkanFenceManager.h"
#include "VulkanPipelineLayout.h"
#include "VulkanDescriptorSet.h"
#include "VulkanPipelineState.h"
#include "VulkanDescriptorSetCache.h"
#include "VulkanQuery.h"
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

struct FVulkanDeviceCreateInfo
{
    FVulkanDeviceCreateInfo()
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

struct FVulkanDefaultResources
{
    FVulkanDefaultResources()
        : NullBuffer(VK_NULL_HANDLE)
        , NullImage(VK_NULL_HANDLE)
        , NullImageView(VK_NULL_HANDLE)
        , NullSampler(VK_NULL_HANDLE)
    {
    }
    
    ~FVulkanDefaultResources()
    {
        CHECK(NullBuffer    == VK_NULL_HANDLE);
        CHECK(NullImage     == VK_NULL_HANDLE);
        CHECK(NullImageView == VK_NULL_HANDLE);
        CHECK(NullSampler   == VK_NULL_HANDLE);
    }
    
    bool Initialize(FVulkanDevice& Device);
    bool InitializeBuffersAndImages(FVulkanDevice& Device);
    void Release(FVulkanDevice& Device);
    
    // Null-Buffer
    VkBuffer                NullBuffer;
    FVulkanMemoryAllocation NullBufferMemory;
    
    // Null-Image
    VkImage                 NullImage;
    VkImageView             NullImageView;
    FVulkanMemoryAllocation NullImageMemory;
    
    // NullSampler
    VkSampler               NullSampler;
};

struct FVulkanHashableSamplerCreateInfo
{
    bool operator==(const FVulkanHashableSamplerCreateInfo& Other) const
    {
        return FMemory::Memcmp(this, &Other, sizeof(FVulkanHashableSamplerCreateInfo)) == 0;
    }

    bool operator!=(const FVulkanHashableSamplerCreateInfo& Other) const
    {
        return FMemory::Memcmp(this, &Other, sizeof(FVulkanHashableSamplerCreateInfo)) != 0;
    }

    friend uint64 HashType(const FVulkanHashableSamplerCreateInfo& Value)
    {
        return FCRC32::Generate(&Value, sizeof(Value));
    }

    VkSamplerCreateFlags Flags;
    VkFilter             MagFilter;
    VkFilter             MinFilter;
    VkSamplerMipmapMode  MipmapMode;
    VkSamplerAddressMode AddressModeU;
    VkSamplerAddressMode AddressModeV;
    VkSamplerAddressMode AddressModeW;
    float                MipLodBias;
    VkBool32             AnisotropyEnable;
    float                MaxAnisotropy;
    VkBool32             CompareEnable;
    VkCompareOp          CompareOp;
    float                MinLod;
    float                MaxLod;
    VkBorderColor        BorderColor;
    VkBool32             UnnormalizedCoordinates;
};

class FVulkanDevice
{
public:
    FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter);
    ~FVulkanDevice();

    // Initialize the device, query device properties and load functions that require a device
    bool Initialize(const FVulkanDeviceCreateInfo& DeviceDesc);

    // Initialize systems that needs an initialized device, but is a part of the device object
    bool PostLoaderInitalize();

    // Initialize default resources that are just for null bindings
    bool InitializeDefaultResources(class FVulkanCommandContext& CommandContext);

    // Create or returns an already created sampler, this is to avoid creating duplicate samplers
    bool FindOrCreateSampler(const VkSamplerCreateInfo& SamplerCreateInfo, VkSampler& OutSampler);

    FVulkanRenderPassCache&       GetRenderPassCache()       { return RenderPassCache; }
    FVulkanFramebufferCache&      GetFramebufferCache()      { return FramebufferCache; }
    FVulkanMemoryManager&         GetMemoryManager()         { return MemoryManager; }
    FVulkanUploadHeapAllocator&   GetUploadHeap()            { return UploadHeap; }
    FVulkanFenceManager&          GetFenceManager()          { return FenceManager; }
    FVulkanPipelineLayoutManager& GetPipelineLayoutManager() { return PipelineLayoutManager; }
    FVulkanPipelineCache&         GetPipelineCache()         { return PipelineCache; }
    FVulkanDescriptorSetCache&    GetDescriptorSetCache()    { return DescriptorSetCache; }
    FVulkanDefaultResources&      GetDefaultResources()      { return DefaultResources; }
    FVulkanQueryPoolManager&      GetQueryPoolManager()      { return QueryPoolManager; }

    uint32 GetQueueIndexFromType(EVulkanCommandQueueType Type) const;

    bool IsDepthClipSupported()                 const { return bSupportsDepthClip; }
    bool IsConservativeRasterizationSupported() const { return bSupportsConservativeRasterization; }
    bool IsPipelineCacheControlSupported()      const { return bSupportsPipelineCacheControl; }
    
    bool IsLayerEnabled(const FString& LayerName)
    {
        return LayerNames.Find(LayerName) != nullptr;
    }

    bool IsExtensionEnabled(const FString& ExtensionName)
    {
        return ExtensionNames.Find(ExtensionName) != nullptr;
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

    FVulkanRenderPassCache       RenderPassCache;
    FVulkanFramebufferCache      FramebufferCache;
    FVulkanUploadHeapAllocator   UploadHeap;
    FVulkanMemoryManager         MemoryManager;
    FVulkanFenceManager          FenceManager;
    FVulkanPipelineLayoutManager PipelineLayoutManager;
    FVulkanPipelineCache         PipelineCache;
    FVulkanDescriptorSetCache    DescriptorSetCache;
    FVulkanDefaultResources      DefaultResources;
    FVulkanQueryPoolManager      QueryPoolManager;

    TOptional<FVulkanQueueFamilyIndices> QueueIndicies;

    TMap<FVulkanHashableSamplerCreateInfo, VkSampler> SamplerMap;
    FCriticalSection SamplerMapCS;

    TSet<FString> ExtensionNames;
    TSet<FString> LayerNames;

    bool bSupportsDepthClip                 : 1;
    bool bSupportsConservativeRasterization : 1;
    bool bSupportsPipelineCacheControl      : 1;
};
