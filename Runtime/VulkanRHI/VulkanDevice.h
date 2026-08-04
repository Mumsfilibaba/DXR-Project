#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Optional.h"
#include "Core/Threading/Atomic.h"
#include "Core/Containers/String.h"
#include "VulkanRHI/VulkanCore.h"
#include "VulkanRHI/VulkanCapabilities.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanExtensions.h"
#include "VulkanRHI/VulkanMemoryManager.h"
#include "VulkanRHI/VulkanRenderPass.h"
#include "VulkanRHI/VulkanFenceManager.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanPipelineState.h"
#include "VulkanRHI/VulkanQuery.h"

class FVulkanInstance;
class FVulkanPhysicalDevice;
class FVulkanTimelineFence;
class FVulkanQueue;

enum class EVulkanCommandQueueType : uint8
{
    Unknown  = 0,
    Graphics = 1,
    Copy     = 2,
    Compute  = 3,
    Present  = 4,
};

struct VULKANRHI_API FVulkanCoreFeatures
{
    FVulkanCoreFeatures();

    VkPhysicalDeviceFeatures         Features10;
    VkPhysicalDeviceVulkan11Features Features11;
    VkPhysicalDeviceVulkan12Features Features12;

    void BuildQueryChain(VkPhysicalDeviceFeatures2& Root);
    bool CheckRequired(VkPhysicalDevice PhysicalDevice) const;
    void EnableAvailable(FVulkanCoreFeatures& OutEnabled, const FVulkanCoreFeatures& Available) const;
    void BuildEnableChain(VkPhysicalDeviceFeatures2& Root);
};

struct FVulkanDeviceCreateInfo
{
    FVulkanCoreFeatures RequiredFeatures;
    FVulkanCoreFeatures OptionalFeatures;

    TArray<const CHAR*> RequiredLayerNames;
    TArray<const CHAR*> OptionalLayerNames;
    TArray<TUniquePtr<FVulkanDeviceExtension>> Extensions;
};

struct FVulkanQueueFamilyIndices
{
    FVulkanQueueFamilyIndices() = default;

    FVulkanQueueFamilyIndices(uint32 InGraphicsQueueIndex, uint32 InCopyQueueIndex, uint32 InComputeQueueIndex)
        : GraphicsQueueIndex(InGraphicsQueueIndex)
        , CopyQueueIndex(InCopyQueueIndex)
        , ComputeQueueIndex(InComputeQueueIndex)
        , PresentQueueIndex(InGraphicsQueueIndex)
    {
    }

    bool HasSeparatePresentQueue() const { return PresentQueueIndex != GraphicsQueueIndex; }

    uint32 GraphicsQueueIndex = uint32(~0);
    uint32 CopyQueueIndex     = uint32(~0);
    uint32 ComputeQueueIndex  = uint32(~0);
    uint32 PresentQueueIndex  = uint32(~0);
};

struct FVulkanDefaultResources
{
	FVulkanDefaultResources()
		: NullBuffer(VK_NULL_HANDLE)
		, NullBufferLocation(nullptr)
		, NullImage(VK_NULL_HANDLE)
		, NullImageViews()
		, NullImageLocation(nullptr)
		, NullSampler(VK_NULL_HANDLE)
	{
	}

	~FVulkanDefaultResources()
	{
		CHECK(NullBuffer == VK_NULL_HANDLE);
		CHECK(NullImage == VK_NULL_HANDLE);
		CHECK(NullSampler == VK_NULL_HANDLE);

		for (VkImageView NullImageView : NullImageViews)
		{
			CHECK(NullImageView == VK_NULL_HANDLE);
		}
	}

	bool Initialize(FVulkanDevice& Device);
	bool InitializeNullBuffer(FVulkanDevice& Device);
	bool InitializeNullBufferAndImage(FVulkanDevice& Device);
	void Release(FVulkanDevice& Device);

	VkImageView GetNullImageView(EVulkanNullImageViewType ViewType) const
	{
		CHECK(ViewType < EVulkanNullImageViewType::Count);
		return NullImageViews[static_cast<uint32>(ViewType)];
	}

	VkBuffer              NullBuffer;
	FVulkanMemoryLocation NullBufferLocation;
	VkImage               NullImage;
	VkImageView           NullImageViews[static_cast<uint32>(EVulkanNullImageViewType::Count)];
	FVulkanMemoryLocation NullImageLocation;
	VkSampler             NullSampler;
};

