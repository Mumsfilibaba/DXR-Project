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

enum class EShadingRateTier
{
    NotSupported = 0,
    Tier1        = 1,
    Tier2        = 2,
};

struct ShadingRateSupport
{
    EShadingRateTier Tier = EShadingRateTier::NotSupported;
    UInt32           ShadingRateImageTileSize = 0;
};

enum class ERayTracingTier
{
    NotSupported = 0,
    Tier1        = 1,
    Tier1_1      = 2,
};

struct RayTracingSupport
{
    ERayTracingTier Tier;
    UInt32          MaxRecursionDepth;
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
    virtual ConstantBuffer* CreateConstantBuffer(UInt32 Size, UInt32 Flags, EResourceState InitialState, const ResourceData* InitalData) = 0;
    virtual StructuredBuffer* CreateStructuredBuffer(UInt32 Stride, UInt32 NumElements, UInt32 Flags, EResourceState InitialState, const ResourceData* InitalData) = 0;

    virtual RayTracingScene* CreateRayTracingScene() = 0;
    virtual RayTracingGeometry* CreateRayTracingGeometry(UInt32 Flags, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer) = 0;

    virtual ShaderResourceView* CreateShaderResourceView(const ShaderResourceViewCreateInfo& CreateInfo) = 0;
    virtual UnorderedAccessView* CreateUnorderedAccessView(const UnorderedAccessViewCreateInfo& CreateInfo) = 0;
    virtual RenderTargetView* CreateRenderTargetView(const RenderTargetViewCreateInfo& CreateInfo) = 0;
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

    virtual void CheckRayTracingSupport(RayTracingSupport& OutSupport)   = 0;
    virtual void CheckShadingRateSupport(ShadingRateSupport& OutSupport) = 0;

    virtual Bool UAVSupportsFormat(EFormat Format)
    {
        UNREFERENCED_VARIABLE(Format);
        return false;
    }

    ERenderLayerApi GetApi() { return Api; }

private:
    ERenderLayerApi Api;
};

