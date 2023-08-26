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


class FVulkanBlendState : public FRHIBlendState
{
public:
    FVulkanBlendState(const FRHIBlendStateDesc& InDesc)
        : FRHIBlendState()
        , Desc(InDesc)
    {
    }

    virtual ~FVulkanBlendState() = default;

    virtual FRHIBlendStateDesc GetDesc() const override final { return Desc; }

private:
    FRHIBlendStateDesc Desc;
};

class FVulkanGraphicsPipelineState : public FRHIGraphicsPipelineState
{
public:
    FVulkanGraphicsPipelineState()  = default;
    ~FVulkanGraphicsPipelineState() = default;
};

class FVulkanComputePipelineState : public FRHIComputePipelineState
{
public:
    FVulkanComputePipelineState()  = default;
    ~FVulkanComputePipelineState() = default;
};

class FVulkanRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:
    FVulkanRayTracingPipelineState()  = default;
    ~FVulkanRayTracingPipelineState() = default;
};
