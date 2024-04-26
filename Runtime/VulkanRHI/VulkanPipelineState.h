#pragma once
#include "VulkanShader.h"
#include "VulkanRefCounted.h"
#include "VulkanDeviceChild.h"
#include "RHI/RHIResources.h"
#include "Core/Utilities/StringUtilities.h"

typedef TSharedRef<class FVulkanVertexInputLayout>       FVulkanVertexInputLayoutRef;
typedef TSharedRef<class FVulkanDepthStencilState>       FVulkanDepthStencilStateRef;
typedef TSharedRef<class FVulkanGraphicsPipelineState>   FVulkanGraphicsPipelineStateRef;
typedef TSharedRef<class FVulkanComputePipelineState>    FVulkanComputePipelineStateRef;
typedef TSharedRef<class FVulkanRayTracingPipelineState> FVulkanRayTracingPipelineStateRef;

class FVulkanVertexInputLayout : public FRHIVertexInputLayout
{
public:
    FVulkanVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer);
    virtual ~FVulkanVertexInputLayout() = default;

    const VkPipelineVertexInputStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }
    
private:
    TArray<VkVertexInputBindingDescription>   VertexInputBindingDescriptions;
    TArray<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;
    VkPipelineVertexInputStateCreateInfo      CreateInfo;
};

class FVulkanDepthStencilState : public FRHIDepthStencilState
{
public:
    FVulkanDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer);
    virtual ~FVulkanDepthStencilState() = default;

    virtual FRHIDepthStencilStateInitializer GetInitializer() const override final { return Initializer; }
    
    const VkPipelineDepthStencilStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    FRHIDepthStencilStateInitializer      Initializer;
    VkPipelineDepthStencilStateCreateInfo CreateInfo;
};

class FVulkanRasterizerState : public FRHIRasterizerState, public FVulkanDeviceChild
{
public:
    FVulkanRasterizerState(FVulkanDevice* InDevice, const FRHIRasterizerStateInitializer& InInitializer);
    virtual ~FVulkanRasterizerState() = default;

    virtual FRHIRasterizerStateInitializer GetInitializer() const override final { return Initializer; }

    const VkPipelineRasterizationStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }
    
private:
    FRHIRasterizerStateInitializer         Initializer;
    VkPipelineRasterizationStateCreateInfo CreateInfo;
#if VK_EXT_depth_clip_enable
    VkPipelineRasterizationDepthClipStateCreateInfoEXT    DepthClipStateCreateInfo;
#endif
#if VK_EXT_conservative_rasterization
    VkPipelineRasterizationConservativeStateCreateInfoEXT ConservativeStateCreateInfo;
#endif
};

class FVulkanBlendState : public FRHIBlendState
{
public:
    FVulkanBlendState(const FRHIBlendStateInitializer& InInitializer);
    virtual ~FVulkanBlendState() = default;

    virtual FRHIBlendStateInitializer GetInitializer() const override final { return Initializer; }

    const VkPipelineColorBlendStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    FRHIBlendStateInitializer           Initializer;
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
    // Pipeline Object is owned by this class
    VkPipeline Pipeline;
    // Layout is NOT owned by this class and should not be deleted when the FVulkanPipeline is destroyed
    FVulkanPipelineLayout* PipelineLayout;
};

class FVulkanGraphicsPipelineState : public FRHIGraphicsPipelineState, public FVulkanPipeline
{
public:
    FVulkanGraphicsPipelineState(FVulkanDevice* InDevice);
    virtual ~FVulkanGraphicsPipelineState() = default;

    bool Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer);
    
    virtual void SetDebugName(const FString& InName) override final
    {
        FVulkanPipeline::SetDebugName(InName);
    }
};

class FVulkanComputePipelineState : public FRHIComputePipelineState, public FVulkanPipeline
{
public:
    FVulkanComputePipelineState(FVulkanDevice* InDevice);
    virtual ~FVulkanComputePipelineState() = default;
    
    bool Initialize(const FRHIComputePipelineStateInitializer& Initializer);

    virtual void SetDebugName(const FString& InName) override final
    {
        FVulkanPipeline::SetDebugName(InName);
    }
};

class FVulkanRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:
    FVulkanRayTracingPipelineState() = default;
    virtual ~FVulkanRayTracingPipelineState() = default;
};


struct FVulkanPipelineCacheHeader
{
    uint32 Length;  // == sizeof(FVulkanPipelineCacheHeader)
    uint32 Version; // == VK_PIPELINE_CACHE_HEADER_VERSION_ONE
    uint32 VendorID;
    uint32 DeviceID;
    uint8  UUID[VK_UUID_SIZE];
};

class FVulkanPipelineCache : public FVulkanDeviceChild
{
public:
    FVulkanPipelineCache(FVulkanDevice* InDevice);
    ~FVulkanPipelineCache();

    bool Initialize();
    void Release();
    bool CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo& CreateInfo, VkPipeline& OutPipeline);
    bool CreateComputePipeline(const VkComputePipelineCreateInfo& CreateInfo, VkPipeline& OutPipeline);
    bool SaveCacheData();
    
    VkPipelineCache GetVkPipelineCache() const
    {
        return PipelineCache;
    }

private:
    bool LoadCacheFromFile();
    
    VkPipelineCache  PipelineCache;
    FCriticalSection PipelineCacheCS;
    bool             bPipelineDirty;
};
