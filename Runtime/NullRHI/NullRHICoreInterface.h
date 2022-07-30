#pragma once
#include "RHI/RHICoreInterface.h"

#include "NullRHIBuffer.h"
#include "NullRHITexture.h"
#include "NullRHIViews.h"
#include "NullRHISamplerState.h"
#include "NullRHIViewport.h"
#include "NullRHIShader.h"
#include "NullRHICommandContext.h"
#include "NullRHITimestampQuery.h"
#include "NullRHIPipelineState.h"
#include "NullRHIRayTracing.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHICoreInterface

class FNullRHICoreInterface final 
    : public FRHICoreInterface
{
public:
    FNullRHICoreInterface()
        : FRHICoreInterface(ERHIInstanceType::Null)
        , CommandContext(FNullRHICommandContext::CreateNullRHIContext())
    { }

    ~FNullRHICoreInterface()
    {
        SAFE_DELETE(CommandContext);
    }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHICoreInterface Interface

    virtual bool Initialize(bool bEnableDebug) override final { return true; }

    virtual FRHITexture2D* RHICreateTexture2D(const FRHITexture2DInitializer& Initializer) override final
    {
        return dbg_new FNullRHITexture2D(Initializer);
    }

    virtual FRHITexture2DArray* RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer) override final
    {
        return dbg_new FNullRHITexture2DArray(Initializer);
    }

    virtual FRHITextureCube* RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer) override final
    {
        return dbg_new FNullRHITextureCube(Initializer);
    }

    virtual FRHITextureCubeArray* RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer) override final
    {
        return dbg_new FNullRHITextureCubeArray(Initializer);
    }

    virtual FRHITexture3D* RHICreateTexture3D(const FRHITexture3DInitializer& Initializer) override final
    {
        return dbg_new FNullRHITexture3D(Initializer);
    }

    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer) override final
    {
        return dbg_new FNullRHISamplerState();
    }

    virtual FRHIVertexBuffer* RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer) override final
    {
        return dbg_new FNullRHIVertexBuffer(Initializer);
    }

    virtual FRHIIndexBuffer* RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer) override final
    {
        return dbg_new FNullRHIIndexBuffer(Initializer);
    }

    virtual FRHIGenericBuffer* RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer) override final
    {
        return dbg_new FNullRHIGenericBuffer(Initializer);
    }

    virtual FRHIConstantBuffer* RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer) override final
    {
        return dbg_new FNullRHIConstantBuffer(Initializer);
    }

    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer) override final
    {
        return dbg_new FNullRHIRayTracingScene(Initializer);
    }

    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer) override final
    {
        return dbg_new FNullRHIRayTracingGeometry(Initializer);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer) override final
    {
        return dbg_new FNullRHIShaderResourceView(Initializer.Texture);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer) override final
    {
        return dbg_new FNullRHIShaderResourceView(Initializer.Buffer);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer) override final
    {
        return dbg_new FNullRHIUnorderedAccessView(Initializer.Texture);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer) override final
    {
        return dbg_new FNullRHIUnorderedAccessView(Initializer.Buffer);
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

    virtual class FRHIViewport* RHICreateViewport(const FRHIViewportInitializer& Initializer) override final
    {
        return dbg_new FNullRHIViewport(Initializer);
    }

    virtual class IRHICommandContext* RHIGetDefaultCommandContext() override final
    {
        return CommandContext;
    }

    virtual FString GetAdapterDescription() const override final
    {
        return FString();
    }

    virtual void RHIQueryRayTracingSupport(FRayTracingSupport& OutSupport) const override final
    {
        OutSupport = FRayTracingSupport();
    }

    virtual void RHIQueryShadingRateSupport(FShadingRateSupport& OutSupport) const override final
    {
        OutSupport = FShadingRateSupport();
    }

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final
    {
        return true;
    }

private:
    FNullRHICommandContext* CommandContext;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
