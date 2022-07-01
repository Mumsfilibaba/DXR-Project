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
// CNullRHICoreInterface

class CNullRHICoreInterface final : public FRHICoreInterface
{
private:

    CNullRHICoreInterface()
        : FRHICoreInterface(ERHIInstanceType::Null)
        , CommandContext(CNullRHICommandContext::CreateNullRHIContext())
    { }

    ~CNullRHICoreInterface()
    {
        SafeDelete(CommandContext);
    }

public:

    static CNullRHICoreInterface* CreateNullRHICoreInterface() { return dbg_new CNullRHICoreInterface(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHICoreInterface Interface

    virtual bool Initialize(bool bEnableDebug) override final { return true; }

    virtual FRHITexture2D* RHICreateTexture2D(const FRHITexture2DInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture2D>(Initializer);
    }

    virtual FRHITexture2DArray* RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture2DArray>(Initializer);
    }

    virtual FRHITextureCube* RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITextureCube>(Initializer);
    }

    virtual FRHITextureCubeArray* RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITextureCubeArray>(Initializer);
    }

    virtual FRHITexture3D* RHICreateTexture3D(const FRHITexture3DInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture3D>(Initializer);
    }

    virtual FRHISamplerState* RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHISamplerState();
    }

    virtual FRHIVertexBuffer* RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIVertexBuffer>(Initializer);
    }

    virtual FRHIIndexBuffer* RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIIndexBuffer>(Initializer);
    }

    virtual FRHIGenericBuffer* RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIGenericBuffer>(Initializer);
    }

    virtual FRHIConstantBuffer* RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIConstantBuffer>(Initializer);
    }

    virtual FRHIRayTracingScene* RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer) override final
    {
        return dbg_new CNullRHIRayTracingScene(Initializer);
    }

    virtual FRHIRayTracingGeometry* RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer) override final
    {
        return dbg_new CNullRHIRayTracingGeometry(Initializer);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer) override final
    {
        return dbg_new CNullRHIShaderResourceView(Initializer.Texture);
    }

    virtual FRHIShaderResourceView* RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer) override final
    {
        return dbg_new CNullRHIShaderResourceView(Initializer.Buffer);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer) override final
    {
        return dbg_new CNullRHIUnorderedAccessView(Initializer.Texture);
    }

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer) override final
    {
        return dbg_new CNullRHIUnorderedAccessView(Initializer.Buffer);
    }

    virtual class FRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CNullRHIComputeShader>();
    }

    virtual class FRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<FRHIVertexShader>();
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
        return dbg_new TNullRHIShader<FRHIPixelShader>();
    }

    virtual class FRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<FRHIRayGenShader>();
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
        return dbg_new CNullRHIDepthStencilState();
    }

    virtual class FRHIRasterizerState* RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIRasterizerState();
    }

    virtual class FRHIBlendState* RHICreateBlendState(const FRHIBlendStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIBlendState();
    }

    virtual class FRHIVertexInputLayout* RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer) override final
    {
        return dbg_new CNullRHIInputLayoutState();
    }

    virtual class FRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIGraphicsPipelineState();
    }

    virtual class FRHIComputePipelineState* RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIComputePipelineState();
    }

    virtual class FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIRayTracingPipelineState();
    }

    virtual class FRHITimestampQuery* RHICreateTimestampQuery() override final
    {
        return dbg_new CNullRHITimestampQuery();
    }

    virtual class FRHIViewport* RHICreateViewport(const FRHIViewportInitializer& Initializer) override final
    {
        return dbg_new CNullRHIViewport(Initializer);
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
    CNullRHICommandContext* CommandContext;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