struct FVulkanHashableSamplerCreateInfo
{
	bool operator==(const FVulkanHashableSamplerCreateInfo& Other) const
	{
		return Memory::Memcmp(this, &Other, sizeof(FVulkanHashableSamplerCreateInfo)) == 0;
	}

	bool operator!=(const FVulkanHashableSamplerCreateInfo& Other) const
	{
		return Memory::Memcmp(this, &Other, sizeof(FVulkanHashableSamplerCreateInfo)) != 0;
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

template<>
struct THash<FVulkanHashableSamplerCreateInfo>
{
	static uint64 GetHash(const FVulkanHashableSamplerCreateInfo& Value)
	{
		return CRC32::Generate(&Value, sizeof(Value));
	}
};

class FVulkanPhysicalDevice
{
public:
    static TOptional<FVulkanQueueFamilyIndices> GetQueueFamilyIndices(VkPhysicalDevice PhysicalDevice);

public:
    FVulkanPhysicalDevice(FVulkanInstance* InInstance);
    ~FVulkanPhysicalDevice();

    bool Initialize(const FVulkanDeviceCreateInfo& InDeviceCreateInfo);

    uint32 FindMemoryTypeIndex(uint32 TypeFilter, VkMemoryPropertyFlags Properties);
    VkFormatProperties GetFormatProperties(VkFormat Format) const;
    
    // Vulkan 1.0 features
    const VkPhysicalDeviceProperties&        GetProperties()       const { return DeviceProperties; }
    const VkPhysicalDeviceFeatures&          GetFeatures()         const { return DeviceFeatures; }
    const VkPhysicalDeviceMemoryProperties&  GetMemoryProperties() const { return DeviceMemoryProperties; }

    // Vulkan 1.1 features
    const VkPhysicalDeviceVulkan11Features&  GetFeaturesVulkan11() const { return DeviceFeatures11; }
    
    // Vulkan 1.2 features
    const VkPhysicalDeviceProperties2&       GetProperties2()       const { return DeviceProperties2; }
    const VkPhysicalDeviceFeatures2&         GetFeatures2()         const { return DeviceFeatures2; }
    const VkPhysicalDeviceMemoryProperties2& GetMemoryProperties2() const { return DeviceMemoryProperties2; }
    const VkPhysicalDeviceVulkan12Features&  GetFeaturesVulkan12()  const { return DeviceFeatures12; }

#if VK_KHR_ray_tracing_pipeline
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingPipelineProperties() const { return RayTracingPipelineProperties; }
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
    
    // Vulkan 1.0 features
    VkPhysicalDeviceProperties        DeviceProperties;
    VkPhysicalDeviceFeatures          DeviceFeatures;
    VkPhysicalDeviceMemoryProperties  DeviceMemoryProperties;

    // Vulkan 1.1 features
    VkPhysicalDeviceVulkan11Features  DeviceFeatures11;

    // Vulkan 1.2 features
    VkPhysicalDeviceProperties2       DeviceProperties2;
    VkPhysicalDeviceFeatures2         DeviceFeatures2;
    VkPhysicalDeviceMemoryProperties2 DeviceMemoryProperties2;
    VkPhysicalDeviceVulkan12Features  DeviceFeatures12;
#if VK_KHR_ray_tracing_pipeline
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingPipelineProperties;
#endif
};

class FVulkanDevice
{
public:
    FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter);
    ~FVulkanDevice();

    bool Initialize(FVulkanDeviceCreateInfo& InDeviceCreateInfo);
    bool PostLoaderInitalize();
    
    bool InitializeDeviceFeatureSupport();
    bool InitializeDefaultResources(class FVulkanCommandContext& CommandContext);

    // Create or returns an already created sampler, this is to avoid creating duplicate samplers
    bool FindOrCreateSampler(const VkSamplerCreateInfo& SamplerCreateInfo, VkSampler& OutSampler);
    bool FindOrCreateSampler(const struct FRHISamplerStateDesc& SamplerDesc, VkSampler& OutSampler);
    bool FindOrCreateSampler(const struct FRHIStaticSamplerInfo& StaticSamplerInfo, VkSampler& OutSampler);

    FVulkanQueryPoolManager* GetQueryPoolManager(VkQueryType QueryType);
    FVulkanQueryPool*        ObtainQueryPool(VkQueryType QueryType);
    void                     RecycleQueryPool(FVulkanQueryPool* Pool);
    uint32                   GetQueueIndexFromType(EVulkanCommandQueueType Type) const;
    bool                     InitializePresentQueueFamily(VkSurfaceKHR Surface);

    bool CreateGraphicsQueue();
    bool EnsurePresentQueue();
    void WaitForGPU();

    FVulkanQueue* GetQueue(EVulkanCommandQueueType Type) const;
    FVulkanQueue* GetGraphicsQueue() const { return GraphicsQueue; }
    FVulkanQueue* GetPresentQueue()  const { return PresentQueue; }

    FVulkanMemoryManager&             GetMemoryManager()             { return *MemoryManager; }
    FVulkanFenceManager&              GetFenceManager()              { return *FenceManager; }
    FVulkanTimelineFence&             GetFrameFence()                { return *FrameFence; }
    FVulkanPipelineLayoutManager&     GetPipelineLayoutManager()     { return *PipelineLayoutManager; }
    FVulkanPipelineStateManager&      GetPipelineStateManager()      { return *PipelineStateManager; }
    FVulkanBindlessDescriptorManager* GetBindlessDescriptorManager() { return BindlessDescriptorManager; }
    FVulkanDefaultResources&          GetDefaultResources()          { return DefaultResources; }

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    FVulkanRenderPassCache& GetRenderPassCache()
    {
        return *RenderPassCache;
    }
#endif
    
#if VULKAN_USE_DESCRIPTOR_CACHE
    FVulkanDescriptorSetCache& GetDescriptorSetCache()
    {
        return *DescriptorSetCache;
    }
#else
    FVulkanDescriptorPoolManager& GetDescriptorPoolManager()
    {
        return *DescriptorPoolManager;
    }
#endif

    bool IsLayerEnabled(const String& LayerName)         const { return (LayerNames.Find(LayerName) != nullptr); }
    bool IsExtensionEnabled(const String& ExtensionName) const { return (ExtensionNames.Find(ExtensionName) != nullptr); }

#if VULKAN_ENABLE_CRASH_MARKERS
    bool IsAMDBufferMarkerEnabled()         const { return bSupportsAMDBufferMarker; }
    bool IsNVDiagnosticCheckpointsEnabled() const { return bSupportsNVDiagnosticCheckpoints; }
    bool IsCrashMarkerExtensionsEnabled()   const { return bSupportsAMDBufferMarker || bSupportsNVDiagnosticCheckpoints; }
#endif

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
    void DeriveCoreCapabilities(FVulkanDeviceCreateInfo& InDeviceCreateInfo, const FVulkanCoreFeatures& AvailableFeatures, const VkPhysicalDeviceProperties& CoreDeviceProperties10,
        const VkPhysicalDeviceMultiviewProperties& MultiviewProperties, const VkPhysicalDeviceSubgroupProperties& SubgroupProperties,
        const VkPhysicalDeviceVulkan12Properties& CoreDeviceProperties12);
    void DeriveEnabledFeatureCapabilities(const FVulkanCoreFeatures& EnabledFeatures);

    using FSamplerMap = TMap<FVulkanHashableSamplerCreateInfo, VkSampler>;

    FVulkanInstance*                     Instance;
    FVulkanPhysicalDevice*               PhysicalDevice;
    VkDevice                             Device;
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    FVulkanRenderPassCache*              RenderPassCache;
#endif
    FVulkanMemoryManager*                MemoryManager;
    FVulkanFenceManager*                 FenceManager;
    FVulkanTimelineFence*                FrameFence;
    FVulkanPipelineLayoutManager*        PipelineLayoutManager;
    FVulkanPipelineStateManager*         PipelineStateManager;
#if VULKAN_USE_DESCRIPTOR_CACHE
    FVulkanDescriptorSetCache*           DescriptorSetCache;
#else
    FVulkanDescriptorPoolManager*        DescriptorPoolManager;
#endif
    FVulkanBindlessDescriptorManager*    BindlessDescriptorManager;
    FVulkanQueryPoolManager*             TimingQueryPoolManager;
    FVulkanQueryPoolManager*             OcclusionQueryPoolManager;
    FVulkanQueryPoolManager*             PipelineStatsQueryPoolManager;
    FVulkanDefaultResources              DefaultResources;
    TSet<String>                         ExtensionNames;
    TSet<String>                         LayerNames;
    TOptional<FVulkanQueueFamilyIndices> QueueIndicies;
    FVulkanQueue*                        GraphicsQueue;
    FVulkanQueue*                        PresentQueue;
    FSamplerMap                          SamplerMap;
    FCriticalSection                     SamplerMapCS;

#if VULKAN_ENABLE_CRASH_MARKERS
    bool bSupportsAMDBufferMarker         : 1;
    bool bSupportsNVDiagnosticCheckpoints : 1;
#endif
};
