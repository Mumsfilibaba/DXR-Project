#pragma once
#include "RenderLayer/GenericRenderLayer.h"

#include "Core/Application/Windows/WindowsWindow.h"

#include "D3D12Device.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"
#include "D3D12RootSignature.h"

class D3D12CommandContext;
class D3D12Buffer;

template<typename TD3D12Texture>
D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension();

template<typename TD3D12Texture>
bool IsTextureCube();

class D3D12RenderLayer : public GenericRenderLayer
{
public:
    D3D12RenderLayer();
    ~D3D12RenderLayer();

    FORCEINLINE D3D12OfflineDescriptorHeap* GetResourceOfflineDescriptorHeap()
    {
        return ResourceOfflineDescriptorHeap;
    }

    FORCEINLINE D3D12OfflineDescriptorHeap* GetRenderTargetOfflineDescriptorHeap()
    {
        return RenderTargetOfflineDescriptorHeap;
    }

    FORCEINLINE D3D12OfflineDescriptorHeap* GetDepthStencilOfflineDescriptorHeap()
    {
        return DepthStencilOfflineDescriptorHeap;
    }

    FORCEINLINE D3D12OfflineDescriptorHeap* GetSamplerOfflineDescriptorHeap()
    {
        return SamplerOfflineDescriptorHeap;
    }

    virtual bool Init( bool EnableDebug ) override final;

    virtual Texture2D* CreateTexture2D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMipLevels,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue ) override final;

    virtual Texture2DArray* CreateTexture2DArray(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMipLevels,
        uint32 NumSamples,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue ) override final;

    virtual TextureCube* CreateTextureCube(
        EFormat Format,
        uint32 Size,
        uint32 NumMipLevels,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue ) override final;

    virtual TextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        uint32 Size,
        uint32 NumMipLevels,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue ) override final;

    virtual Texture3D* CreateTexture3D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 Depth,
        uint32 NumMipLevels,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue ) override final;

    virtual class SamplerState* CreateSamplerState( const struct SamplerStateCreateInfo& CreateInfo ) override final;

    virtual VertexBuffer* CreateVertexBuffer(
        uint32 Stride,
        uint32 NumVertices,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData ) override final;

    virtual IndexBuffer* CreateIndexBuffer(
        EIndexFormat Format,
        uint32 NumIndices,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData ) override final;

    virtual ConstantBuffer* CreateConstantBuffer(
        uint32 Size,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData ) override final;

    virtual StructuredBuffer* CreateStructuredBuffer(
        uint32 Stride,
        uint32 NumElements,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData ) override final;

    virtual class RayTracingScene* CreateRayTracingScene( uint32 Flags, RayTracingGeometryInstance* Instances, uint32 NumInstances ) override final;
    virtual class RayTracingGeometry* CreateRayTracingGeometry( uint32 Flags, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer ) override final;

    virtual ShaderResourceView* CreateShaderResourceView( const ShaderResourceViewCreateInfo& CreateInfo ) override final;
    virtual UnorderedAccessView* CreateUnorderedAccessView( const UnorderedAccessViewCreateInfo& CreateInfo ) override final;
    virtual RenderTargetView* CreateRenderTargetView( const RenderTargetViewCreateInfo& CreateInfo ) override final;
    virtual DepthStencilView* CreateDepthStencilView( const DepthStencilViewCreateInfo& CreateInfo ) override final;

    virtual class ComputeShader* CreateComputeShader( const TArray<uint8>& ShaderCode ) override final;

    virtual class VertexShader* CreateVertexShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class HullShader* CreateHullShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class DomainShader* CreateDomainShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class GeometryShader* CreateGeometryShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class MeshShader* CreateMeshShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class AmplificationShader* CreateAmplificationShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class PixelShader* CreatePixelShader( const TArray<uint8>& ShaderCode ) override final;

    virtual class RayGenShader* CreateRayGenShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class RayAnyHitShader* CreateRayAnyHitShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class RayClosestHitShader* CreateRayClosestHitShader( const TArray<uint8>& ShaderCode ) override final;
    virtual class RayMissShader* CreateRayMissShader( const TArray<uint8>& ShaderCode ) override final;

    virtual class DepthStencilState* CreateDepthStencilState( const DepthStencilStateCreateInfo& CreateInfo ) override final;
    virtual class RasterizerState* CreateRasterizerState( const RasterizerStateCreateInfo& CreateInfo ) override final;
    virtual class BlendState* CreateBlendState( const BlendStateCreateInfo& CreateInfo ) override final;
    virtual class InputLayoutState* CreateInputLayout( const InputLayoutStateCreateInfo& CreateInfo ) override final;

    virtual class GraphicsPipelineState* CreateGraphicsPipelineState( const GraphicsPipelineStateCreateInfo& CreateInfo ) override final;
    virtual class ComputePipelineState* CreateComputePipelineState( const ComputePipelineStateCreateInfo& CreateInfo ) override final;
    virtual class RayTracingPipelineState* CreateRayTracingPipelineState( const RayTracingPipelineStateCreateInfo& CreateInfo ) override final;

    virtual class GPUProfiler* CreateProfiler() override final;

    virtual class Viewport* CreateViewport( CCoreWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat ) override final;

    // TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
    virtual bool UAVSupportsFormat( EFormat Format ) override final;

    virtual class ICommandContext* GetDefaultCommandContext() override final
    {
        return DirectCmdContext.Get();
    }

    virtual std::string GetAdapterName() override final
    {
        return Device->GetAdapterName();
    }

    virtual void CheckRayTracingSupport( RayTracingSupport& OutSupport ) override final;
    virtual void CheckShadingRateSupport( ShadingRateSupport& OutSupport ) override final;

private:
    template<typename TD3D12Texture>
    TD3D12Texture* CreateTexture(
        EFormat Format,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 NumMips,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const ResourceData* InitialData,
        const ClearValue& OptimalClearValue );

    template<typename TD3D12Buffer>
    bool FinalizeBufferResource( TD3D12Buffer* Buffer, uint32 SizeInBytes, uint32 Flags, EResourceState InitialState, const ResourceData* InitialData );

private:
    D3D12Device* Device;
    TSharedRef<D3D12CommandContext> DirectCmdContext;
    D3D12RootSignatureCache* RootSignatureCache;

    D3D12OfflineDescriptorHeap* ResourceOfflineDescriptorHeap = nullptr;
    D3D12OfflineDescriptorHeap* RenderTargetOfflineDescriptorHeap = nullptr;
    D3D12OfflineDescriptorHeap* DepthStencilOfflineDescriptorHeap = nullptr;
    D3D12OfflineDescriptorHeap* SamplerOfflineDescriptorHeap = nullptr;
};

extern D3D12RenderLayer* GD3D12RenderLayer;
