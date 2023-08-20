#pragma once
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanCommandContext.h"
#include "VulkanQueue.h"
#include "RHI/RHI.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"

struct VULKANRHI_API FVulkanRHIModule final : public FRHIModule
{
    virtual class FRHI* CreateRHI() override final;
};

class VULKANRHI_API FVulkanRHI : public FRHI
{
public:
    FVulkanRHI();
    ~FVulkanRHI();

    static FVulkanRHI* GetRHI() 
    {
        CHECK(GVulkanRHI != nullptr);
        return GVulkanRHI; 
    }

    virtual bool Initialize() override final;

    virtual FRHITexture* RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    
    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData) override final;

    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateDesc& InDesc) override final;
    
    virtual FRHIViewport* RHICreateViewport(const FRHIViewportDesc& InDesc) override final;

    virtual FRHITimestampQuery* RHICreateTimestampQuery() override final;
    
    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc) override final;

    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc) override final;

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc) override final;
    
    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc) override final;

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc) override final;

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc) override final;

    virtual FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    
    virtual FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) override final;
    
    virtual FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    
    virtual FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    
    virtual FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final;
    
    virtual FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    
    virtual FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    
    virtual FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final;
    
    virtual FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final;
    
    virtual FRHIBlendState* RHICreateBlendState(const FRHIBlendStateDesc& InDesc) override final;
    
    virtual FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& InDesc) override final;

    virtual FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final;

    virtual FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final;
    
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final;

    virtual IRHICommandContext* RHIObtainCommandContext() override final { return GraphicsCommandContext.Get(); }

    virtual void RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport) const override final;
    
    virtual void RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport) const override final;
    
    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;

    virtual FString RHIGetAdapterName() const override final;

    virtual void* RHIGetAdapter() override final 
    {
        CHECK(PhysicalDevice != nullptr);
        return reinterpret_cast<void*>(PhysicalDevice->GetVkPhysicalDevice());
    }

    virtual void* RHIGetDevice() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetVkDevice());
    }

    virtual void* RHIGetDirectCommandQueue() override final
    {
        CHECK(GraphicsCommandQueue != nullptr);
        return reinterpret_cast<void*>(GraphicsCommandQueue->GetVkQueue());
    }

    virtual void* RHIGetComputeCommandQueue() override final
    {
        // TODO: Finish
        return nullptr;
    }

    virtual void* RHIGetCopyCommandQueue() override final
    {
        // TODO: Finish
        return nullptr;
    }

public:
    FORCEINLINE FVulkanInstance* GetInstance() const
    {
        return Instance.Get();
    }

    FORCEINLINE FVulkanPhysicalDevice* GetAdapter() const
    {
        return PhysicalDevice.Get();
    }

    FORCEINLINE FVulkanDevice* GetDevice() const
    {
        return Device.Get();
    }

private:
    TSharedRef<FVulkanInstance>       Instance;
    TSharedRef<FVulkanPhysicalDevice> PhysicalDevice;
    TSharedRef<FVulkanDevice>         Device;

    TSharedRef<FVulkanQueue>          GraphicsCommandQueue;
    TSharedRef<FVulkanCommandContext> GraphicsCommandContext;

    static FVulkanRHI* GVulkanRHI;
};