FORCEINLINE Texture2D* CreateTexture2D(
    EFormat Format,
    UInt32 Width,
    UInt32 Height,
    UInt32 NumMips,
    UInt32 NumSamples,
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData,
    const ClearValue& OptimizedClearValue = ClearValue())
{
    return gRenderLayer->CreateTexture2D(Format, Width, Height, NumMips, NumSamples, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE Texture2DArray* CreateTexture2DArray(
    EFormat Format,
    UInt32 Width,
    UInt32 Height,
    UInt32 NumMips,
    UInt32 NumSamples,
    UInt32 NumArraySlices,
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData,
    const ClearValue& OptimizedClearValue = ClearValue())
{
    return gRenderLayer->CreateTexture2DArray(Format, Width, Height, NumMips, NumSamples, NumArraySlices, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE TextureCube* CreateTextureCube(
    EFormat Format,
    UInt32 Size,
    UInt32 NumMips,
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData,
    const ClearValue& OptimizedClearValue = ClearValue())
{
    return gRenderLayer->CreateTextureCube(Format, Size, NumMips, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE TextureCubeArray* CreateTextureCubeArray(
    EFormat Format,
    UInt32 Size,
    UInt32 NumMips,
    UInt32 NumArraySlices,
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData,
    const ClearValue& OptimizedClearValue = ClearValue())
{
    return gRenderLayer->CreateTextureCubeArray(Format, Size, NumMips, NumArraySlices, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE Texture3D* CreateTexture3D(
    EFormat Format,
    UInt32 Width,
    UInt32 Height,
    UInt32 Depth,
    UInt32 NumMips,
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData,
    const ClearValue& OptimizedClearValue = ClearValue())
{
    return gRenderLayer->CreateTexture3D(Format, Width, Height, Depth, NumMips, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateSamplerState(CreateInfo);
}

FORCEINLINE VertexBuffer* CreateVertexBuffer(
    UInt32 Stride,
    UInt32 NumVertices,
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData)
{
    return gRenderLayer->CreateVertexBuffer(Stride, NumVertices, Flags, InitialState, InitialData);
}

template<typename T>
FORCEINLINE VertexBuffer* CreateVertexBuffer(UInt32 NumVertices, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
{
    constexpr UInt32 STRIDE = sizeof(T);
    return CreateVertexBuffer(STRIDE, NumVertices, Flags, InitialState, InitialData);
}

FORCEINLINE IndexBuffer* CreateIndexBuffer(
    EIndexFormat Format,
    UInt32 NumIndices,
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData)
{
    return gRenderLayer->CreateIndexBuffer(Format, NumIndices, Flags, InitialState, InitialData);
}

FORCEINLINE ConstantBuffer* CreateConstantBuffer(UInt32 Size, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
{
    return gRenderLayer->CreateConstantBuffer(Size, Flags, InitialState, InitialData);
}

template<typename T>
FORCEINLINE ConstantBuffer* CreateConstantBuffer(UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
{
    constexpr UInt32 SIZE_IN_BYTES = sizeof(T);
    return CreateConstantBuffer(SIZE_IN_BYTES, Flags, InitialState, InitialData);
}

FORCEINLINE StructuredBuffer* CreateStructuredBuffer(
    UInt32 Stride,
    UInt32 NumElements,
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData)
{
    return gRenderLayer->CreateStructuredBuffer(Stride, NumElements, Flags, InitialState, InitialData);
}

FORCEINLINE RayTracingScene* CreateRayTracingScene()
{
    return gRenderLayer->CreateRayTracingScene();
}

FORCEINLINE RayTracingGeometry* CreateRayTracingGeometry(UInt32 Flags, VertexBuffer* VertexBuffer, IndexBuffer* IndexBuffer)
{
    return gRenderLayer->CreateRayTracingGeometry(Flags, VertexBuffer, IndexBuffer);
}

FORCEINLINE ShaderResourceView* CreateShaderResourceView(const ShaderResourceViewCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateShaderResourceView(CreateInfo);
}

FORCEINLINE ShaderResourceView* CreateShaderResourceView(Texture2D* Texture, EFormat Format, UInt32 Mip, UInt32 NumMips, Float MinMipBias)
{
    ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::Texture2D);
    CreateInfo.Texture2D.Texture    = Texture;
    CreateInfo.Texture2D.Format     = Format;
    CreateInfo.Texture2D.Mip        = Mip;
    CreateInfo.Texture2D.NumMips    = NumMips;
    CreateInfo.Texture2D.MinMipBias = MinMipBias;
    return CreateShaderResourceView(CreateInfo);
}

FORCEINLINE ShaderResourceView* CreateShaderResourceView(
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

FORCEINLINE ShaderResourceView* CreateShaderResourceView(TextureCube* Texture, EFormat Format, UInt32 Mip, UInt32 NumMips, Float MinMipBias)
{
    ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::TextureCube);
    CreateInfo.TextureCube.Texture    = Texture;
    CreateInfo.TextureCube.Format     = Format;
    CreateInfo.TextureCube.Mip        = Mip;
    CreateInfo.TextureCube.NumMips    = NumMips;
    CreateInfo.TextureCube.MinMipBias = MinMipBias;
    return CreateShaderResourceView(CreateInfo);
}

FORCEINLINE ShaderResourceView* CreateShaderResourceView(
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

FORCEINLINE ShaderResourceView* CreateShaderResourceView(
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

FORCEINLINE ShaderResourceView* CreateShaderResourceView(VertexBuffer* Buffer, UInt32 FirstVertex, UInt32 NumVertices)
{
    ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::VertexBuffer);
    CreateInfo.VertexBuffer.Buffer      = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return CreateShaderResourceView(CreateInfo);
}

FORCEINLINE ShaderResourceView* CreateShaderResourceView(IndexBuffer* Buffer, UInt32 FirstIndex, UInt32 NumIndices)
{
    ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::IndexBuffer);
    CreateInfo.IndexBuffer.Buffer     = Buffer;
    CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
    CreateInfo.IndexBuffer.NumIndices = NumIndices;
    return CreateShaderResourceView(CreateInfo);
}

FORCEINLINE ShaderResourceView* CreateShaderResourceView(StructuredBuffer* Buffer, UInt32 FirstElement, UInt32 NumElements)
{
    ShaderResourceViewCreateInfo CreateInfo(ShaderResourceViewCreateInfo::EType::StructuredBuffer);
    CreateInfo.StructuredBuffer.Buffer       = Buffer;
    CreateInfo.StructuredBuffer.FirstElement = FirstElement;
    CreateInfo.StructuredBuffer.NumElements  = NumElements;
    return CreateShaderResourceView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(const UnorderedAccessViewCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(Texture2D* Texture, EFormat Format, UInt32 Mip)
{
    UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::Texture2D);
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format  = Format;
    CreateInfo.Texture2D.Mip     = Mip;
    return CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
{
    UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::Texture2DArray);
    CreateInfo.Texture2DArray.Texture        = Texture;
    CreateInfo.Texture2DArray.Format         = Format;
    CreateInfo.Texture2DArray.Mip            = Mip;
    CreateInfo.Texture2DArray.ArraySlice     = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(TextureCube* Texture, EFormat Format, UInt32 Mip)
{
    UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::TextureCube);
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format  = Format;
    CreateInfo.TextureCube.Mip     = Mip;
    return CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(TextureCubeArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
{
    UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::TextureCubeArray);
    CreateInfo.TextureCubeArray.Texture        = Texture;
    CreateInfo.TextureCubeArray.Format         = Format;
    CreateInfo.TextureCubeArray.Mip            = Mip;
    CreateInfo.TextureCubeArray.ArraySlice     = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    return CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(Texture3D* Texture, EFormat Format, UInt32 Mip, UInt32 DepthSlice, UInt32 NumDepthSlices)
{
    UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::Texture3D);
    CreateInfo.Texture3D.Texture        = Texture;
    CreateInfo.Texture3D.Format         = Format;
    CreateInfo.Texture3D.Mip            = Mip;
    CreateInfo.Texture3D.DepthSlice     = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(VertexBuffer* Buffer, UInt32 FirstVertex, UInt32 NumVertices)
{
    UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::VertexBuffer);
    CreateInfo.VertexBuffer.Buffer      = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(IndexBuffer* Buffer, UInt32 FirstIndex, UInt32 NumIndices)
{
    UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::IndexBuffer);
    CreateInfo.IndexBuffer.Buffer     = Buffer;
    CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
    CreateInfo.IndexBuffer.NumIndices = NumIndices;
    return CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE UnorderedAccessView* CreateUnorderedAccessView(StructuredBuffer* Buffer, UInt32 FirstElement, UInt32 NumElements)
{
    UnorderedAccessViewCreateInfo CreateInfo(UnorderedAccessViewCreateInfo::EType::StructuredBuffer);
    CreateInfo.StructuredBuffer.Buffer       = Buffer;
    CreateInfo.StructuredBuffer.FirstElement = FirstElement;
    CreateInfo.StructuredBuffer.NumElements  = NumElements;
    return CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE RenderTargetView* CreateRenderTargetView(const RenderTargetViewCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateRenderTargetView(CreateInfo);
}

FORCEINLINE RenderTargetView* CreateRenderTargetView(Texture2D* Texture, EFormat Format, UInt32 Mip)
{
    RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::Texture2D);
    CreateInfo.Format            = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip     = Mip;
    return CreateRenderTargetView(CreateInfo);
}

FORCEINLINE RenderTargetView* CreateRenderTargetView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
{
    RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::Texture2DArray);
    CreateInfo.Format                        = Format;
    CreateInfo.Texture2DArray.Texture        = Texture;
    CreateInfo.Texture2DArray.Mip            = Mip;
    CreateInfo.Texture2DArray.ArraySlice     = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return CreateRenderTargetView(CreateInfo);
}

FORCEINLINE RenderTargetView* CreateRenderTargetView(TextureCube* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip)
{
    RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::TextureCube);
    CreateInfo.Format               = Format;
    CreateInfo.TextureCube.Texture  = Texture;
    CreateInfo.TextureCube.Mip      = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return CreateRenderTargetView(CreateInfo);
}

FORCEINLINE RenderTargetView* CreateRenderTargetView(TextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip, UInt32 ArraySlice)
{
    RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::TextureCubeArray);
    CreateInfo.Format                      = Format;
    CreateInfo.TextureCubeArray.Texture    = Texture;
    CreateInfo.TextureCubeArray.Mip        = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace   = CubeFace;
    return CreateRenderTargetView(CreateInfo);
}

FORCEINLINE RenderTargetView* CreateRenderTargetView(Texture3D* Texture, EFormat Format, UInt32 Mip, UInt32 DepthSlice, UInt32 NumDepthSlices)
{
    RenderTargetViewCreateInfo CreateInfo(RenderTargetViewCreateInfo::EType::Texture3D);
    CreateInfo.Format                   = Format;
    CreateInfo.Texture3D.Texture        = Texture;
    CreateInfo.Texture3D.Mip            = Mip;
    CreateInfo.Texture3D.DepthSlice     = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return CreateRenderTargetView(CreateInfo);
}

FORCEINLINE DepthStencilView* CreateDepthStencilView(const DepthStencilViewCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateDepthStencilView(CreateInfo);
}

FORCEINLINE DepthStencilView* CreateDepthStencilView(Texture2D* Texture, EFormat Format, UInt32 Mip)
{
    DepthStencilViewCreateInfo CreateInfo(DepthStencilViewCreateInfo::EType::Texture2D);
    CreateInfo.Format            = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip     = Mip;
    return CreateDepthStencilView(CreateInfo);
}

FORCEINLINE DepthStencilView* CreateDepthStencilView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
{
    DepthStencilViewCreateInfo CreateInfo(DepthStencilViewCreateInfo::EType::Texture2DArray);
    CreateInfo.Format                        = Format;
    CreateInfo.Texture2DArray.Texture        = Texture;
    CreateInfo.Texture2DArray.Mip            = Mip;
    CreateInfo.Texture2DArray.ArraySlice     = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return CreateDepthStencilView(CreateInfo);
}

FORCEINLINE DepthStencilView* CreateDepthStencilView(TextureCube* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip)
{
    DepthStencilViewCreateInfo CreateInfo(DepthStencilViewCreateInfo::EType::TextureCube);
    CreateInfo.Format               = Format;
    CreateInfo.TextureCube.Texture  = Texture;
    CreateInfo.TextureCube.Mip      = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return CreateDepthStencilView(CreateInfo);
}

FORCEINLINE DepthStencilView* CreateDepthStencilView(TextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip, UInt32 ArraySlice)
{
    DepthStencilViewCreateInfo CreateInfo(DepthStencilViewCreateInfo::EType::TextureCubeArray);
    CreateInfo.Format                      = Format;
    CreateInfo.TextureCubeArray.Texture    = Texture;
    CreateInfo.TextureCubeArray.Mip        = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace   = CubeFace;
    return CreateDepthStencilView(CreateInfo);
}

FORCEINLINE ComputeShader* CreateComputeShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateComputeShader(ShaderCode);
}

FORCEINLINE VertexShader* CreateVertexShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateVertexShader(ShaderCode);
}

FORCEINLINE HullShader* CreateHullShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateHullShader(ShaderCode);
}

FORCEINLINE DomainShader* CreateDomainShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateDomainShader(ShaderCode);
}

FORCEINLINE GeometryShader* CreateGeometryShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateGeometryShader(ShaderCode);
}

FORCEINLINE MeshShader* CreateMeshShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateMeshShader(ShaderCode);
}

FORCEINLINE AmplificationShader* CreateAmplificationShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateAmplificationShader(ShaderCode);
}

FORCEINLINE PixelShader* CreatePixelShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreatePixelShader(ShaderCode);
}

FORCEINLINE RayGenShader* CreateRayGenShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateRayGenShader(ShaderCode);
}

FORCEINLINE RayHitShader* CreateRayHitShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateRayHitShader(ShaderCode);
}

FORCEINLINE RayMissShader* CreateRayMissShader(const TArray<UInt8>& ShaderCode)
{
    return gRenderLayer->CreateRayMissShader(ShaderCode);
}

FORCEINLINE InputLayoutState* CreateInputLayout(const InputLayoutStateCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateInputLayout(CreateInfo);
}

FORCEINLINE DepthStencilState* CreateDepthStencilState(const DepthStencilStateCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateDepthStencilState(CreateInfo);
}

FORCEINLINE RasterizerState* CreateRasterizerState(const RasterizerStateCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateRasterizerState(CreateInfo);
}

FORCEINLINE BlendState* CreateBlendState(const BlendStateCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateBlendState(CreateInfo);
}

FORCEINLINE ComputePipelineState* CreateComputePipelineState(const ComputePipelineStateCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateComputePipelineState(CreateInfo);
}

FORCEINLINE GraphicsPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo)
{
    return gRenderLayer->CreateGraphicsPipelineState(CreateInfo);
}

FORCEINLINE class Viewport* CreateViewport(GenericWindow* Window, UInt32 Width, UInt32 Height, EFormat ColorFormat, EFormat DepthFormat)
{
    return gRenderLayer->CreateViewport(Window, Width, Height, ColorFormat, DepthFormat);
}

FORCEINLINE Bool UAVSupportsFormat(EFormat Format)
{
    return gRenderLayer->UAVSupportsFormat(Format);
}

FORCEINLINE class ICommandContext* GetDefaultCommandContext()
{
    return gRenderLayer->GetDefaultCommandContext();
}

FORCEINLINE std::string GetAdapterName()
{
    return gRenderLayer->GetAdapterName();
}

FORCEINLINE void CheckShadingRateSupport(ShadingRateSupport& OutSupport)
{
    gRenderLayer->CheckShadingRateSupport(OutSupport);
}

FORCEINLINE void CheckRayTracingSupport(RayTracingSupport& OutSupport)
{
    gRenderLayer->CheckRayTracingSupport(OutSupport);
}

FORCEINLINE Bool IsRayTracingSupported()
{
    RayTracingSupport Support;
    CheckRayTracingSupport(Support);

    return Support.Tier != ERayTracingTier::NotSupported;
}