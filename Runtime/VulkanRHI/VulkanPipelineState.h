#pragma once
#include "VulkanRefCounted.h"
#include "VulkanDeviceObject.h"
#include "RHI/RHIResources.h"
#include "Core/Utilities/StringUtilities.h"

typedef TSharedRef<class FVulkanVertexInputLayout>       FVulkanVertexInputLayoutRef;
typedef TSharedRef<class FVulkanDepthStencilState>       FVulkanDepthStencilStateRef;
typedef TSharedRef<class FVulkanGraphicsPipelineState>   FVulkanGraphicsPipelineStateRef;
typedef TSharedRef<class FVulkanComputePipelineState>    FVulkanComputePipelineStateRef;
typedef TSharedRef<class FVulkanRayTracingPipelineState> FVulkanRayTracingPipelineStateRef;


class FVulkanVertexInputLayout : public FRHIVertexInputLayout, public FVulkanRefCounted
{
public:
    FVulkanVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer);
    virtual ~FVulkanVertexInputLayout() = default;

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

    const VkVertexInputBindingDescription* GetVertexInputBindingDescriptions() const
    {
        return VertexInputBindingDescriptions.Data();
    }
    
    uint32 GetNumVertexInputBindingDescriptions() const
    {
        return VertexInputBindingDescriptions.Size();
    }
    
    const VkVertexInputAttributeDescription* GetVertexInputAttributeDescriptions() const
    {
        return VertexInputAttributeDescriptions.Data();
    }
    
    uint32 GetNumVertexInputAttributeDescriptions() const
    {
        return VertexInputAttributeDescriptions.Size();
    }
    
private:
    TArray<VkVertexInputBindingDescription>   VertexInputBindingDescriptions;
    TArray<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;
};


class FVulkanDepthStencilState : public FRHIDepthStencilState, public FVulkanRefCounted
{
public:
    FVulkanDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer);
    virtual ~FVulkanDepthStencilState() = default;

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

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


class FVulkanRasterizerState : public FRHIRasterizerState, public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanRasterizerState(FVulkanDevice* InDevice, const FRHIRasterizerStateInitializer& InInitializer);
    virtual ~FVulkanRasterizerState() = default;

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

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
    VkPipelineRasterizationDepthClipStateCreateInfoEXT    DepthClipStateCreateInfo;
#endif
#if VK_EXT_conservative_rasterization
    VkPipelineRasterizationConservativeStateCreateInfoEXT ConservativeStateCreateInfo;
#endif
};


class FVulkanBlendState : public FRHIBlendState, public FVulkanRefCounted
{
public:
    FVulkanBlendState(const FRHIBlendStateInitializer& InInitializer);
    virtual ~FVulkanBlendState() = default;

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }

    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }

    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

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
    VkPipelineColorBlendAttachmentState BlendAttachmentStates[FRHILimits::MaxRenderTargets];
};


class FVulkanPipeline : public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanPipeline(FVulkanDevice* InDevice);
    ~FVulkanPipeline();

    void SetDebugName(const FString& InName);

    VkPipeline GetVkPipeline() const
    {
        return Pipeline;
    }

protected:
    VkPipeline            Pipeline;
    VkPipelineLayout      PipelineLayout;
    VkDescriptorSetLayout DescriptorSetLayouts[5];
};


class FVulkanGraphicsPipelineState : public FRHIGraphicsPipelineState, public FVulkanPipeline
{
public:
    FVulkanGraphicsPipelineState(FVulkanDevice* InDevice);
    virtual ~FVulkanGraphicsPipelineState() = default;

    bool Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer);

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }

    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }

    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }
};


class FVulkanComputePipelineState : public FRHIComputePipelineState, public FVulkanPipeline
{
public:
    FVulkanComputePipelineState(FVulkanDevice* InDevice);
    virtual ~FVulkanComputePipelineState() = default;
    
    bool Initialize(const FRHIComputePipelineStateInitializer& Initializer);

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }

    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }

    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }
};


class FVulkanRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:
    FVulkanRayTracingPipelineState()  = default;
    ~FVulkanRayTracingPipelineState() = default;
};
