#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/StringView.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Optional.h"
#include "Core/Threading/Atomic.h"
#include "Core/Containers/String.h"
#include "VulkanRHI/VulkanCore.h"
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

// -------------------------------------------------------------------------------------------
// Vulkan Device Feature Support
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanForceBinding;
extern VULKANRHI_API bool   GVulkanAllowNullDescriptors;
extern VULKANRHI_API bool   GVulkanAllowGeometryShaders;
extern VULKANRHI_API bool   GVulkanAllowResetCommandBuffers;
extern VULKANRHI_API bool   GVulkanRobustBufferAccessEnabled;
extern VULKANRHI_API bool   GVulkanGPUAssistedValidationEnabled;

extern VULKANRHI_API bool   GVulkanSupportsDepthClip;
extern VULKANRHI_API bool   GVulkanSupportsDepthClamp;
extern VULKANRHI_API bool   GVulkanSupportsNullDescriptors;
extern VULKANRHI_API bool   GVulkanSupportsRobustness2;
extern VULKANRHI_API bool   GVulkanSupportsConservativeRasterization;
extern VULKANRHI_API float  GVulkanMaxExtraPrimitiveOverestimationSize;
extern VULKANRHI_API bool   GVulkanSupportsPipelineCacheControl;
extern VULKANRHI_API bool   GVulkanSupportsMultiviews;
extern VULKANRHI_API bool   GVulkanSupportsBindless;
extern VULKANRHI_API bool   GVulkanSupportsDepthBoundsTest;
extern VULKANRHI_API bool   GVulkanSupportsSparseBinding;
extern VULKANRHI_API bool   GVulkanSupportsSparseResidency2D;
extern VULKANRHI_API bool   GVulkanSupportsSparseResidency3D;
extern VULKANRHI_API bool   GVulkanSupportsSparseResidencyAliased;
extern VULKANRHI_API bool   GVulkanSupportsGeometryShader;
extern VULKANRHI_API bool   GVulkanSupportsTessellation;

extern VULKANRHI_API uint32 GVulkanMaxMultiviewViewCount;
extern VULKANRHI_API uint32 GVulkanMaxDrawIndirectCount;

// -------------------------------------------------------------------------------------------
// Programmable sample positions (VK_EXT_sample_locations)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool GVulkanSupportsSampleLocations;

// -------------------------------------------------------------------------------------------
// Programmable sample positions (VK_EXT_fragment_shader_interlock)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool GVulkanSupportsFragmentShaderInterlock;

// -------------------------------------------------------------------------------------------
// Ray Tracing (VK_KHR_ray_tracing_pipeline, VK_KHR_ray_query)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool GVulkanSupportsRayTracingPipeline; 
extern VULKANRHI_API bool GVulkanSupportsRayQuery;
extern VULKANRHI_API bool GVulkanSupportsAccelerationStructures;

// -------------------------------------------------------------------------------------------
// Variable Rate Shading (VK_KHR_fragment_shading_rate)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanSupportsFragmentShadingRate;
extern VULKANRHI_API uint32 GVulkanShadingRateTileSize;

// -------------------------------------------------------------------------------------------
// Mesh shaders (VK_EXT_mesh_shader)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanSupportsMeshShaders;
extern VULKANRHI_API uint32 GVulkanMaxMeshOutputVertices;
extern VULKANRHI_API uint32 GVulkanMaxMeshWorkGroupInvocations;
extern VULKANRHI_API uint32 GVulkanMaxTaskWorkGroupInvocations;

// -------------------------------------------------------------------------------------------
// Transform Feedback / Stream Output (VK_EXT_transform_feedback)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanSupportsTransformFeedback;

// -------------------------------------------------------------------------------------------
// Dynamic Rendering (VK_KHR_dynamic_rendering / Vulkan 1.3)
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API bool   GVulkanUseDynamicRendering;

// -------------------------------------------------------------------------------------------
// Descriptor / Heap Limits
// -------------------------------------------------------------------------------------------

extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetSamplers;
extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetSampledImages;
extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageImages;
extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetUniformBuffers;
extern VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageBuffers;

enum class EVulkanCommandQueueType
{
    Unknown  = 0, 
    Graphics = 1, 
    Copy     = 2, 
    Compute  = 3, 
    Present  = 4,
};

