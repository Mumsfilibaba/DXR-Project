#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanRefCounted.h"
#include "VulkanRHI/VulkanDeviceChild.h"

typedef TSharedRef<class FVulkanInputLayout>            FVulkanVertexInputLayoutRef;
typedef TSharedRef<class FVulkanDepthStencilState>       FVulkanDepthStencilStateRef;
typedef TSharedRef<class FVulkanGraphicsPipelineState>   FVulkanGraphicsPipelineStateRef;
typedef TSharedRef<class FVulkanComputePipelineState>    FVulkanComputePipelineStateRef;
typedef TSharedRef<class FVulkanRayTracingPipelineState> FVulkanRayTracingPipelineStateRef;

class FVulkanInputLayout : public FRHIInputLayout
{
public:
    FVulkanInputLayout(const TArray<FRHIInputElementInfo>& InInputElements);
    virtual ~FVulkanInputLayout();

    virtual const FRHIInputElementInfo* GetInputElementInfo(uint32 Index) const override final
    {
        return &InputElements[Index];
    }

    virtual uint32 GetNumInputElementInfos() const override final
    {
        return InputElements.Size();
    }

    const VkPipelineVertexInputStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    TArray<FRHIInputElementInfo>              InputElements;
    TArray<VkVertexInputBindingDescription>   VertexInputBindingDescriptions;
    TArray<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;
    VkPipelineVertexInputStateCreateInfo      CreateInfo;
};

class FVulkanDepthStencilState : public FRHIDepthStencilState
{
public:
    FVulkanDepthStencilState(const FRHIDepthStencilStateInfo& InInfo);
    virtual ~FVulkanDepthStencilState();

    virtual FRHIDepthStencilStateInfo GetInfo() const override final
    {
        return Info;
    }

    const VkPipelineDepthStencilStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    FRHIDepthStencilStateInfo Info;
    VkPipelineDepthStencilStateCreateInfo CreateInfo;
};

class FVulkanRasterizerState : public FRHIRasterizerState, public FVulkanDeviceChild
{
public:
    FVulkanRasterizerState(FVulkanDevice* InDevice, const FRHIRasterizerStateInfo& InInfo);
    virtual ~FVulkanRasterizerState();

    virtual FRHIRasterizerStateInfo GetInfo() const override final
    {
        return Info;
    }

    const VkPipelineRasterizationStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }
    
private:
    FRHIRasterizerStateInfo Info;
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
    FVulkanBlendState(const FRHIBlendStateInfo& InInfo);
    virtual ~FVulkanBlendState();

    virtual FRHIBlendStateInfo GetInfo() const override final
    {
        return Info;
    }

    const VkPipelineColorBlendStateCreateInfo& GetVkCreateInfo() const
    {
        return CreateInfo;
    }

private:
    FRHIBlendStateInfo Info;
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
    FString    DebugName;
    VkPipeline Pipeline;

    // Layout is NOT owned by this class and should not be deleted when the FVulkanPipeline is destroyed
    FVulkanPipelineLayout* PipelineLayout;
};

class FVulkanGraphicsPipelineState : public FRHIGraphicsPipelineState, public FVulkanPipeline
{
public:
    FVulkanGraphicsPipelineState(FVulkanDevice* InDevice);
    virtual ~FVulkanGraphicsPipelineState();

    bool Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer);
    
    // FRHIPipelineState Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetVkPipeline()); }

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

    // FRHIPipelineState Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetVkPipeline()); }
    
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
