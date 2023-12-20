#pragma once
#include "VulkanShader.h"
#include "VulkanRefCounted.h"
#include "VulkanDeviceObject.h"
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


class FVulkanDepthStencilState : public FRHIDepthStencilState
{
public:
    FVulkanDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer);
    virtual ~FVulkanDepthStencilState() = default;

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


class FVulkanRasterizerState : public FRHIRasterizerState, public FVulkanDeviceObject
{
public:
    FVulkanRasterizerState(FVulkanDevice* InDevice, const FRHIRasterizerStateInitializer& InInitializer);
    virtual ~FVulkanRasterizerState() = default;

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
    virtual ~FVulkanBlendState() = default;

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


class FVulkanPipeline : public FVulkanDeviceObject
{
public:
    FVulkanPipeline(FVulkanDevice* InDevice);
    ~FVulkanPipeline();

    void SetDebugName(const FString& InName);

    VkPipeline GetVkPipeline() const
    {
        return Pipeline;
    }
    
    VkPipelineLayout GetVkPipelineLayout() const
    {
        return PipelineLayout;
    }

protected:
    VkPipeline       Pipeline;
    VkPipelineLayout PipelineLayout;
};


class FVulkanGraphicsPipelineState : public FRHIGraphicsPipelineState, public FVulkanPipeline
{
public:
    FVulkanGraphicsPipelineState(FVulkanDevice* InDevice);
    ~FVulkanGraphicsPipelineState();

    bool Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer);
    
    virtual void SetName(const FString& InName) override final
    {
        FVulkanPipeline::SetDebugName(InName);
    }

    FORCEINLINE FVulkanVertexShader*   GetVertexShader()   const { return VertexShader.Get(); }
    FORCEINLINE FVulkanHullShader*     GetHullShader()     const { return HullShader.Get(); }
    FORCEINLINE FVulkanDomainShader*   GetDomainShader()   const { return DomainShader.Get(); }
    FORCEINLINE FVulkanGeometryShader* GetGeometryShader() const { return GeometryShader.Get(); }
    FORCEINLINE FVulkanPixelShader*    GetPixelShader()    const { return PixelShader.Get(); }

    FORCEINLINE VkDescriptorSetLayout GetVkDescriptorSetLayout(EShaderVisibility ShaderVisibility) const { return DescriptorSetLayouts[ShaderVisibility]; }
    
private:
    TSharedRef<FVulkanVertexShader>   VertexShader;
    TSharedRef<FVulkanHullShader>     HullShader;
    TSharedRef<FVulkanDomainShader>   DomainShader;
    TSharedRef<FVulkanGeometryShader> GeometryShader;
    TSharedRef<FVulkanPixelShader>    PixelShader;
    
    VkDescriptorSetLayout DescriptorSetLayouts[5];
};


class FVulkanComputePipelineState : public FRHIComputePipelineState, public FVulkanPipeline
{
public:
    FVulkanComputePipelineState(FVulkanDevice* InDevice);
    ~FVulkanComputePipelineState();
    
    bool Initialize(const FRHIComputePipelineStateInitializer& Initializer);

    virtual void SetName(const FString& InName) override final
    {
        FVulkanPipeline::SetDebugName(InName);
    }

    FORCEINLINE FVulkanComputeShader* GetComputeShader() const { return Shader.Get(); }

    FORCEINLINE VkDescriptorSetLayout GetVkDescriptorSetLayout() const { return DescriptorSetLayout; }
    
private:
    TSharedRef<FVulkanComputeShader> Shader;
    VkDescriptorSetLayout DescriptorSetLayout;
};


class FVulkanRayTracingPipelineState : public FRHIRayTracingPipelineState
{
public:
    FVulkanRayTracingPipelineState()  = default;
    ~FVulkanRayTracingPipelineState() = default;
};
