#pragma once
#include "VulkanCore.h"
#include "VulkanLoader.h"
#include "VulkanAllocators.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanMemory.h"
#include "VulkanFenceManager.h"
#include "VulkanPipelineLayout.h"
#include "VulkanDescriptorSet.h"
#include "VulkanPipelineState.h"
#include "VulkanQuery.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Optional.h"
#include "Core/Threading/AtomicInt.h"
#include "Core/Containers/String.h"

class FVulkanInstance;
class FVulkanPhysicalDevice;

////////////////////////////////////////////////////
// Global variables that describe different features

extern VULKANRHI_API bool GVulkanForceBinding;
extern VULKANRHI_API bool GVulkanForceDedicatedAllocations;
extern VULKANRHI_API bool GVulkanForceDedicatedImageAllocations;
extern VULKANRHI_API bool GVulkanForceDedicatedBufferAllocations;
extern VULKANRHI_API bool GVulkanAllowNullDescriptors;


enum class EVulkanCommandQueueType
{
    Unknown  = 0, 
    Graphics = 1, 
    Copy     = 2, 
    Compute  = 3, 
};

struct FVulkanPhysicalDeviceCreateInfo
{
    FVulkanPhysicalDeviceCreateInfo()
        : RequiredExtensionNames()
        , OptionalExtensionNames()
    {
        FMemory::Memzero(&RequiredFeatures);
        FMemory::Memzero(&RequiredFeatures11);
        FMemory::Memzero(&RequiredFeatures12);
    }

    TArray<const CHAR*> RequiredExtensionNames;
    TArray<const CHAR*> OptionalExtensionNames; // Used to select most optimal adapter

    VkPhysicalDeviceFeatures         RequiredFeatures;
    VkPhysicalDeviceVulkan11Features RequiredFeatures11;
    VkPhysicalDeviceVulkan12Features RequiredFeatures12;
    
    // TODO: Optional features that can be used so select the best adapter
};

struct FVulkanQueueFamilyIndices
{
    FVulkanQueueFamilyIndices() = default;

    FVulkanQueueFamilyIndices(uint32 InGraphicsQueueIndex, uint32 InCopyQueueIndex, uint32 InComputeQueueIndex)
        : GraphicsQueueIndex(InGraphicsQueueIndex)
        , CopyQueueIndex(InCopyQueueIndex)
        , ComputeQueueIndex(InComputeQueueIndex)
    {
    }

    uint32 GraphicsQueueIndex = uint32(~0);
    uint32 CopyQueueIndex     = uint32(~0);
    uint32 ComputeQueueIndex  = uint32(~0);
};

class FVulkanPhysicalDevice
{
public:
    static TOptional<FVulkanQueueFamilyIndices> GetQueueFamilyIndices(VkPhysicalDevice physicalDevice);

    FVulkanPhysicalDevice(FVulkanInstance* InInstance);
    ~FVulkanPhysicalDevice();

    bool Initialize(const FVulkanPhysicalDeviceCreateInfo& AdapterDesc);
    uint32 FindMemoryTypeIndex(uint32 TypeFilter, VkMemoryPropertyFlags Properties);
    VkFormatProperties GetFormatProperties(VkFormat Format) const;
    
    const VkPhysicalDeviceProperties& GetProperties() const { return DeviceProperties; }
    const VkPhysicalDeviceFeatures& GetFeatures() const { return DeviceFeatures; }
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties()  const { return DeviceMemoryProperties; }
    // Vulkan 1.1, Vulkan 1.2 features
    const VkPhysicalDeviceProperties2& GetProperties2() const { return DeviceProperties2; }
    const VkPhysicalDeviceFeatures2& GetFeatures2() const { return DeviceFeatures2; }
    const VkPhysicalDeviceMemoryProperties2& GetMemoryProperties2() const { return DeviceMemoryProperties2; }
    const VkPhysicalDeviceVulkan11Features& GetFeaturesVulkan11() const { return DeviceFeatures11; }
    const VkPhysicalDeviceVulkan12Features& GetFeaturesVulkan12() const { return DeviceFeatures12; }

    // Extension Information
#if VK_EXT_depth_clip_enable
    const VkPhysicalDeviceDepthClipEnableFeaturesEXT& GetDepthClipEnableFeatures() const { return DepthClipEnableFeatures; }
#endif
#if VK_EXT_robustness2
    const VkPhysicalDeviceRobustness2FeaturesEXT& GetRobustness2Features() const { return Robustness2Features; }
#endif
#if VK_EXT_conservative_rasterization
    const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& GetConservativeRasterizationProperties() const { return ConservativeRasterizationProperties; }
#endif
#if VK_EXT_pipeline_creation_cache_control
    const VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT& GetPipelineCreationCacheControlFeatures() const { return PipelineCreationCacheControlFeatures; }
#endif
#if VK_KHR_acceleration_structure
    const VkPhysicalDeviceAccelerationStructureFeaturesKHR& GetAccelerationStructureFeatures() const { return AccelerationStructureFeatures; }
#endif
#if VK_KHR_ray_tracing_pipeline
    const VkPhysicalDeviceRayTracingPipelineFeaturesKHR& GetRayTracingPipelineFeatures() const { return RayTracingPipelineFeatures; }
#endif

