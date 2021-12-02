#pragma once
#include "RHI/RHIInterface.h"

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

class CNullRHIInterface final : public CRHIInterface
{
public:

    CNullRHIInterface()
        : CRHIInterface( ERHIModule::Null )
        , CommandContext( CNullRHICommandContext::Make() )
    {
    }

    ~CNullRHIInterface() = default;

    virtual bool Init( bool EnableDebug ) override final
    {
        return true;
    }

    virtual CRHITexture2D* CreateTexture2D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMips,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture2D>( Format, Width, Height, NumMips, NumSamples, Flags, OptimizedClearValue );
    }

    virtual CRHITexture2DArray* CreateTexture2DArray(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMips,
        uint32 NumSamples,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture2DArray>( Format, Width, Height, NumArraySlices, NumMips, NumSamples, Flags, OptimizedClearValue );
    }

    virtual CRHITextureCube* CreateTextureCube(
        EFormat Format,
        uint32 Size,
        uint32 NumMips,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final
    {
        return dbg_new TNullRHITexture<CNullRHITextureCube>( Format, Size, NumMips, Flags, OptimizedClearValue );
    }

    virtual CRHITextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        uint32 Size,
        uint32 NumMips,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final
    {
        return dbg_new TNullRHITexture<CNullRHITextureCubeArray>( Format, Size, NumArraySlices, NumMips, Flags, OptimizedClearValue );
    }

    virtual CRHITexture3D* CreateTexture3D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 Depth,
        uint32 NumMips,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final
    {
        return dbg_new TNullRHITexture<CNullRHITexture3D>( Format, Width, Height, Depth, NumMips, Flags, OptimizedClearValue );
    }

    virtual class CRHISamplerState* CreateSamplerState( const struct SSamplerStateCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHISamplerState();
    }

    virtual CRHIVertexBuffer* CreateVertexBuffer( uint32 Stride, uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIVertexBuffer>( NumVertices, Stride, Flags );
    }

    virtual CRHIIndexBuffer* CreateIndexBuffer( EIndexFormat Format, uint32 NumIndices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIIndexBuffer>( Format, NumIndices, Flags );
    }

    virtual CRHIConstantBuffer* CreateConstantBuffer( uint32 Size, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIConstantBuffer>( Size, Flags );
    }

    virtual CRHIStructuredBuffer* CreateStructuredBuffer( uint32 Stride, uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) override final
    {
        return dbg_new TNullRHIBuffer<CNullRHIStructuredBuffer>( Stride * NumElements, Stride, Flags );
    }

    virtual CRHIRayTracingScene* CreateRayTracingScene( uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances ) override final
    {
        return dbg_new CNullRHIRayTracingScene( Flags );
    }

    virtual CRHIRayTracingGeometry* CreateRayTracingGeometry( uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer ) override final
    {
        return dbg_new CNullRHIRayTracingGeometry( Flags );
    }

    virtual CRHIShaderResourceView* CreateShaderResourceView( const SShaderResourceViewCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIShaderResourceView();
    }

    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView( const SUnorderedAccessViewCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIUnorderedAccessView();
    }

    virtual CRHIRenderTargetView* CreateRenderTargetView( const SRenderTargetViewCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIRenderTargetView();
    }

    virtual CRHIDepthStencilView* CreateDepthStencilView( const SDepthStencilViewCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIDepthStencilView();
    }

    virtual class CRHIComputeShader* CreateComputeShader( const TArray<uint8>& ShaderCode ) override final
    {
        return dbg_new TNullRHIShader<CNullRHIComputeShader>();
    }

    virtual class CRHIVertexShader* CreateVertexShader( const TArray<uint8>& ShaderCode ) override final
    {
        return dbg_new TNullRHIShader<CRHIVertexShader>();
    }

    virtual class CRHIHullShader* CreateHullShader( const TArray<uint8>& ShaderCode ) override final
    {
        return nullptr;
    }

    virtual class CRHIDomainShader* CreateDomainShader( const TArray<uint8>& ShaderCode ) override final
    {
        return nullptr;
    }

    virtual class CRHIGeometryShader* CreateGeometryShader( const TArray<uint8>& ShaderCode ) override final
    {
        return nullptr;
    }

    virtual class CRHIMeshShader* CreateMeshShader( const TArray<uint8>& ShaderCode ) override final
    {
        return nullptr;
    }

    virtual class CRHIAmplificationShader* CreateAmplificationShader( const TArray<uint8>& ShaderCode ) override final
    {
        return nullptr;
    }

    virtual class CRHIPixelShader* CreatePixelShader( const TArray<uint8>& ShaderCode ) override final
    {
        return dbg_new TNullRHIShader<CRHIPixelShader>();
    }

    virtual class CRHIRayGenShader* CreateRayGenShader( const TArray<uint8>& ShaderCode ) override final
    {
        return dbg_new TNullRHIShader<CRHIRayGenShader>();
    }

    virtual class CRHIRayAnyHitShader* CreateRayAnyHitShader( const TArray<uint8>& ShaderCode ) override final
    {
        return dbg_new TNullRHIShader<CRHIRayAnyHitShader>();
    }

    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader( const TArray<uint8>& ShaderCode ) override final
    {
        return dbg_new TNullRHIShader<CRHIRayClosestHitShader>();
    }

    virtual class CRHIRayMissShader* CreateRayMissShader( const TArray<uint8>& ShaderCode ) override final
    {
        return dbg_new TNullRHIShader<CRHIRayMissShader>();
    }

    virtual class CRHIDepthStencilState* CreateDepthStencilState( const SDepthStencilStateCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIDepthStencilState();
    }

    virtual class CRHIRasterizerState* CreateRasterizerState( const SRasterizerStateCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIRasterizerState();
    }

    virtual class CRHIBlendState* CreateBlendState( const SBlendStateCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIBlendState();
    }

    virtual class CRHIInputLayoutState* CreateInputLayout( const SInputLayoutStateCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIInputLayoutState();
    }

    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState( const SGraphicsPipelineStateCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIGraphicsPipelineState();
    }

    virtual class CRHIComputePipelineState* CreateComputePipelineState( const SComputePipelineStateCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIComputePipelineState();
    }

    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState( const SRayTracingPipelineStateCreateInfo& CreateInfo ) override final
    {
        return dbg_new CNullRHIRayTracingPipelineState();
    }

    virtual class CRHITimestampQuery* CreateTimestampQuery() override final
    {
        return dbg_new CNullRHITimestampQuery();
    }

    virtual class CRHIViewport* CreateViewport( CPlatformWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat ) override final
    {
        return dbg_new CNullRHIViewport( ColorFormat, Width, Height );
    }

    virtual class IRHICommandContext* GetDefaultCommandContext() override final
    {
        return CommandContext.Get();
    }

    virtual CString GetAdapterName() const override final
    {
        return CString();
    }

    virtual void CheckRayTracingSupport( SRayTracingSupport& OutSupport ) const override final
    {
        OutSupport = SRayTracingSupport();
    }

    virtual void CheckShadingRateSupport( SShadingRateSupport& OutSupport ) const override final
    {
        OutSupport = SShadingRateSupport();
    }

    virtual bool UAVSupportsFormat( EFormat Format ) const override final
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
