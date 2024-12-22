#pragma once
#include "VulkanShader.h"
#include "VulkanRefCounted.h"
#include "VulkanDeviceChild.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanVertexLayout>            FVulkanVertexInputLayoutRef;
typedef TSharedRef<class FVulkanDepthStencilState>       FVulkanDepthStencilStateRef;
typedef TSharedRef<class FVulkanGraphicsPipelineState>   FVulkanGraphicsPipelineStateRef;
typedef TSharedRef<class FVulkanComputePipelineState>    FVulkanComputePipelineStateRef;
typedef TSharedRef<class FVulkanRayTracingPipelineState> FVulkanRayTracingPipelineStateRef;

class FVulkanVertexLayout : public FRHIVertexLayout
{
public:
    FVulkanVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList);
    virtual ~FVulkanVertexLayout();

    virtual FRHIVertexLayoutInitializerList GetInitializerList() const override final
    {
        return InitializerList;
    }

    const VkPipelineVertexInputStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    FRHIVertexLayoutInitializerList           InitializerList;
    TArray<VkVertexInputBindingDescription>   VertexInputBindingDescriptions;
    TArray<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;
    VkPipelineVertexInputStateCreateInfo      CreateInfo;
};

class FVulkanDepthStencilState : public FRHIDepthStencilState
{
public:
    FVulkanDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer);
    virtual ~FVulkanDepthStencilState();

    virtual FRHIDepthStencilStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

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
    virtual ~FVulkanRasterizerState();

    virtual FRHIRasterizerStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

    const VkPipelineRasterizationStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }
    
private:
    FRHIRasterizerStateInitializer         Initializer;
    VkPipelineRasterizationStateCreateInfo CreateInfo;
#if VK_EXT_depth_clip_enable
    VkPipelineRasterizationDepthClipStateCreateInfoEXT DepthClipStateCreateInfo;
#endif
#if VK_EXT_conservative_rasterization
    VkPipelineRasterizationConservativeStateCreateInfoEXT ConservativeStateCreateInfo;
#endif
};

class FVulkanBlendState : public FRHIBlendState
{
public:
    FVulkanBlendState(const FRHIBlendStateInitializer& InInitializer);
    virtual ~FVulkanBlendState();

    virtual FRHIBlendStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

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
    VkPipeline             Pipeline;
    // Layout is NOT owned by this class and should not be deleted when the FVulkanPipeline is destroyed
    FVulkanPipelineLayout* PipelineLayout;
    FString                DebugName;
};

class FVulkanGraphicsPipelineState : public FRHIGraphicsPipelineState, public FVulkanPipeline
{
public:
    FVulkanGraphicsPipelineState(FVulkanDevice* InDevice);
    virtual ~FVulkanGraphicsPipelineState();

    bool Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer);
    
    virtual void SetDebugName(const FString& InName) override final
    {
        FVulkanPipeline::SetDebugName(InName);
    }

    FORCEINLINE const FViewInstancingInfo& GetViewInstancingInfo() const
    {
        return ViewInstancingInfo;
    }
    
private:
    FViewInstancingInfo ViewInstancingInfo;
};

class FVulkanComputePipelineState : public FRHIComputePipelineState, public FVulkanPipeline
{
public:
    FVulkanComputePipelineState(FVulkanDevice* InDevice);
    virtual ~FVulkanComputePipelineState();
    
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
    
    VkPipelineCache GetVkPipelineCache() const
    {
        return PipelineCache;
    }

private:
    bool LoadCacheFromFile();
    
    VkPipelineCache  PipelineCache;
    FCriticalSection PipelineCacheCS;
    bool             bPipelineCacheDirty;
};
