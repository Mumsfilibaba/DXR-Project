#pragma once
#include "RenderLayer/GenericRenderLayer.h"

#include "Windows/WindowsWindow.h"

#include "D3D12Device.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"
#include "D3D12RootSignature.h"

class D3D12CommandContext;
class D3D12Buffer;

template<typename TD3D12Texture>
D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension();

template<typename TD3D12Texture>
Bool IsTextureCube();

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

    virtual Bool Init(Bool EnableDebug) override final;

    virtual Texture2D* CreateTexture2D(
        EFormat Format,
        UInt32 Width,
        UInt32 Height,
        UInt32 NumMipLevels,
        UInt32 NumSamples,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) override final;

    virtual Texture2DArray* CreateTexture2DArray(
        EFormat Format,
        UInt32 Width,
        UInt32 Height,
        UInt32 NumMipLevels,
        UInt32 NumSamples,
        UInt32 NumArraySlices,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) override final;

    virtual TextureCube* CreateTextureCube(
        EFormat Format,
        UInt32 Size,
        UInt32 NumMipLevels,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) override final;

    virtual TextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        UInt32 Size,
        UInt32 NumMipLevels,
        UInt32 NumArraySlices,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) override final;

    virtual Texture3D* CreateTexture3D(
        EFormat Format,
        UInt32 Width,
        UInt32 Height,
        UInt32 Depth,
        UInt32 NumMipLevels,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) override final;

    virtual class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo) override final;

    virtual VertexBuffer* CreateVertexBuffer(
        UInt32 Stride, 
        UInt32 NumVertices, 
        UInt32 Flags, 
        EResourceState InitialState, 
        const ResourceData* InitalData) override final;

    virtual IndexBuffer* CreateIndexBuffer(
        EIndexFormat Format, 
        UInt32 NumIndices, 
        UInt32 Flags, 
        EResourceState InitialState, 
        const ResourceData* InitalData) override final;

    virtual ConstantBuffer* CreateConstantBuffer(
        UInt32 Size,
        UInt32 Flags, 
        EResourceState InitialState, 
        const ResourceData* InitalData) override final;

    virtual StructuredBuffer* CreateStructuredBuffer(
        UInt32 Stride, 
        UInt32 NumElements, 
        UInt32 Flags, 
        EResourceState InitialState, 
        const ResourceData* InitalData) override final;

    virtual class RayTracingScene* CreateRayTracingScene(UInt32 Flags, TArrayView<RayTracingGeometryInstance> Instances) override final;
    virtual class RayTracingGeometry* CreateRayTracingGeometry(UInt32 Flags, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer) override final;

    virtual ShaderResourceView* CreateShaderResourceView(const ShaderResourceViewCreateInfo& CreateInfo) override final;
    virtual UnorderedAccessView* CreateUnorderedAccessView(const UnorderedAccessViewCreateInfo& CreateInfo) override final;
    virtual RenderTargetView* CreateRenderTargetView(const RenderTargetViewCreateInfo& CreateInfo) override final;
    virtual DepthStencilView* CreateDepthStencilView(const DepthStencilViewCreateInfo& CreateInfo) override final;

    virtual class ComputeShader* CreateComputeShader(const TArray<UInt8>& ShaderCode) override final;
    
    virtual class VertexShader* CreateVertexShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class HullShader* CreateHullShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class DomainShader* CreateDomainShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class GeometryShader* CreateGeometryShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class MeshShader* CreateMeshShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class AmplificationShader* CreateAmplificationShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class PixelShader* CreatePixelShader(const TArray<UInt8>& ShaderCode) override final;
    
    virtual class RayGenShader* CreateRayGenShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class RayAnyHitShader* CreateRayAnyHitShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class RayClosestHitShader* CreateRayClosestHitShader(const TArray<UInt8>& ShaderCode) override final;
    virtual class RayMissShader* CreateRayMissShader(const TArray<UInt8>& ShaderCode) override final;

    virtual class DepthStencilState* CreateDepthStencilState(const DepthStencilStateCreateInfo& CreateInfo) override final;
    virtual class RasterizerState* CreateRasterizerState(const RasterizerStateCreateInfo& CreateInfo) override final;
    virtual class BlendState* CreateBlendState(const BlendStateCreateInfo& CreateInfo) override final;
    virtual class InputLayoutState* CreateInputLayout(const InputLayoutStateCreateInfo& CreateInfo) override final;

    virtual class GraphicsPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo) override final;
    virtual class ComputePipelineState* CreateComputePipelineState(const ComputePipelineStateCreateInfo& CreateInfo) override final;
    virtual class RayTracingPipelineState* CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& CreateInfo) override final;

    virtual class Viewport* CreateViewport(GenericWindow* Window, UInt32 Width, UInt32 Height, EFormat ColorFormat, EFormat DepthFormat) override final;

    // TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
    virtual Bool UAVSupportsFormat(EFormat Format) override final;
    
    virtual class ICommandContext* GetDefaultCommandContext() override final
    {
        return DirectCmdContext.Get();
    }

    virtual std::string GetAdapterName() override final
    {
        return Device->GetAdapterName();
    }

    virtual void CheckRayTracingSupport(RayTracingSupport& OutSupport) override final;
    virtual void CheckShadingRateSupport(ShadingRateSupport& OutSupport) override final;

private:
    template<typename TD3D12Texture>
    TD3D12Texture* CreateTexture(
        EFormat Format, 
        UInt32 SizeX, UInt32 SizeY, UInt32 SizeZ,
        UInt32 NumMips, 
        UInt32 NumSamples,
        UInt32 Flags,
        EResourceState InitialState, 
        const ResourceData* InitialData, 
        const ClearValue& OptimalClearValue);

    template<typename TD3D12Buffer>
    Bool FinalizeBufferResource(TD3D12Buffer* Buffer, UInt32 SizeInBytes, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData);

private:
    D3D12Device*              Device;
    TRef<D3D12CommandContext> DirectCmdContext;
    D3D12RootSignatureCache*  RootSignatureCache;

    D3D12OfflineDescriptorHeap* ResourceOfflineDescriptorHeap     = nullptr;
    D3D12OfflineDescriptorHeap* RenderTargetOfflineDescriptorHeap = nullptr;
    D3D12OfflineDescriptorHeap* DepthStencilOfflineDescriptorHeap = nullptr;
    D3D12OfflineDescriptorHeap* SamplerOfflineDescriptorHeap      = nullptr;
};

extern D3D12RenderLayer* gD3D12RenderLayer;
