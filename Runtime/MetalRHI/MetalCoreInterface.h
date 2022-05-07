#pragma once
#include "MetalBuffer.h"
#include "MetalTexture.h"
#include "MetalViews.h"
#include "MetalSamplerState.h"
#include "MetalViewport.h"
#include "MetalShader.h"
#include "MetalCommandContext.h"
#include "MetalTimestampQuery.h"
#include "MetalPipelineState.h"
#include "MetalRayTracing.h"
#include "MetalDevice.h"

#include "RHI/RHICoreInterface.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalCoreInterface

class CMetalCoreInterface final : public CRHICoreInterface
{
private:

	CMetalCoreInterface();
	~CMetalCoreInterface();

public:

	static CMetalCoreInterface* CreateMetalCoreInterface();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHICoreInterface Interface

	virtual bool Initialize(bool bEnableDebug) override final;

    virtual CRHITexture2D* RHICreateTexture2D(const CRHITexture2DInitializer& Initializer) override final
    {
        return dbg_new TMetalTexture<CMetalTexture2D>(Initializer);
    }

    virtual CRHITexture2DArray* RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer) override final
    {
        return dbg_new TMetalTexture<CMetalTexture2DArray>(Initializer);
    }

    virtual CRHITextureCube* RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer) override final
    {
        return dbg_new TMetalTexture<CMetalTextureCube>(Initializer);
    }

    virtual CRHITextureCubeArray* RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer) override final
    {
        return dbg_new TMetalTexture<CMetalTextureCubeArray>(Initializer);
    }

    virtual CRHITexture3D* RHICreateTexture3D(const CRHITexture3DInitializer& Initializer) override final
    {
        return dbg_new TMetalTexture<CMetalTexture3D>(Initializer);
    }

    virtual CRHISamplerState* RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer) override final
    {
        return dbg_new CMetalSamplerState();
    }

    virtual CRHIVertexBuffer* RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer) override final
    {
        return dbg_new TMetalBuffer<CMetalVertexBuffer>(Initializer);
    }

    virtual CRHIIndexBuffer* RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer) override final
    {
        return dbg_new TMetalBuffer<CMetalIndexBuffer>(Initializer);
    }

    virtual CRHIGenericBuffer* RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer) override final
    {
        return dbg_new TMetalBuffer<CMetalGenericBuffer>(Initializer);
    }

    virtual CRHIConstantBuffer* RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer) override final
    {
        return dbg_new TMetalBuffer<CMetalConstantBuffer>(Initializer);
    }

    virtual CRHIRayTracingScene* RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer) override final
    {
        return dbg_new CMetalRayTracingScene(Initializer);
    }

    virtual CRHIRayTracingGeometry* RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer) override final
    {
        return dbg_new CMetalRayTracingGeometry(Initializer);
    }

    virtual CRHIShaderResourceView* RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer) override final
    {
        return dbg_new CMetalShaderResourceView(Initializer.Texture);
    }

    virtual CRHIShaderResourceView* RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer) override final
    {
        return dbg_new CMetalShaderResourceView(Initializer.Buffer);
    }

    virtual CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer) override final
    {
        return dbg_new CMetalUnorderedAccessView(Initializer.Texture);
    }

    virtual CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer) override final
    {
        return dbg_new CMetalUnorderedAccessView(Initializer.Buffer);
    }

    virtual class CRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TMetalShader<CMetalComputeShader>();
    }

    virtual class CRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TMetalShader<CRHIVertexShader>();
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
        return dbg_new TMetalShader<CRHIPixelShader>();
    }

    virtual class CRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TMetalShader<CRHIRayGenShader>();
    }

    virtual class CRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TMetalShader<CRHIRayAnyHitShader>();
    }

    virtual class CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TMetalShader<CRHIRayClosestHitShader>();
    }

    virtual class CRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TMetalShader<CRHIRayMissShader>();
    }

    virtual class CRHIDepthStencilState* RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer) override final
    {
        return dbg_new CMetalDepthStencilState();
    }

    virtual class CRHIRasterizerState* RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer) override final
    {
        return dbg_new CMetalRasterizerState();
    }

    virtual class CRHIBlendState* RHICreateBlendState(const CRHIBlendStateInitializer& Initializer) override final
    {
        return dbg_new CMetalBlendState();
    }

    virtual class CRHIVertexInputLayout* RHICreateVertexInputLayout(const CRHIVertexInputLayoutInitializer& Initializer) override final
    {
        return dbg_new CMetalInputLayoutState();
    }

    virtual class CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Initializer) override final
    {
        return dbg_new CMetalGraphicsPipelineState();
    }

    virtual class CRHIComputePipelineState* RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer) override final
    {
        return dbg_new CMetalComputePipelineState();
    }

    virtual class CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Initializer) override final
    {
        return dbg_new CMetalRayTracingPipelineState();
    }

    virtual class CRHITimestampQuery* RHICreateTimestampQuery() override final
    {
        return dbg_new CMetalTimestampQuery();
    }

    virtual class CRHIViewport* RHICreateViewport(const CRHIViewportInitializer& Initializer) override final
    {
        return dbg_new CMetalViewport(Initializer);
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
	CMetalDevice*         Device;
    CMetalCommandContext* CommandContext;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
