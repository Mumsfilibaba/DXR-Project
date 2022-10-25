#pragma once
#include "NullRHI.h"
#include "NullRHIResources.h"
#include "NullRHIShader.h"
#include "NullRHICommandContext.h"

#include "RHI/RHIInterface.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

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
        , CommandContext(dbg_new FNullRHICommandContext())
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
        return dbg_new FNullRHITexture(InDesc);
    }

    virtual FRHIBuffer* RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData) override final
    {
        return dbg_new FNullRHIBuffer(InDesc);
    }

    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateDesc& InDesc) override final
    {
        return dbg_new FNullRHISamplerState(InDesc);
    }

    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc) override final
    {
        return dbg_new FNullRHIRayTracingScene(InDesc);
    }

    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc) override final
    {
        return dbg_new FNullRHIRayTracingGeometry(InDesc);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc) override final
    {
        return dbg_new FNullRHIShaderResourceView(InDesc.Texture);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc) override final
    {
        return dbg_new FNullRHIShaderResourceView(InDesc.Buffer);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc) override final
    {
        return dbg_new FNullRHIUnorderedAccessView(InDesc.Texture);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc) override final
    {
        return dbg_new FNullRHIUnorderedAccessView(InDesc.Buffer);
    }

    virtual class FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new FNullRHIComputeShader();
    }

    virtual class FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new FNullRHIVertexShader();
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
        return dbg_new FNullRHIPixelShader();
    }

    virtual class FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new FNullRHIRayGenShader();
    }

    virtual class FRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<FRHIRayAnyHitShader>();
    }

    virtual class FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<FRHIRayClosestHitShader>();
    }

    virtual class FRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<FRHIRayMissShader>();
    }

    virtual class FRHIDepthStencilState* RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer) override final
    {
        return dbg_new FNullRHIDepthStencilState();
    }

    virtual class FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer) override final
    {
        return dbg_new FNullRHIRasterizerState();
    }

    virtual class FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& Initializer) override final
    {
        return dbg_new FNullRHIBlendState();
    }

    virtual class FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer) override final
    {
        return dbg_new FNullRHIInputLayoutState();
    }

    virtual class FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer) override final
    {
        return dbg_new FNullRHIGraphicsPipelineState();
    }

    virtual class FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer) override final
    {
        return dbg_new FNullRHIComputePipelineState();
    }

    virtual class FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer) override final
    {
        return dbg_new FNullRHIRayTracingPipelineState();
    }

    virtual class FRHITimestampQuery* RHICreateTimestampQuery() override final
    {
        return dbg_new FNullRHITimestampQuery();
    }

    virtual class FRHIViewport* RHICreateViewport(const FRHIViewportDesc& InDesc) override final
    {
        return dbg_new FNullRHIViewport(InDesc);
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

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
