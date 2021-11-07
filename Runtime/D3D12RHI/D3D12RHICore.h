#pragma once
#include "D3D12RHIModule.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12RHICommandContext.h"
#include "D3D12RHITexture.h"

#include "RHI/RHICore.h"

#include "CoreApplication/Windows/WindowsWindow.h"

class CD3D12RHICommandContext;

///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TD3D12Texture>
D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension();

template<typename TD3D12Texture>
bool IsTextureCube();

///////////////////////////////////////////////////////////////////////////////////////////////////

class CD3D12RHICore : public CRHICore
{
public:

    /* Make a new RHI core */
    static FORCEINLINE CRHICore* Make()
    {
        return dbg_new CD3D12RHICore();
    }

    /* Init the RHI Core, create device etc. */
    virtual bool Init( bool EnableDebug ) override final;

    virtual CRHITexture2D* CreateTexture2D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMipLevels,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final;

    virtual CRHITexture2DArray* CreateTexture2DArray(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMipLevels,
        uint32 NumSamples,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final;

    virtual CRHITextureCube* CreateTextureCube(
        EFormat Format,
        uint32 Size,
        uint32 NumMipLevels,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final;

    virtual CRHITextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        uint32 Size,
        uint32 NumMipLevels,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final;

    virtual CRHITexture3D* CreateTexture3D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 Depth,
        uint32 NumMipLevels,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) override final;

    virtual class CRHISamplerState* CreateSamplerState( const struct SSamplerStateCreateInfo& CreateInfo ) override final;

    virtual CRHIVertexBuffer* CreateVertexBuffer( uint32 Stride, uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) override final;
    virtual CRHIIndexBuffer* CreateIndexBuffer( EIndexFormat Format, uint32 NumIndices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) override final;
    virtual CRHIConstantBuffer* CreateConstantBuffer( uint32 Size, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) override final;
    virtual CRHIStructuredBuffer* CreateStructuredBuffer( uint32 Stride, uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) override final;

    virtual class CRHIRayTracingScene* CreateRayTracingScene( uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances ) override final;
    virtual class CRHIRayTracingGeometry* CreateRayTracingGeometry( uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer ) override final;

    virtual CRHIShaderResourceView* CreateShaderResourceView( const SShaderResourceViewCreateInfo& CreateInfo ) override final;
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView( const SUnorderedAccessViewCreateInfo& CreateInfo ) override final;
    virtual CRHIRenderTargetView* CreateRenderTargetView( const SRenderTargetViewCreateInfo& CreateInfo ) override final;
    virtual CRHIDepthStencilView* CreateDepthStencilView( const SDepthStencilViewCreateInfo& CreateInfo ) override final;

    virtual class CRHIComputeShader* CreateComputeShader( const TArray<uint8>& ShaderCode ) override final;

    virtual class CRHIVertexShader* CreateVertexShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIHullShader* CreateHullShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIDomainShader* CreateDomainShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIGeometryShader* CreateGeometryShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIMeshShader* CreateMeshShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIAmplificationShader* CreateAmplificationShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIPixelShader* CreatePixelShader( const TArray<uint8>& ShaderCode ) override final;

    virtual class CRHIRayGenShader* CreateRayGenShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIRayAnyHitShader* CreateRayAnyHitShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class CRHIRayMissShader* CreateRayMissShader( const TArray<uint8>& ShaderCode ) override final;

    virtual class CRHIDepthStencilState* CreateDepthStencilState( const SDepthStencilStateCreateInfo& CreateInfo ) override final;
    virtual class CRHIRasterizerState* CreateRasterizerState( const SRasterizerStateCreateInfo& CreateInfo ) override final;
    virtual class CRHIBlendState* CreateBlendState( const SBlendStateCreateInfo& CreateInfo ) override final;
    virtual class CRHIInputLayoutState* CreateInputLayout( const SInputLayoutStateCreateInfo& CreateInfo ) override final;

    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState( const SGraphicsPipelineStateCreateInfo& CreateInfo ) override final;
    virtual class CRHIComputePipelineState* CreateComputePipelineState( const SComputePipelineStateCreateInfo& CreateInfo ) override final;
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState( const SRayTracingPipelineStateCreateInfo& CreateInfo ) override final;

    virtual class CRHITimestampQuery* CreateTimestampQuery() override final;

    virtual class CRHIViewport* CreateViewport( CPlatformWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat ) override final;

    // TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
    virtual bool UAVSupportsFormat( EFormat Format ) const override final;

    virtual class IRHICommandContext* GetDefaultCommandContext() override final
    {
        return DirectCmdContext.Get();
    }

    virtual CString GetAdapterName() const override final
    {
        return Device->GetAdapterName();
    }

    virtual void CheckRayTracingSupport( SRayTracingSupport& OutSupport ) const override final;
    virtual void CheckShadingRateSupport( SShadingRateSupport& OutSupport ) const override final;

    FORCEINLINE CD3D12OfflineDescriptorHeap* GetResourceOfflineDescriptorHeap()
    {
        return ResourceOfflineDescriptorHeap;
    }

    FORCEINLINE CD3D12OfflineDescriptorHeap* GetRenderTargetOfflineDescriptorHeap()
    {
        return RenderTargetOfflineDescriptorHeap;
    }

    FORCEINLINE CD3D12OfflineDescriptorHeap* GetDepthStencilOfflineDescriptorHeap()
    {
        return DepthStencilOfflineDescriptorHeap;
    }

    FORCEINLINE CD3D12OfflineDescriptorHeap* GetSamplerOfflineDescriptorHeap()
    {
        return SamplerOfflineDescriptorHeap;
    }

private:

    CD3D12RHICore();
    ~CD3D12RHICore();

    template<typename TD3D12Texture>
    TD3D12Texture* CreateTexture(
        EFormat Format,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 NumMips,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitialData,
        const SClearValue& OptimalClearValue );

    template<typename TD3D12Buffer>
    bool FinalizeBufferResource( TD3D12Buffer* Buffer, uint32 SizeInBytes, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData );

    CD3D12Device* Device;
    TSharedRef<CD3D12RHICommandContext> DirectCmdContext;
    CD3D12RootSignatureCache* RootSignatureCache;

    CD3D12OfflineDescriptorHeap* ResourceOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* RenderTargetOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* DepthStencilOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* SamplerOfflineDescriptorHeap = nullptr;
};

extern CD3D12RHICore* GD3D12RHICore;
