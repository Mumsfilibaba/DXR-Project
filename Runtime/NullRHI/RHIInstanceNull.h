#pragma once
#include "RHI/RHIInstance.h"

#include "NullRHIBuffer.h"
#include "NullRHITexture.h"
#include "NullRHIResourceView.h"
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
// CRHIInstanceNull

class CRHIInstanceNull final : public CRHIInstance
{
public:

    CRHIInstanceNull()
        : CRHIInstance(ERHIType::Null)
        , CommandContext(CNullRHICommandContext::Make())
    { }

    ~CRHIInstanceNull() = default;

    virtual bool Initialize(bool bEnableDebug) override final { return true; }

    virtual CRHITexture2D* CreateTexture2D(ERHIFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 Flags, ERHIResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture2D>(Format, Width, Height, NumMips, NumSamples, Flags, OptimizedClearValue);
    }

    virtual CRHITexture2DArray* CreateTexture2DArray(ERHIFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture2DArray>(Format, Width, Height, NumArraySlices, NumMips, NumSamples, Flags, OptimizedClearValue);
    }

    virtual CRHITextureCube* CreateTextureCube(ERHIFormat Format, uint32 Size, uint32 NumMips, uint32 Flags, ERHIResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final
    {
        return dbg_new TNullRHITexture<CNullRHITextureCube>(Format, Size, NumMips, Flags, OptimizedClearValue);
    }

    virtual CRHITextureCubeArray* CreateTextureCubeArray(ERHIFormat Format, uint32 Size, uint32 NumMips, uint32 NumArraySlices, uint32 Flags, ERHIResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final
    {
        return dbg_new TNullRHITexture<CNullRHITextureCubeArray>(Format, Size, NumArraySlices, NumMips, Flags, OptimizedClearValue);
    }

    virtual CRHITexture3D* CreateTexture3D(ERHIFormat Format, uint32 Width, uint32 Height, uint32 Depth, uint32 NumMips, uint32 Flags, ERHIResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture3D>(Format, Width, Height, Depth, NumMips, Flags, OptimizedClearValue);
    }

    virtual CRHIBufferRef CreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceAccess InitialState, const SRHIResourceData* InitalData) override final
    {
        return dbg_new CNullRHIBuffer(BufferDesc);
    }

    virtual CRHISamplerStateRef CreateSamplerState(const class CRHISamplerStateDesc& SamplerDesc) override final
    {
        return dbg_new CNullRHISamplerState();
    }

    virtual CRHIRayTracingScene* CreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances) override final
    {
        return dbg_new CNullRHIRayTracingScene(Flags);
    }

    virtual CRHIRayTracingGeometry* CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices) override final
    {
        return dbg_new CNullRHIRayTracingGeometry(Flags);
    }

    virtual CRHIShaderResourceView* CreateShaderResourceView(const SRHIShaderResourceViewDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIShaderResourceView();
    }

    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SRHIUnorderedAccessViewDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIUnorderedAccessView();
    }

    virtual CRHIRenderTargetView* CreateRenderTargetView(const SRHIRenderTargetViewDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIRenderTargetView();
    }

    virtual CRHIDepthStencilView* CreateDepthStencilView(const SRHIDepthStencilViewDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIDepthStencilView();
    }

    virtual class CRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CNullRHIComputeShader>();
    }

    virtual class CRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIVertexShader>();
    }

    virtual class CRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final
    {
        return nullptr;
    }

    virtual class CRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIPixelShader>();
    }

    virtual class CRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIRayGenShader>();
    }

    virtual class CRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIRayAnyHitShader>();
    }

    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIRayClosestHitShader>();
    }

    virtual class CRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) override final
    {
        return dbg_new TNullRHIShader<CRHIRayMissShader>();
    }

    virtual class CRHIDepthStencilState* CreateDepthStencilState(const SRHIDepthStencilStateDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIDepthStencilState();
    }

    virtual class CRHIRasterizerState* CreateRasterizerState(const SRHIRasterizerStateDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIRasterizerState();
    }

    virtual class CRHIBlendState* CreateBlendState(const SRHIBlendStateDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIBlendState();
    }

    virtual class CRHIInputLayoutState* CreateInputLayout(const SRHIInputLayoutStateDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIInputLayoutState();
    }

    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIGraphicsPipelineState();
    }

    virtual class CRHIComputePipelineState* CreateComputePipelineState(const SRHIComputePipelineStateDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIComputePipelineState();
    }

    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& CreateInfo) override final
    {
        return dbg_new CNullRHIRayTracingPipelineState();
    }

    virtual class CRHITimestampQuery* CreateTimestampQuery() override final
    {
        return dbg_new CNullRHITimestampQuery();
    }

    virtual CRHIViewportRef CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat) override final
    {
        return dbg_new CNullRHIViewport(ColorFormat, Width, Height);
    }

    virtual class IRHICommandContext* GetDefaultCommandContext() override final
    {
        return CommandContext.Get();
    }

    virtual String GetAdapterName() const override final
    {
        return String();
    }

    virtual void CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const override final
    {
        OutSupport = SRHIRayTracingSupport();
    }

    virtual void CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const override final
    {
        OutSupport = SRHIShadingRateSupport();
    }

    virtual bool UAVSupportsFormat(ERHIFormat Format) const override final
    {
        return true;
    }

private:
    TSharedRef<CNullRHICommandContext> CommandContext;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
