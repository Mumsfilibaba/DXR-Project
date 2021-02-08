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
    Unknown = 0,
    D3D12   = 1,
};

inline const Char* ToString(ERenderLayerApi RenderLayerApi)
{
    switch (RenderLayerApi)
    {
    case ERenderLayerApi::D3D12: return "D3D12";
    default: return "Unknown";
    }
}

enum EShadingRateTier
{
    NotSupported = 0,
    Tier1 = 1,
    Tier2 = 2,
};

struct ShadingRateSupport
{
    EShadingRateTier Tier = EShadingRateTier::NotSupported;
    UInt32           ShadingRateImageTileSize = 0;
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
        UInt32 NumMips,
        UInt32 NumSamples,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) = 0;

    virtual Texture2DArray* CreateTexture2DArray(
        EFormat Format,
        UInt32 Width,
        UInt32 Height,
        UInt32 NumMips,
        UInt32 NumSamples,
        UInt32 NumArraySlices,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) = 0;

    virtual TextureCube* CreateTextureCube(
        EFormat Format,
        UInt32 Size,
        UInt32 NumMips,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) = 0;

    virtual TextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        UInt32 Size,
        UInt32 NumMips,
        UInt32 NumArraySlices,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) = 0;

    virtual Texture3D* CreateTexture3D(
        EFormat Format,
        UInt32 Width,
        UInt32 Height,
        UInt32 Depth,
        UInt32 NumMips,
        UInt32 Flags,
        EResourceState InitialState,
        const ResourceData* InitalData,
        const ClearValue& OptimizedClearValue) = 0;

    virtual class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo) = 0;

    virtual VertexBuffer* CreateVertexBuffer(UInt32 Stride, UInt32 NumVertices, UInt32 Flags, EResourceState InitialState, const ResourceData* InitalData) = 0;
    virtual IndexBuffer* CreateIndexBuffer(EIndexFormat Format, UInt32 NumIndices, UInt32 Flags, EResourceState InitialState, const ResourceData* InitalData) = 0;
    virtual ConstantBuffer* CreateConstantBuffer(UInt32 SizeInBytes, UInt32 Flags, EResourceState InitialState, const ResourceData* InitalData) = 0;
    virtual StructuredBuffer* CreateStructuredBuffer(UInt32 Stride, UInt32 NumElements, UInt32 Flags, EResourceState InitialState, const ResourceData* InitalData) = 0;

    virtual RayTracingScene* CreateRayTracingScene() = 0;
    virtual RayTracingGeometry* CreateRayTracingGeometry() = 0;

    ShaderResourceView* CreateShaderResourceView(Texture2D* Texture, EFormat Format, UInt32 Mip, UInt32 NumMips, Float MinMipBias)
    {
        ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::Texture2D);
        CreateInfo.Texture2D.Texture    = Texture;
        CreateInfo.Texture2D.Format     = Format;
        CreateInfo.Texture2D.Mip        = Mip;
        CreateInfo.Texture2D.NumMips    = NumMips;
        CreateInfo.Texture2D.MinMipBias = MinMipBias;
        return CreateShaderResourceView(CreateInfo);
    }

    ShaderResourceView* CreateShaderResourceView(
        Texture2DArray* Texture, 
        EFormat Format, 
        UInt32 Mip, 
        UInt32 NumMips, 
        UInt32 ArraySlice, 
        UInt32 NumArraySlices, 
        Float MinMipBias)
    {
        ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::Texture2DArray);
        CreateInfo.Texture2DArray.Texture        = Texture;
        CreateInfo.Texture2DArray.Format         = Format;
        CreateInfo.Texture2DArray.Mip            = Mip;
        CreateInfo.Texture2DArray.NumMips        = NumMips;
        CreateInfo.Texture2DArray.ArraySlice     = ArraySlice;
        CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
        CreateInfo.Texture2DArray.MinMipBias     = MinMipBias;
        return CreateShaderResourceView(CreateInfo);
    }

    ShaderResourceView* CreateShaderResourceView(TextureCube* Texture, EFormat Format, UInt32 Mip, UInt32 NumMips, Float MinMipBias)
    {
        ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::TextureCube);
        CreateInfo.TextureCube.Texture      = Texture;
        CreateInfo.TextureCube.Format       = Format;
        CreateInfo.TextureCube.Mip          = Mip;
        CreateInfo.TextureCube.NumMips      = NumMips;
        CreateInfo.TextureCube.MinMipBias   = MinMipBias;
        return CreateShaderResourceView(CreateInfo);
    }

    ShaderResourceView* CreateShaderResourceView(
        TextureCubeArray* Texture,
        EFormat Format, 
        UInt32 Mip, 
        UInt32 NumMips, 
        UInt32 ArraySlice, 
        UInt32 NumArraySlices, 
        Float MinMipBias)
    {
        ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::TextureCubeArray);
        CreateInfo.TextureCubeArray.Texture        = Texture;
        CreateInfo.TextureCubeArray.Format         = Format;
        CreateInfo.TextureCubeArray.Mip            = Mip;
        CreateInfo.TextureCubeArray.NumMips        = NumMips;
        CreateInfo.TextureCubeArray.ArraySlice     = ArraySlice;
        CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
        CreateInfo.TextureCubeArray.MinMipBias     = MinMipBias;
        return CreateShaderResourceView(CreateInfo);
    }

    ShaderResourceView* CreateShaderResourceView(
        Texture3D* Texture, 
        EFormat Format, 
        UInt32 Mip, 
        UInt32 NumMips, 
        UInt32 DepthSlice, 
        UInt32 NumDepthSlices, 
        Float MinMipBias)
    {
        ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::Texture3D);
        CreateInfo.Texture3D.Texture        = Texture;
        CreateInfo.Texture3D.Format         = Format;
        CreateInfo.Texture3D.Mip            = Mip;
        CreateInfo.Texture3D.NumMips        = NumMips;
        CreateInfo.Texture3D.DepthSlice     = DepthSlice;
        CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
        CreateInfo.Texture3D.MinMipBias     = MinMipBias;
        return CreateShaderResourceView(CreateInfo);
    }

    ShaderResourceView* CreateShaderResourceView(VertexBuffer* Buffer, UInt32 FirstVertex, UInt32 NumVertices)
    {
        ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::VertexBuffer);
        CreateInfo.VertexBuffer.Buffer      = Buffer;
        CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
        CreateInfo.VertexBuffer.NumVertices = NumVertices;
        return CreateShaderResourceView(CreateInfo);
    }

    ShaderResourceView* CreateShaderResourceView(IndexBuffer* Buffer, UInt32 FirstIndex, UInt32 NumIndices)
    {
        ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::IndexBuffer);
        CreateInfo.IndexBuffer.Buffer     = Buffer;
        CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
        CreateInfo.IndexBuffer.NumIndices = NumIndices;
        return CreateShaderResourceView(CreateInfo);
    }

    ShaderResourceView* CreateShaderResourceView(StructuredBuffer* Buffer, UInt32 FirstElement, UInt32 NumElements)
    {
        ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::StructuredBuffer);
        CreateInfo.StructuredBuffer.Buffer       = Buffer;
        CreateInfo.StructuredBuffer.FirstElement = FirstElement;
        CreateInfo.StructuredBuffer.NumElements  = NumElements;
        return CreateShaderResourceView(CreateInfo);
    }

    virtual ShaderResourceView* CreateShaderResourceView(const ShaderResourceViewCreateInfo& CreateInfo) = 0;

    UnorderedAccessView* CreateUnorderedAccessView(Texture2D* Texture, EFormat Format, UInt32 Mip)
    {
        UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::Texture2D);
        CreateInfo.Texture2D.Texture = Texture;
        CreateInfo.Texture2D.Format  = Format;
        CreateInfo.Texture2D.Mip     = Mip;
        return CreateUnorderedAccessView(CreateInfo);
    }

    UnorderedAccessView* CreateUnorderedAccessView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
    {
        UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::Texture2DArray);
        CreateInfo.Texture2DArray.Texture        = Texture;
        CreateInfo.Texture2DArray.Format         = Format;
        CreateInfo.Texture2DArray.Mip            = Mip;
        CreateInfo.Texture2DArray.ArraySlice     = ArraySlice;
        CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
        return CreateUnorderedAccessView(CreateInfo);
    }

    UnorderedAccessView* CreateUnorderedAccessView(TextureCube* Texture, EFormat Format, UInt32 Mip)
    {
        UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::TextureCube);
        CreateInfo.TextureCube.Texture = Texture;
        CreateInfo.TextureCube.Format  = Format;
        CreateInfo.TextureCube.Mip     = Mip;
        return CreateUnorderedAccessView(CreateInfo);
    }

    UnorderedAccessView* CreateUnorderedAccessView(TextureCubeArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
    {
        UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::TextureCubeArray);
        CreateInfo.TextureCubeArray.Texture        = Texture;
        CreateInfo.TextureCubeArray.Format         = Format;
        CreateInfo.TextureCubeArray.Mip            = Mip;
        CreateInfo.TextureCubeArray.ArraySlice     = ArraySlice;
        CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
        return CreateUnorderedAccessView(CreateInfo);
    }

    UnorderedAccessView* CreateUnorderedAccessView(Texture3D* Texture, EFormat Format, UInt32 Mip, UInt32 DepthSlice, UInt32 NumDepthSlices)
    {
        UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::Texture3D);
        CreateInfo.Texture3D.Texture        = Texture;
        CreateInfo.Texture3D.Format         = Format;
        CreateInfo.Texture3D.Mip            = Mip;
        CreateInfo.Texture3D.DepthSlice     = DepthSlice;
        CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
        return CreateUnorderedAccessView(CreateInfo);
    }

    UnorderedAccessView* CreateUnorderedAccessView(VertexBuffer* Buffer, UInt32 FirstVertex, UInt32 NumVertices)
    {
        UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::VertexBuffer);
        CreateInfo.VertexBuffer.Buffer      = Buffer;
        CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
        CreateInfo.VertexBuffer.NumVertices = NumVertices;
        return CreateUnorderedAccessView(CreateInfo);
    }

    UnorderedAccessView* CreateUnorderedAccessView(IndexBuffer* Buffer, UInt32 FirstIndex, UInt32 NumIndices)
    {
        UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::IndexBuffer);
        CreateInfo.IndexBuffer.Buffer     = Buffer;
        CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
        CreateInfo.IndexBuffer.NumIndices = NumIndices;
        return CreateUnorderedAccessView(CreateInfo);
    }

    UnorderedAccessView* CreateUnorderedAccessView(StructuredBuffer* Buffer, UInt32 FirstElement, UInt32 NumElements)
    {
        UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::StructuredBuffer);
        CreateInfo.StructuredBuffer.Buffer       = Buffer;
        CreateInfo.StructuredBuffer.FirstElement = FirstElement;
        CreateInfo.StructuredBuffer.NumElements  = NumElements;
        return CreateUnorderedAccessView(CreateInfo);
    }

    virtual UnorderedAccessView* CreateUnorderedAccessView(const UnorderedAccessViewCreateInfo& CreateInfo) = 0;

    RenderTargetView* CreateRenderTargetView(Texture2D* Texture, EFormat Format, UInt32 Mip)
    {
        RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::Texture2D);
        CreateInfo.Format            = Format;
        CreateInfo.Texture2D.Texture = Texture;
        CreateInfo.Texture2D.Mip     = Mip;
        return CreateRenderTargetView(CreateInfo);
    }

    RenderTargetView* CreateRenderTargetView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
    {
        RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::Texture2DArray);
        CreateInfo.Format                        = Format;
        CreateInfo.Texture2DArray.Texture        = Texture;
        CreateInfo.Texture2DArray.Mip            = Mip;
        CreateInfo.Texture2DArray.ArraySlice     = ArraySlice;
        CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
        return CreateRenderTargetView(CreateInfo);
    }

    RenderTargetView* CreateRenderTargetView(TextureCube* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip)
    {
        RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::TextureCube);
        CreateInfo.Format               = Format;
        CreateInfo.TextureCube.Texture  = Texture;
        CreateInfo.TextureCube.Mip      = Mip;
        CreateInfo.TextureCube.CubeFace = CubeFace;
        return CreateRenderTargetView(CreateInfo);
    }

    RenderTargetView* CreateRenderTargetView(TextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip, UInt32 ArraySlice)
    {
        RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::TextureCubeArray);
        CreateInfo.Format                      = Format;
        CreateInfo.TextureCubeArray.Texture    = Texture;
        CreateInfo.TextureCubeArray.Mip        = Mip;
        CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
        CreateInfo.TextureCubeArray.CubeFace   = CubeFace;
        return CreateRenderTargetView(CreateInfo);
    }

    RenderTargetView* CreateRenderTargetView(Texture3D* Texture, EFormat Format, UInt32 Mip, UInt32 DepthSlice, UInt32 NumDepthSlices)
    {
        RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::Texture3D);
        CreateInfo.Format                   = Format;
        CreateInfo.Texture3D.Texture        = Texture;
        CreateInfo.Texture3D.Mip            = Mip;
        CreateInfo.Texture3D.DepthSlice     = DepthSlice;
        CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
        return CreateRenderTargetView(CreateInfo);
    }

    virtual RenderTargetView* CreateRenderTargetView(const RenderTargetViewCreateInfo& CreateInfo) = 0;

    DepthStencilView* CreateDepthStencilView(Texture2D* Texture, EFormat Format, UInt32 Mip)
    {
        DepthStencilViewCreateInfo CreateInfo(DepthStencilViewCreateInfo::EType::Texture2D);
        CreateInfo.Format            = Format;
        CreateInfo.Texture2D.Texture = Texture;
        CreateInfo.Texture2D.Mip     = Mip;
        return CreateDepthStencilView(CreateInfo);
    }

    DepthStencilView* CreateDepthStencilView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
    {
        DepthStencilViewCreateInfo CreateInfo(DepthStencilViewCreateInfo::EType::Texture2DArray);
        CreateInfo.Format                        = Format;
        CreateInfo.Texture2DArray.Texture        = Texture;
        CreateInfo.Texture2DArray.Mip            = Mip;
        CreateInfo.Texture2DArray.ArraySlice     = ArraySlice;
        CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
        return CreateDepthStencilView(CreateInfo);
    }

    DepthStencilView* CreateDepthStencilView(TextureCube* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip)
    {
        DepthStencilViewCreateInfo CreateInfo(DepthStencilViewCreateInfo::EType::TextureCube);
        CreateInfo.Format               = Format;
        CreateInfo.TextureCube.Texture  = Texture;
        CreateInfo.TextureCube.Mip      = Mip;
        CreateInfo.TextureCube.CubeFace = CubeFace;
        return CreateDepthStencilView(CreateInfo);
    }

    DepthStencilView* CreateDepthStencilView(TextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip, UInt32 ArraySlice)
    {
        DepthStencilViewCreateInfo CreateInfo(DepthStencilViewCreateInfo::EType::TextureCubeArray);
        CreateInfo.Format                      = Format;
        CreateInfo.TextureCubeArray.Texture    = Texture;
        CreateInfo.TextureCubeArray.Mip        = Mip;
        CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
        CreateInfo.TextureCubeArray.CubeFace   = CubeFace;
        return CreateDepthStencilView(CreateInfo);
    }

    virtual DepthStencilView* CreateDepthStencilView(const DepthStencilViewCreateInfo& CreateInfo) = 0;

    virtual class ComputeShader* CreateComputeShader(const TArray<UInt8>& ShaderCode) = 0;

    virtual class VertexShader* CreateVertexShader(const TArray<UInt8>& ShaderCode) = 0;
    virtual class HullShader* CreateHullShader(const TArray<UInt8>& ShaderCode) = 0;
    virtual class DomainShader* CreateDomainShader(const TArray<UInt8>& ShaderCode) = 0;
    virtual class GeometryShader* CreateGeometryShader(const TArray<UInt8>& ShaderCode) = 0;
    virtual class MeshShader* CreateMeshShader(const TArray<UInt8>& ShaderCode) = 0;
    virtual class AmplificationShader* CreateAmplificationShader(const TArray<UInt8>& ShaderCode) = 0;
    virtual class PixelShader* CreatePixelShader(const TArray<UInt8>& ShaderCode) = 0;
    
    virtual class RayGenShader* CreateRayGenShader(const TArray<UInt8>& ShaderCode) = 0;
    virtual class RayHitShader* CreateRayHitShader(const TArray<UInt8>& ShaderCode) = 0;
    virtual class RayMissShader* CreateRayMissShader(const TArray<UInt8>& ShaderCode) = 0;

    virtual class DepthStencilState* CreateDepthStencilState(const DepthStencilStateCreateInfo& CreateInfo) = 0;

    virtual class RasterizerState* CreateRasterizerState(const RasterizerStateCreateInfo& CreateInfo) = 0;
    
    virtual class BlendState* CreateBlendState(const BlendStateCreateInfo& CreateInfo) = 0;
    
    virtual class InputLayoutState* CreateInputLayout(const InputLayoutStateCreateInfo& CreateInfo) = 0;

    virtual class GraphicsPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo) = 0;
    virtual class ComputePipelineState* CreateComputePipelineState(const ComputePipelineStateCreateInfo& CreateInfo) = 0;
    virtual class RayTracingPipelineState* CreateRayTracingPipelineState() = 0;

    virtual class Viewport* CreateViewport(GenericWindow* Window, UInt32 Width, UInt32 Height, EFormat ColorFormat, EFormat DepthFormat) = 0;

    virtual class ICommandContext* GetDefaultCommandContext() = 0;

    virtual std::string GetAdapterName() { return std::string(); }

    virtual void CheckShadingRateSupport(ShadingRateSupport& OutSupport) = 0;

    virtual Bool IsRayTracingSupported() { return false; }

    virtual Bool UAVSupportsFormat(EFormat Format)
    {
        UNREFERENCED_VARIABLE(Format);
        return false;
    }

    ERenderLayerApi GetApi() { return Api; }

private:
    ERenderLayerApi Api;
};