struct VULKANRHI_API FVulkanCoreFeatures
{
    VkPhysicalDeviceFeatures         Features10 = {};
    VkPhysicalDeviceVulkan11Features Features11 = {};
    VkPhysicalDeviceVulkan12Features Features12 = {};
    VkPhysicalDeviceVulkan13Features Features13 = {};

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
		, NullBufferStorage(nullptr)
		, NullImage(VK_NULL_HANDLE)
		, NullImageView(VK_NULL_HANDLE)
		, NullImageStorage(nullptr)
		, NullSampler(VK_NULL_HANDLE)
	{
	}

	~FVulkanDefaultResources()
	{
		CHECK(NullBuffer == VK_NULL_HANDLE);
		CHECK(NullImage == VK_NULL_HANDLE);
		CHECK(NullImageView == VK_NULL_HANDLE);
		CHECK(NullSampler == VK_NULL_HANDLE);
	}

	bool Initialize(FVulkanDevice& Device);
	bool InitializeNullBuffer(FVulkanDevice& Device);
	bool InitializeNullBufferAndImage(FVulkanDevice& Device);
	void Release(FVulkanDevice& Device);

	VkBuffer             NullBuffer;
	FVulkanMemoryStorage NullBufferStorage;
	VkImage              NullImage;
	VkImageView          NullImageView;
	FVulkanMemoryStorage NullImageStorage;
	VkSampler            NullSampler;
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
		return CRC32::Generate(&Value, sizeof(Value));
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
    bool FindOrCreateSampler(const struct FRHISamplerStateInfo& SamplerInfo, VkSampler& OutSampler);
    bool FindOrCreateSampler(const struct FRHIStaticSamplerInfo& StaticSamplerInfo, VkSampler& OutSampler);

    FVulkanQueryPoolManager* GetQueryPoolManager(VkQueryType QueryType);
    FVulkanQueryPool*        ObtainQueryPool(VkQueryType QueryType);
    void                     RecycleQueryPool(FVulkanQueryPool* Pool);
    uint32 GetQueueIndexFromType(EVulkanCommandQueueType Type) const;
    bool   InitializePresentQueueFamily(VkSurfaceKHR Surface);

    FVulkanRenderPassCache&       GetRenderPassCache()       { return *RenderPassCache; }
    FVulkanMemoryManager&         GetMemoryManager()         { return *MemoryManager; }
    FVulkanFenceManager&          GetFenceManager()          { return *FenceManager; }
    FVulkanTimelineFence&         GetFrameFence()            { return *FrameFence; }
    FVulkanPipelineLayoutManager& GetPipelineLayoutManager() { return *PipelineLayoutManager; }
    FVulkanPipelineStateManager&  GetPipelineStateManager()  { return *PipelineStateManager; }
#if VULKAN_USE_DESCRIPTOR_CACHE
    FVulkanDescriptorSetCache&    GetDescriptorSetCache()    { return *DescriptorSetCache; }
#else
    FVulkanDescriptorPoolManager& GetDescriptorPoolManager() { return *DescriptorPoolManager; }
#endif
    FVulkanDefaultResources&      GetDefaultResources()      { return DefaultResources; }

    bool IsLayerEnabled(const FString& LayerName)         const { return (LayerNames.Find(LayerName) != nullptr); }
    bool IsExtensionEnabled(const FString& ExtensionName) const { return (ExtensionNames.Find(ExtensionName) != nullptr); }

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
    using FSamplerMap = TMap<FVulkanHashableSamplerCreateInfo, VkSampler>;

    FVulkanInstance*                     Instance;
    FVulkanPhysicalDevice*               PhysicalDevice;
    VkDevice                             Device;
    FVulkanRenderPassCache*              RenderPassCache;
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
    FVulkanQueryPoolManager*             TimingQueryPoolManager;
    FVulkanQueryPoolManager*             OcclusionQueryPoolManager;
    FVulkanQueryPoolManager*             PipelineStatsQueryPoolManager;
    FVulkanDefaultResources              DefaultResources;
    TSet<FString>                        ExtensionNames;
    TSet<FString>                        LayerNames;
    TOptional<FVulkanQueueFamilyIndices> QueueIndicies;
    FSamplerMap                          SamplerMap;
    FCriticalSection                     SamplerMapCS;

#if VULKAN_ENABLE_CRASH_MARKERS
    bool bSupportsAMDBufferMarker         : 1;
    bool bSupportsNVDiagnosticCheckpoints : 1;
#endif
};
