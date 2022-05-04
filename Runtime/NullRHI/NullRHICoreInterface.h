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

class CNullRHICoreInterface final : public CRHICoreInterface
{
public:

    CNullRHICoreInterface()
        : CRHICoreInterface(ERHIInstanceType::Null)
        , CommandContext(CNullRHICommandContext::CreateNullRHIContext())
    { }

    ~CNullRHICoreInterface()
    {
        SafeDelete(CommandContext);
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHICoreInterface Interface

    virtual bool Initialize(bool bEnableDebug) override final { return true; }

    virtual CRHITexture2D* RHICreateTexture2D(const CRHITexture2DInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture2D>(Initializer);
    }

    virtual CRHITexture2DArray* RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture2DArray>(Initializer);
    }

    virtual CRHITextureCube* RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITextureCube>(Initializer);
    }

    virtual CRHITextureCubeArray* RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITextureCubeArray>(Initializer);
    }

    virtual CRHITexture3D* RHICreateTexture3D(const CRHITexture3DInitializer& Initializer) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture3D>(Initializer);
    }

    virtual CRHISamplerState* RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHISamplerState();
    }

    virtual CRHIVertexBuffer* RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIVertexBuffer>(Initializer);
    }

    virtual CRHIIndexBuffer* RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIIndexBuffer>(Initializer);
    }

    virtual CRHIGenericBuffer* RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIGenericBuffer>(Initializer);
    }

    virtual CRHIConstantBuffer* RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIConstantBuffer>(Initializer);
    }

    virtual CRHIRayTracingScene* RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer) override final
    {
        return dbg_new CNullRHIRayTracingScene(Initializer);
    }

    virtual CRHIRayTracingGeometry* RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer) override final
    {
        return dbg_new CNullRHIRayTracingGeometry(Initializer);
    }

    virtual CRHIShaderResourceView* RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer) override final
    {
        return dbg_new CNullRHIShaderResourceView(Initializer.Texture);
    }

    virtual CRHIShaderResourceView* RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer) override final
    {
        return dbg_new CNullRHIShaderResourceView(Initializer.Buffer);
    }

    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo) override final
    {
        return dbg_new CNullRHIUnorderedAccessView();
    }

    virtual CRHIRenderTargetView* CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo) override final
    {
        return dbg_new CNullRHIRenderTargetView();
    }

    virtual CRHIDepthStencilView* CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo) override final
    {
        return dbg_new CNullRHIDepthStencilView();
    }

    virtual class CRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CNullRHIComputeShader>();
    }

    virtual class CRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIVertexShader>();
    }

    virtual class CRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIPixelShader>();
    }

    virtual class CRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIRayGenShader>();
    }

    virtual class CRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIRayAnyHitShader>();
    }

    virtual class CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIRayClosestHitShader>();
    }

    virtual class CRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIRayMissShader>();
    }

    virtual class CRHIDepthStencilState* RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIDepthStencilState();
    }

    virtual class CRHIRasterizerState* RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIRasterizerState();
    }

    virtual class CRHIBlendState* RHICreateBlendState(const CRHIBlendStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIBlendState();
    }

    virtual class CRHIVertexInputLayout* RHICreateVertexInputLayout(const CRHIVertexInputLayoutInitializer& Initializer) override final
    {
        return dbg_new CNullRHIInputLayoutState();
    }

    virtual class CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIGraphicsPipelineState();
    }

    virtual class CRHIComputePipelineState* RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIComputePipelineState();
    }

    virtual class CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Initializer) override final
    {
        return dbg_new CNullRHIRayTracingPipelineState();
    }

    virtual class CRHITimestampQuery* RHICreateTimestampQuery() override final
    {
        return dbg_new CNullRHITimestampQuery();
    }

    virtual class CRHIViewport* RHICreateViewport(const CRHIViewportInitializer& Initializer) override final
    {
        return dbg_new CNullRHIViewport(Initializer);
    }

    virtual class IRHICommandContext* RHIGetDefaultCommandContext() override final
    {
        return CommandContext;
    }

    virtual String GetAdapterName() const override final
    {
        return String();
    }

    virtual void RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport) const override final
    {
        OutSupport = SRayTracingSupport();
    }

    virtual void RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport) const override final
    {
        OutSupport = SShadingRateSupport();
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
