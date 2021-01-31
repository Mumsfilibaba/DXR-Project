#pragma once
#include "Application/Generic/GenericWindow.h"

#include "RenderingCore.h"
#include "Resources.h"
#include "ResourceViews.h"
#include "CommandList.h"

struct ResourceData;
struct ClearValue;
class RayTracingGeometry;
class RayTracingScene;

enum class ERenderLayerApi : UInt32
{
    RenderLayerApi_Unknown = 0,
    RenderLayerApi_D3D12   = 1,
};

class GenericRenderLayer
{
public:
    GenericRenderLayer(ERenderLayerApi InApi)
        : Api(InApi)
    {
    }

    virtual ~GenericRenderLayer() = default;

    virtual Bool Init(Bool EnableDebug) = 0;

    virtual Texture2D* CreateTexture2D(
        EFormat Format,
        UInt32 Width,
        UInt32 Height,
        UInt32 NumMipLevels,
        UInt32 NumSamples,
        UInt32 Usage,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) const = 0;

    virtual Texture2DArray* CreateTexture2DArray(
        EFormat Format,
        UInt32 Width,
        UInt32 Height,
        UInt32 NumMipLevels,
        UInt32 NumSamples,
        UInt32 NumArraySlices,
        UInt32 Usage,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) const = 0;

    virtual TextureCube* CreateTextureCube(
        EFormat Format,
        UInt32 Size,
        UInt32 NumMipLevels,
        UInt32 Usage,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) const = 0;

    virtual TextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        UInt32 Size,
        UInt32 NumMipLevels,
        UInt32 NumArraySlices,
        UInt32 Usage,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) const = 0;

    virtual Texture3D* CreateTexture3D(
        EFormat Format,
        UInt32 Width,
        UInt32 Height,
        UInt32 Depth,
        UInt32 NumMipLevels,
        UInt32 Usage,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) const = 0;

    virtual class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo) const = 0;

    virtual VertexBuffer* CreateVertexBuffer(UInt32 Stride, UInt32 NumVertices, UInt32 Usage, EResourceState InitialState, const ResourceData* InitalData) const = 0;
    virtual IndexBuffer* CreateIndexBuffer(EIndexFormat Format, UInt32 NumIndices, UInt32 Usage, EResourceState InitialState, const ResourceData* InitalData) const = 0;
    virtual ConstantBuffer* CreateConstantBuffer(UInt32 SizeInBytes, EResourceState InitialState, const ResourceData* InitalData) const = 0;
    virtual StructuredBuffer* CreateStructuredBuffer(UInt32 Stride, UInt32 NumElements, UInt32 Usage, EResourceState InitialState, const ResourceData* InitalData) const = 0;

    virtual RayTracingScene* CreateRayTracingScene() const = 0;
    virtual RayTracingGeometry* CreateRayTracingGeometry() const = 0;

    virtual ShaderResourceView* CreateShaderResourceView(const ShaderResourceViewCreateInfo& CreateInfo) const = 0;
    virtual UnorderedAccessView* CreateUnorderedAccessView(const UnorderedAccessViewCreateInfo& CreateInfo) const = 0;
    virtual RenderTargetView* CreateRenderTargetView(const RenderTargetViewCreateInfo& CreateInfo) const = 0;
    virtual DepthStencilView* CreateDepthStencilView(const DepthStencilViewCreateInfo& CreateInfo) const = 0;

    virtual class ComputeShader* CreateComputeShader(const TArray<UInt8>& ShaderCode) const = 0;

    virtual class VertexShader* CreateVertexShader(const TArray<UInt8>& ShaderCode) const = 0;
    virtual class HullShader* CreateHullShader(const TArray<UInt8>& ShaderCode) const = 0;
    virtual class DomainShader* CreateDomainShader(const TArray<UInt8>& ShaderCode) const = 0;
    virtual class GeometryShader* CreateGeometryShader(const TArray<UInt8>& ShaderCode) const = 0;
    virtual class MeshShader* CreateMeshShader(const TArray<UInt8>& ShaderCode) const = 0;
    virtual class AmplificationShader* CreateAmplificationShader(const TArray<UInt8>& ShaderCode) const = 0;
    virtual class PixelShader* CreatePixelShader(const TArray<UInt8>& ShaderCode) const = 0;
    
    virtual class RayGenShader* CreateRayGenShader(const TArray<UInt8>& ShaderCode) const = 0;
    virtual class RayHitShader* CreateRayHitShader(const TArray<UInt8>& ShaderCode) const = 0;
    virtual class RayMissShader* CreateRayMissShader(const TArray<UInt8>& ShaderCode) const = 0;

    virtual class DepthStencilState* CreateDepthStencilState(const DepthStencilStateCreateInfo& CreateInfo) const = 0;

    virtual class RasterizerState* CreateRasterizerState(const RasterizerStateCreateInfo& CreateInfo) const = 0;
    
    virtual class BlendState* CreateBlendState(const BlendStateCreateInfo& CreateInfo) const = 0;
    
    virtual class InputLayoutState* CreateInputLayout(const InputLayoutStateCreateInfo& CreateInfo) const = 0;

    virtual class GraphicsPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo) const = 0;
    virtual class ComputePipelineState* CreateComputePipelineState(const ComputePipelineStateCreateInfo& CreateInfo) const = 0;
    virtual class RayTracingPipelineState* CreateRayTracingPipelineState() const = 0;

    virtual class Viewport* CreateViewport(GenericWindow* Window, UInt32 Width, UInt32 Height, EFormat ColorFormat, EFormat DepthFormat) const = 0;

    virtual class ICommandContext* GetDefaultCommandContext() const = 0;

    virtual std::string GetAdapterName() const { return std::string(); }

    virtual Bool IsRayTracingSupported() const { return false; }

    virtual Bool UAVSupportsFormat(EFormat Format) const
    {
        UNREFERENCED_VARIABLE(Format);
        return false;
    }

    ERenderLayerApi GetApi() const { return Api; }

protected:
    ERenderLayerApi Api;
};