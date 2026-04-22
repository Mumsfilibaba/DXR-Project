#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanDeviceChild.h"

typedef TSharedRef<class FVulkanInputLayoutRHI>            FVulkanVertexInputLayoutRHIRef;
typedef TSharedRef<class FVulkanDepthStencilStateRHI>       FVulkanDepthStencilStateRHIRef;
typedef TSharedRef<class FVulkanGraphicsPipelineStateRHI>   FVulkanGraphicsPipelineStateRHIRef;
typedef TSharedRef<class FVulkanComputePipelineStateRHI>    FVulkanComputePipelineStateRHIRef;
typedef TSharedRef<class FVulkanRayTracingPipelineStateRHI> FVulkanRayTracingPipelineStateRHIRef;

class FVulkanInputLayoutRHI : public FRHIInputLayout
{
public:
    FVulkanInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements);
    virtual ~FVulkanInputLayoutRHI();

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final
    {
        return &InputElements[Index];
    }

    virtual uint32 GetNumInputElementDescs() const override final
    {
        return InputElements.Size();
    }

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

    virtual FRHIDepthStencilStateDesc GetDesc() const override final
    {
        return Desc;
    }

    const VkPipelineDepthStencilStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    FRHIDepthStencilStateDesc             Desc;
    VkPipelineDepthStencilStateCreateInfo CreateInfo;
};

class FVulkanRasterizerStateRHI : public FRHIRasterizerState, public FVulkanDeviceChild
{
public:
    FVulkanRasterizerStateRHI(FVulkanDevice* InDevice, const FRHIRasterizerStateDesc& InDesc);
    virtual ~FVulkanRasterizerStateRHI();

    virtual FRHIRasterizerStateDesc GetDesc() const override final
    {
        return Desc;
    }

    const VkPipelineRasterizationStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }
    
private:
    FRHIRasterizerStateDesc                Desc;
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

    virtual FRHIBlendStateDesc GetDesc() const override final
    {
        return Desc;
    }

    const VkPipelineColorBlendStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    FRHIBlendStateDesc                  Desc;
    VkPipelineColorBlendStateCreateInfo CreateInfo;
    VkPipelineColorBlendAttachmentState BlendAttachmentStates[VULKAN_MAX_RENDER_TARGET_COUNT];
};

class FVulkanPipeline : public FVulkanDeviceChild
{
public:
    FVulkanPipeline(FVulkanDevice* InDevice);
    virtual ~FVulkanPipeline();

    void SetDebugName(const FString& InName);

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
    FString                DebugName;
};

class FVulkanGraphicsPipelineStateRHI : public FRHIGraphicsPipelineState, public FVulkanPipeline
{
public:
    FVulkanGraphicsPipelineStateRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanGraphicsPipelineStateRHI();

    bool Initialize(const FRHIGraphicsPipelineStateDesc& InDesc);
    
    // FRHIPipelineState Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetVkPipeline()); }

    virtual void SetDebugName(const FString& InName) override final
    {
        FVulkanPipeline::SetDebugName(InName);
    }

    virtual FString GetDebugName() const override final
    {
        return DebugName;
    }

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
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetVkPipeline()); }
    
    virtual void SetDebugName(const FString& InName) override final
    {
        FVulkanPipeline::SetDebugName(InName);
    }

    virtual FString GetDebugName() const override final
    {
        return DebugName;
    }
};

class FVulkanRayTracingPipelineStateRHI : public FRHIRayTracingPipelineState
{
public:
    FVulkanRayTracingPipelineStateRHI() = default;
    virtual ~FVulkanRayTracingPipelineStateRHI() = default;
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
