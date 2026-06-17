#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanDeviceChild.h"

typedef TSharedRef<class FVulkanInputLayoutRHI>             FVulkanVertexInputLayoutRHIRef;
typedef TSharedRef<class FVulkanDepthStencilStateRHI>       FVulkanDepthStencilStateRHIRef;
typedef TSharedRef<class FVulkanGraphicsPipelineStateRHI>   FVulkanGraphicsPipelineStateRHIRef;
typedef TSharedRef<class FVulkanComputePipelineStateRHI>    FVulkanComputePipelineStateRHIRef;
typedef TSharedRef<class FVulkanRayTracingPipelineStateRHI> FVulkanRayTracingPipelineStateRHIRef;

class FVulkanInputLayoutRHI : public FRHIInputLayout
{
public:
    FVulkanInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements);
    virtual ~FVulkanInputLayoutRHI();

    // FRHIInputLayout Interface
    virtual void* GetRHINativeState() const override final;

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final;
    virtual uint32 GetNumInputElementDescs() const override final;

    const VkPipelineVertexInputStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    TArray<FRHIInputElementDesc>              InputElements;
    TArray<VkVertexInputBindingDescription>   VertexInputBindingDescriptions;
    TArray<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;
    VkPipelineVertexInputStateCreateInfo      CreateInfo;
};

class FVulkanDepthStencilStateRHI : public FRHIDepthStencilState
{
public:
    FVulkanDepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc);
    virtual ~FVulkanDepthStencilStateRHI();

    // FRHIDepthStencilState Interface
    virtual void* GetRHINativeState() const override final;

    const VkPipelineDepthStencilStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    VkPipelineDepthStencilStateCreateInfo CreateInfo;
};

class FVulkanRasterizerStateRHI : public FRHIRasterizerState, public FVulkanDeviceChild
{
public:
    FVulkanRasterizerStateRHI(FVulkanDevice* InDevice, const FRHIRasterizerStateDesc& InDesc);
    virtual ~FVulkanRasterizerStateRHI();

    // FRHIRasterizerState Interface
    virtual void* GetRHINativeState() const override final;

    const VkPipelineRasterizationStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }
    
private:
    VkPipelineRasterizationStateCreateInfo CreateInfo;
#if VK_EXT_depth_clip_enable
    VkPipelineRasterizationDepthClipStateCreateInfoEXT    DepthClipStateCreateInfo;
#endif
#if VK_EXT_conservative_rasterization
    VkPipelineRasterizationConservativeStateCreateInfoEXT ConservativeStateCreateInfo;
#endif
};

class FVulkanBlendStateRHI : public FRHIBlendState
{
public:
    FVulkanBlendStateRHI(const FRHIBlendStateDesc& InDesc);
    virtual ~FVulkanBlendStateRHI();

    // FRHIBlendState Interface
    virtual void* GetRHINativeState() const override final;

    const VkPipelineColorBlendStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    VkPipelineColorBlendStateCreateInfo CreateInfo;
    VkPipelineColorBlendAttachmentState BlendAttachmentStates[VULKAN_MAX_RENDER_TARGET_COUNT];
};

class FVulkanPipeline : public FVulkanDeviceChild
{
public:
    FVulkanPipeline(FVulkanDevice* InDevice);
    virtual ~FVulkanPipeline();

    void SetDebugName(const String& InName);

    VkPipeline GetVkPipeline() const
    {
        return Pipeline;
    }
    
    FVulkanPipelineLayout* GetPipelineLayout() const
    {
        return PipelineLayout;
    }

protected:
    VkPipeline             Pipeline;
    FVulkanPipelineLayout* PipelineLayout; // Layout is NOT owned by this class and should not be deleted when the FVulkanPipeline is destroyed
#if VULKAN_STORE_DEBUG_NAMES
    String                 DebugName;
#endif
};

class FVulkanGraphicsPipelineStateRHI : public FRHIGraphicsPipelineState, public FVulkanPipeline
{
public:
    FVulkanGraphicsPipelineStateRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanGraphicsPipelineStateRHI();

    bool Initialize(const FRHIGraphicsPipelineStateDesc& InDesc);
    
    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    FORCEINLINE const FRHIViewInstancingState& GetViewInstancingState() const
    {
        return ViewInstancingState;
    }
    
private:
    FRHIViewInstancingState ViewInstancingState;
};

class FVulkanComputePipelineStateRHI : public FRHIComputePipelineState, public FVulkanPipeline
{
public:
    FVulkanComputePipelineStateRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanComputePipelineStateRHI();
    
    bool Initialize(const FRHIComputePipelineStateDesc& InDesc);

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;
    
    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;
};

class FVulkanRayTracingPipelineStateRHI : public FRHIRayTracingPipelineState
{
public:
    FVulkanRayTracingPipelineStateRHI() = default;
    virtual ~FVulkanRayTracingPipelineStateRHI() = default;

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;
};

struct FVulkanPipelineDataHeader
{
    CHAR   Magic[5];   // Always "VKPSO"
    CHAR   Padding[3]; 
    uint64 DataCRC;
    uint64 DataSize;
};

struct FVulkanPipelineCacheHeader
{
    uint32 Length;  // == sizeof(FVulkanPipelineCacheHeader)
    uint32 Version; // == VK_PIPELINE_CACHE_HEADER_VERSION_ONE
    uint32 VendorID;
    uint32 DeviceID;
    uint8  UUID[VK_UUID_SIZE];
};

class FVulkanPipelineStateManager : public FVulkanDeviceChild
{
public:
    FVulkanPipelineStateManager(FVulkanDevice* InDevice);
    ~FVulkanPipelineStateManager();

    bool Initialize();
    bool CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo& CreateInfo, VkPipeline& OutPipeline);
    bool CreateComputePipeline(const VkComputePipelineCreateInfo& CreateInfo, VkPipeline& OutPipeline);
    bool SaveCacheData();
    void SaveCacheDataAsync();
    
    VkPipelineCache GetVkPipelineCache() const
    {
        return PipelineCache;
    }

private:
    bool LoadCacheFromFile();
    
    VkPipelineCache  PipelineCache;
    FCriticalSection PipelineCacheCS;
    bool             bPipelineCacheDirty;
    uint64           LastSaveTimestamp;
};
