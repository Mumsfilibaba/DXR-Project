#pragma once
#include "NullRHI.h"
#include "NullRHIResources.h"
#include "NullRHIShader.h"
#include "NullRHICommandContext.h"

#include "RHI/RHIInterface.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct NULLRHI_API FNullRHIInterfaceModule final
    : public FRHIInterfaceModule
{
    virtual FRHIInterface* CreateInterface() override final;
};


class NULLRHI_API FNullRHIInterface final 
    : public FRHIInterface
{
public:
    FNullRHIInterface()
        : FRHIInterface(ERHIInstanceType::Null)
        , CommandContext(new FNullRHICommandContext())
    { }

    ~FNullRHIInterface()
    {
        SAFE_DELETE(CommandContext);
    }

    virtual bool Initialize() override final
    { 
        return true; 
    }

    virtual FRHITexture* RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final
    {
        return new FNullRHITexture(InDesc);
    }

    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData) override final
    {
        return new FNullRHIBuffer(InDesc);
    }

    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateDesc& InDesc) override final
    {
        return new FNullRHISamplerState(InDesc);
    }

    virtual class FRHIViewport* RHICreateViewport(const FRHIViewportDesc& InDesc) override final
    {
        return new FNullRHIViewport(InDesc);
    }

    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc) override final
    {
        return new FNullRHIRayTracingScene(InDesc);
    }

    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc) override final
    {
        return new FNullRHIRayTracingGeometry(InDesc);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc) override final
    {
        return new FNullRHIShaderResourceView(InDesc.Texture);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc) override final
    {
        return new FNullRHIShaderResourceView(InDesc.Buffer);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc) override final
    {
        return new FNullRHIUnorderedAccessView(InDesc.Texture);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc) override final
    {
        return new FNullRHIUnorderedAccessView(InDesc.Buffer);
    }

    virtual class FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIComputeShader();
    }

    virtual class FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIVertexShader();
    }

    virtual class FRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class FRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIPixelShader();
    }

    virtual class FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return new FNullRHIRayGenShader();
    }

    virtual class FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayAnyHitShader>();
    }

    virtual class FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayClosestHitShader>();
    }

    virtual class FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final
    {
        return new TNullRHIShader<FRHIRayMissShader>();
    }

    virtual class FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final
    {
        return new FNullRHIDepthStencilState(InDesc);
    }

    virtual class FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateDesc& InDesc) override final
    {
        return new FNullRHIRasterizerState(InDesc);
    }

    virtual class FRHIBlendState* RHICreateBlendState(const FRHIBlendStateDesc& InDesc) override final
    {
        return new FNullRHIBlendState(InDesc);
    }

    virtual class FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& InDesc) override final
    {
        return new FNullRHIInputLayoutState();
    }

    virtual class FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc) override final
    {
        return new FNullRHIGraphicsPipelineState();
    }

    virtual class FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc) override final
    {
        return new FNullRHIComputePipelineState();
    }

    virtual class FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final
    {
        return new FNullRHIRayTracingPipelineState();
    }

    virtual class FRHITimestampQuery* RHICreateTimestampQuery() override final
    {
        return new FNullRHITimestampQuery();
    }

    virtual struct IRHICommandContext* RHIObtainCommandContext() override final
    {
        return CommandContext;
    }

    virtual FString RHIGetAdapterDescription() const override final
    {
        return FString();
    }

    virtual void RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport) const override final
    {
        OutSupport = FRHIRayTracingSupport();
    }

    virtual void RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport) const override final
    {
        OutSupport = FRHIShadingRateSupport();
    }

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final
    {
        return true;
    }

private:
    FNullRHICommandContext* CommandContext;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