    FVulkanInstance* GetInstance() const
    {
        return Instance;
    }

    VkPhysicalDevice GetVkPhysicalDevice() const
    {
        return PhysicalDevice;
    }

private:
    FVulkanInstance*                  Instance;
    VkPhysicalDevice                  PhysicalDevice;
    
    VkPhysicalDeviceProperties        DeviceProperties;
    VkPhysicalDeviceFeatures          DeviceFeatures;
    VkPhysicalDeviceMemoryProperties  DeviceMemoryProperties;

    // Vulkan 1.1, Vulkan 1.2 features
    VkPhysicalDeviceProperties2       DeviceProperties2;
    VkPhysicalDeviceFeatures2         DeviceFeatures2;
    VkPhysicalDeviceMemoryProperties2 DeviceMemoryProperties2;
    VkPhysicalDeviceVulkan11Features  DeviceFeatures11;
    VkPhysicalDeviceVulkan12Features  DeviceFeatures12;

    // Extension Information
#if VK_EXT_depth_clip_enable
    VkPhysicalDeviceDepthClipEnableFeaturesEXT DepthClipEnableFeatures;
#endif
#if VK_EXT_robustness2
    VkPhysicalDeviceRobustness2FeaturesEXT Robustness2Features;
#endif
#if VK_EXT_conservative_rasterization
    VkPhysicalDeviceConservativeRasterizationPropertiesEXT ConservativeRasterizationProperties;
#endif
#if VK_EXT_pipeline_creation_cache_control
    VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT PipelineCreationCacheControlFeatures;
#endif
#if VK_KHR_acceleration_structure
    VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures;
#endif
#if VK_KHR_ray_tracing_pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeatures;
#endif
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

    friend uint64 GetHashForType(const FVulkanHashableSamplerCreateInfo& Value)
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

    FVulkanRenderPassCache& GetRenderPassCache() { return RenderPassCache; }
    FVulkanFramebufferCache& GetFramebufferCache() { return FramebufferCache; }
    FVulkanMemoryManager& GetMemoryManager() { return MemoryManager; }
    FVulkanUploadHeapAllocator& GetUploadHeap() { return UploadHeap; }
    FVulkanFenceManager& GetFenceManager() { return FenceManager; }
    FVulkanPipelineLayoutManager& GetPipelineLayoutManager() { return PipelineLayoutManager; }
    FVulkanPipelineStateManager& GetPipelineStateManager() { return PipelineStateManager; }
    FVulkanDescriptorSetCache& GetDescriptorSetCache() { return DescriptorSetCache; }
    FVulkanDefaultResources& GetDefaultResources() { return DefaultResources; }
    FVulkanQueryPoolManager& GetQueryPoolManager() { return QueryPoolManager; }

    uint32 GetQueueIndexFromType(EVulkanCommandQueueType Type) const;

    bool IsDepthClipSupported()                 const { return bSupportsDepthClip; }
    bool IsConservativeRasterizationSupported() const { return bSupportsConservativeRasterization; }
    bool IsPipelineCacheControlSupported()      const { return bSupportsPipelineCacheControl; }
    bool IsAccelerationStructuresSupported()    const { return bSupportsAccelerationStructures; }
    
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
    FVulkanInstance*             Instance;
    FVulkanPhysicalDevice*       PhysicalDevice;
    VkDevice                     Device;
    FVulkanRenderPassCache       RenderPassCache;
    FVulkanFramebufferCache      FramebufferCache;
    FVulkanUploadHeapAllocator   UploadHeap;
    FVulkanMemoryManager         MemoryManager;
    FVulkanFenceManager          FenceManager;
    FVulkanPipelineLayoutManager PipelineLayoutManager;
    FVulkanPipelineStateManager  PipelineStateManager;
    FVulkanDescriptorSetCache    DescriptorSetCache;
    FVulkanDefaultResources      DefaultResources;
    FVulkanQueryPoolManager      QueryPoolManager;

    TSet<FString>                        ExtensionNames;
    TSet<FString>                        LayerNames;
    TOptional<FVulkanQueueFamilyIndices> QueueIndicies;

    TMap<FVulkanHashableSamplerCreateInfo, VkSampler> SamplerMap;
    FCriticalSection                                  SamplerMapCS;

    bool bSupportsDepthClip                 : 1;
    bool bSupportsConservativeRasterization : 1;
    bool bSupportsPipelineCacheControl      : 1;
    bool bSupportsAccelerationStructures    : 1;
};
