#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHICommandList.h"

#include "Core/Engine/EngineGlobals.h"
#include "Core/Application/Core/CoreWindow.h"

struct SResourceData;
struct SClearValue;
class CRHIRayTracingGeometry;
class CRHIRayTracingScene;

enum class ERenderLayerApi : uint32
{
    Unknown = 0,
    D3D12 = 1,
};

inline const char* ToString( ERenderLayerApi RenderLayerApi )
{
    switch ( RenderLayerApi )
    {
        case ERenderLayerApi::D3D12: return "D3D12";
        default: return "Unknown";
    }
}

enum class EShadingRateTier
{
    NotSupported = 0,
    Tier1 = 1,
    Tier2 = 2,
};

struct SShadingRateSupport
{
    EShadingRateTier Tier = EShadingRateTier::NotSupported;
    uint32           ShadingRateImageTileSize = 0;
};

enum class ERayTracingTier
{
    NotSupported = 0,
    Tier1 = 1,
    Tier1_1 = 2,
};

struct SRayTracingSupport
{
    ERayTracingTier Tier;
    uint32          MaxRecursionDepth;
};

class CRHICore
{
public:
    FORCEINLINE CRHICore( ERenderLayerApi InApi )
        : Api( InApi )
    {
    }

    virtual ~CRHICore() = default;

    virtual bool Init( bool EnableDebug ) = 0;

    virtual CRHITexture2D* CreateTexture2D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMips,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) = 0;

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
        const SClearValue& OptimizedClearValue ) = 0;

    virtual CRHITextureCube* CreateTextureCube(
        EFormat Format,
        uint32 Size,
        uint32 NumMips,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) = 0;

    virtual CRHITextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        uint32 Size,
        uint32 NumMips,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) = 0;

    virtual CRHITexture3D* CreateTexture3D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 Depth,
        uint32 NumMips,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue ) = 0;

    virtual class CRHISamplerState* CreateSamplerState( const struct SSamplerStateCreateInfo& CreateInfo ) = 0;

    virtual CRHIVertexBuffer* CreateVertexBuffer( uint32 Stride, uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) = 0;
    virtual CRHIIndexBuffer* CreateIndexBuffer( EIndexFormat Format, uint32 NumIndices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) = 0;
    virtual CRHIConstantBuffer* CreateConstantBuffer( uint32 Size, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) = 0;
    virtual CRHIStructuredBuffer* CreateStructuredBuffer( uint32 Stride, uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData ) = 0;

    virtual CRHIRayTracingScene* CreateRayTracingScene( uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances ) = 0;
    virtual CRHIRayTracingGeometry* CreateRayTracingGeometry( uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer ) = 0;

    virtual CRHIShaderResourceView* CreateShaderResourceView( const SShaderResourceViewCreateInfo& CreateInfo ) = 0;
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView( const SUnorderedAccessViewCreateInfo& CreateInfo ) = 0;
    virtual CRHIRenderTargetView* CreateRenderTargetView( const SRenderTargetViewCreateInfo& CreateInfo ) = 0;
    virtual CRHIDepthStencilView* CreateDepthStencilView( const SDepthStencilViewCreateInfo& CreateInfo ) = 0;

    virtual class CRHIComputeShader* CreateComputeShader( const TArray<uint8>& ShaderCode ) = 0;

    virtual class CRHIVertexShader* CreateVertexShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIHullShader* CreateHullShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIDomainShader* CreateDomainShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIGeometryShader* CreateGeometryShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIMeshShader* CreateMeshShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIAmplificationShader* CreateAmplificationShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIPixelShader* CreatePixelShader( const TArray<uint8>& ShaderCode ) = 0;

    virtual class CRHIRayGenShader* CreateRayGenShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIRayAnyHitShader* CreateRayAnyHitShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader( const TArray<uint8>& ShaderCode ) = 0;
    virtual class CRHIRayMissShader* CreateRayMissShader( const TArray<uint8>& ShaderCode ) = 0;

    virtual class CRHIDepthStencilState* CreateDepthStencilState( const SDepthStencilStateCreateInfo& CreateInfo ) = 0;

    virtual class CRHIRasterizerState* CreateRasterizerState( const SRasterizerStateCreateInfo& CreateInfo ) = 0;

    virtual class CRHIBlendState* CreateBlendState( const SBlendStateCreateInfo& CreateInfo ) = 0;

    virtual class CRHIInputLayoutState* CreateInputLayout( const SInputLayoutStateCreateInfo& CreateInfo ) = 0;

    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState( const SGraphicsPipelineStateCreateInfo& CreateInfo ) = 0;
    virtual class CRHIComputePipelineState* CreateComputePipelineState( const SComputePipelineStateCreateInfo& CreateInfo ) = 0;
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState( const SRayTracingPipelineStateCreateInfo& CreateInfo ) = 0;

    virtual class CGPUProfiler* CreateProfiler() = 0;

    virtual class CRHIViewport* CreateViewport( CCoreWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat ) = 0;

    virtual class IRHICommandContext* GetDefaultCommandContext() = 0;

    virtual CString GetAdapterName()
    {
        return CString();
    }

    virtual void CheckRayTracingSupport( SRayTracingSupport& OutSupport ) = 0;
    virtual void CheckShadingRateSupport( SShadingRateSupport& OutSupport ) = 0;

    virtual bool UAVSupportsFormat( EFormat Format )
    {
        UNREFERENCED_VARIABLE( Format );
        return false;
    }

    FORCEINLINE ERenderLayerApi GetApi() const
    {
        return Api;
    }

private:
    ERenderLayerApi Api;
};

FORCEINLINE CRHITexture2D* CreateTexture2D(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 NumMips,
    uint32 NumSamples,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue() )
{
    return GRHICore->CreateTexture2D( Format, Width, Height, NumMips, NumSamples, Flags, InitialState, InitialData, OptimizedClearValue );
}

FORCEINLINE CRHITexture2DArray* CreateTexture2DArray(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 NumMips,
    uint32 NumSamples,
    uint32 NumArraySlices,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue() )
{
    return GRHICore->CreateTexture2DArray( Format, Width, Height, NumMips, NumSamples, NumArraySlices, Flags, InitialState, InitialData, OptimizedClearValue );
}

FORCEINLINE CRHITextureCube* CreateTextureCube(
    EFormat Format,
    uint32 Size,
    uint32 NumMips,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue() )
{
    return GRHICore->CreateTextureCube( Format, Size, NumMips, Flags, InitialState, InitialData, OptimizedClearValue );
}

FORCEINLINE CRHITextureCubeArray* CreateTextureCubeArray(
    EFormat Format,
    uint32 Size,
    uint32 NumMips,
    uint32 NumArraySlices,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue() )
{
    return GRHICore->CreateTextureCubeArray( Format, Size, NumMips, NumArraySlices, Flags, InitialState, InitialData, OptimizedClearValue );
}

FORCEINLINE CRHITexture3D* CreateTexture3D(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 Depth,
    uint32 NumMips,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue() )
{
    return GRHICore->CreateTexture3D( Format, Width, Height, Depth, NumMips, Flags, InitialState, InitialData, OptimizedClearValue );
}

FORCEINLINE class CRHISamplerState* CreateSamplerState( const struct SSamplerStateCreateInfo& CreateInfo )
{
    return GRHICore->CreateSamplerState( CreateInfo );
}

FORCEINLINE CRHIVertexBuffer* CreateVertexBuffer(
    uint32 Stride,
    uint32 NumVertices,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData )
{
    return GRHICore->CreateVertexBuffer( Stride, NumVertices, Flags, InitialState, InitialData );
}

template<typename T>
FORCEINLINE CRHIVertexBuffer* CreateVertexBuffer( uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    constexpr uint32 STRIDE = sizeof( T );
    return CreateVertexBuffer( STRIDE, NumVertices, Flags, InitialState, InitialData );
}

FORCEINLINE CRHIIndexBuffer* CreateIndexBuffer( EIndexFormat Format, uint32 NumIndices, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    return GRHICore->CreateIndexBuffer( Format, NumIndices, Flags, InitialState, InitialData );
}

FORCEINLINE CRHIConstantBuffer* CreateConstantBuffer( uint32 Size, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    return GRHICore->CreateConstantBuffer( Size, Flags, InitialState, InitialData );
}

template<typename TSize>
FORCEINLINE CRHIConstantBuffer* CreateConstantBuffer( uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    constexpr uint32 SIZE_IN_BYTES = sizeof( TSize );
    return CreateConstantBuffer( SIZE_IN_BYTES, Flags, InitialState, InitialData );
}

FORCEINLINE CRHIStructuredBuffer* CreateStructuredBuffer( uint32 Stride, uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    return GRHICore->CreateStructuredBuffer( Stride, NumElements, Flags, InitialState, InitialData );
}

template<typename TStride>
FORCEINLINE CRHIStructuredBuffer* CreateStructuredBuffer( uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData )
{
    constexpr uint32 STRIDE_IN_BYTES = sizeof( TStride );
    return CreateStructuredBuffer( STRIDE_IN_BYTES, NumElements, Flags, InitialState, InitialData );
}

FORCEINLINE CRHIRayTracingScene* CreateRayTracingScene( uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances )
{
    return GRHICore->CreateRayTracingScene( Flags, Instances, NumInstances );
}

FORCEINLINE CRHIRayTracingGeometry* CreateRayTracingGeometry( uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer )
{
    return GRHICore->CreateRayTracingGeometry( Flags, VertexBuffer, IndexBuffer );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView( const SShaderResourceViewCreateInfo& CreateInfo )
{
    return GRHICore->CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView( CRHITexture2D* Texture, EFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias )
{
    SShaderResourceViewCreateInfo CreateInfo( SShaderResourceViewCreateInfo::EType::Texture2D );
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format = Format;
    CreateInfo.Texture2D.Mip = Mip;
    CreateInfo.Texture2D.NumMips = NumMips;
    CreateInfo.Texture2D.MinMipBias = MinMipBias;
    return CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView(
    CRHITexture2DArray* Texture,
    EFormat Format,
    uint32 Mip,
    uint32 NumMips,
    uint32 ArraySlice,
    uint32 NumArraySlices,
    float MinMipBias )
{
    SShaderResourceViewCreateInfo CreateInfo( SShaderResourceViewCreateInfo::EType::Texture2DArray );
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Format = Format;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.NumMips = NumMips;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    CreateInfo.Texture2DArray.MinMipBias = MinMipBias;
    return CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView( CRHITextureCube* Texture, EFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias )
{
    SShaderResourceViewCreateInfo CreateInfo( SShaderResourceViewCreateInfo::EType::TextureCube );
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format = Format;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.NumMips = NumMips;
    CreateInfo.TextureCube.MinMipBias = MinMipBias;
    return CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView(
    CRHITextureCubeArray* Texture,
    EFormat Format,
    uint32 Mip,
    uint32 NumMips,
    uint32 ArraySlice,
    uint32 NumArraySlices,
    float MinMipBias )
{
    SShaderResourceViewCreateInfo CreateInfo( SShaderResourceViewCreateInfo::EType::TextureCubeArray );
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Format = Format;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.NumMips = NumMips;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    CreateInfo.TextureCubeArray.MinMipBias = MinMipBias;
    return CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView(
    CRHITexture3D* Texture,
    EFormat Format,
    uint32 Mip,
    uint32 NumMips,
    uint32 DepthSlice,
    uint32 NumDepthSlices,
    float MinMipBias )
{
    SShaderResourceViewCreateInfo CreateInfo( SShaderResourceViewCreateInfo::EType::Texture3D );
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Format = Format;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.NumMips = NumMips;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    CreateInfo.Texture3D.MinMipBias = MinMipBias;
    return CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView( CRHIVertexBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices )
{
    SShaderResourceViewCreateInfo CreateInfo( SShaderResourceViewCreateInfo::EType::VertexBuffer );
    CreateInfo.VertexBuffer.Buffer = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView( CRHIIndexBuffer* Buffer, uint32 FirstIndex, uint32 NumIndices )
{
    SShaderResourceViewCreateInfo CreateInfo( SShaderResourceViewCreateInfo::EType::IndexBuffer );
    CreateInfo.IndexBuffer.Buffer = Buffer;
    CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
    CreateInfo.IndexBuffer.NumIndices = NumIndices;
    return CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIShaderResourceView* CreateShaderResourceView( CRHIStructuredBuffer* Buffer, uint32 FirstElement, uint32 NumElements )
{
    SShaderResourceViewCreateInfo CreateInfo( SShaderResourceViewCreateInfo::EType::StructuredBuffer );
    CreateInfo.StructuredBuffer.Buffer = Buffer;
    CreateInfo.StructuredBuffer.FirstElement = FirstElement;
    CreateInfo.StructuredBuffer.NumElements = NumElements;
    return CreateShaderResourceView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( const SUnorderedAccessViewCreateInfo& CreateInfo )
{
    return GRHICore->CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( CRHITexture2D* Texture, EFormat Format, uint32 Mip )
{
    SUnorderedAccessViewCreateInfo CreateInfo( SUnorderedAccessViewCreateInfo::EType::Texture2D );
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format = Format;
    CreateInfo.Texture2D.Mip = Mip;
    return CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices )
{
    SUnorderedAccessViewCreateInfo CreateInfo( SUnorderedAccessViewCreateInfo::EType::Texture2DArray );
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Format = Format;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( CRHITextureCube* Texture, EFormat Format, uint32 Mip )
{
    SUnorderedAccessViewCreateInfo CreateInfo( SUnorderedAccessViewCreateInfo::EType::TextureCube );
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format = Format;
    CreateInfo.TextureCube.Mip = Mip;
    return CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( CRHITextureCubeArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices )
{
    SUnorderedAccessViewCreateInfo CreateInfo( SUnorderedAccessViewCreateInfo::EType::TextureCubeArray );
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Format = Format;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    return CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices )
{
    SUnorderedAccessViewCreateInfo CreateInfo( SUnorderedAccessViewCreateInfo::EType::Texture3D );
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Format = Format;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( CRHIVertexBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices )
{
    SUnorderedAccessViewCreateInfo CreateInfo( SUnorderedAccessViewCreateInfo::EType::VertexBuffer );
    CreateInfo.VertexBuffer.Buffer = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( CRHIIndexBuffer* Buffer, uint32 FirstIndex, uint32 NumIndices )
{
    SUnorderedAccessViewCreateInfo CreateInfo( SUnorderedAccessViewCreateInfo::EType::IndexBuffer );
    CreateInfo.IndexBuffer.Buffer = Buffer;
    CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
    CreateInfo.IndexBuffer.NumIndices = NumIndices;
    return CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIUnorderedAccessView* CreateUnorderedAccessView( CRHIStructuredBuffer* Buffer, uint32 FirstElement, uint32 NumElements )
{
    SUnorderedAccessViewCreateInfo CreateInfo( SUnorderedAccessViewCreateInfo::EType::StructuredBuffer );
    CreateInfo.StructuredBuffer.Buffer = Buffer;
    CreateInfo.StructuredBuffer.FirstElement = FirstElement;
    CreateInfo.StructuredBuffer.NumElements = NumElements;
    return CreateUnorderedAccessView( CreateInfo );
}

FORCEINLINE CRHIRenderTargetView* CreateRenderTargetView( const SRenderTargetViewCreateInfo& CreateInfo )
{
    return GRHICore->CreateRenderTargetView( CreateInfo );
}

FORCEINLINE CRHIRenderTargetView* CreateRenderTargetView( CRHITexture2D* Texture, EFormat Format, uint32 Mip )
{
    SRenderTargetViewCreateInfo CreateInfo( SRenderTargetViewCreateInfo::EType::Texture2D );
    CreateInfo.Format = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip = Mip;
    return CreateRenderTargetView( CreateInfo );
}

FORCEINLINE CRHIRenderTargetView* CreateRenderTargetView( CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices )
{
    SRenderTargetViewCreateInfo CreateInfo( SRenderTargetViewCreateInfo::EType::Texture2DArray );
    CreateInfo.Format = Format;
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return CreateRenderTargetView( CreateInfo );
}

FORCEINLINE CRHIRenderTargetView* CreateRenderTargetView( CRHITextureCube* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip )
{
    SRenderTargetViewCreateInfo CreateInfo( SRenderTargetViewCreateInfo::EType::TextureCube );
    CreateInfo.Format = Format;
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return CreateRenderTargetView( CreateInfo );
}

FORCEINLINE CRHIRenderTargetView* CreateRenderTargetView( CRHITextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip, uint32 ArraySlice )
{
    SRenderTargetViewCreateInfo CreateInfo( SRenderTargetViewCreateInfo::EType::TextureCubeArray );
    CreateInfo.Format = Format;
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace = CubeFace;
    return CreateRenderTargetView( CreateInfo );
}

FORCEINLINE CRHIRenderTargetView* CreateRenderTargetView( CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices )
{
    SRenderTargetViewCreateInfo CreateInfo( SRenderTargetViewCreateInfo::EType::Texture3D );
    CreateInfo.Format = Format;
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return CreateRenderTargetView( CreateInfo );
}

FORCEINLINE CRHIDepthStencilView* CreateDepthStencilView( const SDepthStencilViewCreateInfo& CreateInfo )
{
    return GRHICore->CreateDepthStencilView( CreateInfo );
}

FORCEINLINE CRHIDepthStencilView* CreateDepthStencilView( CRHITexture2D* Texture, EFormat Format, uint32 Mip )
{
    SDepthStencilViewCreateInfo CreateInfo( SDepthStencilViewCreateInfo::EType::Texture2D );
    CreateInfo.Format = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip = Mip;
    return CreateDepthStencilView( CreateInfo );
}

FORCEINLINE CRHIDepthStencilView* CreateDepthStencilView( CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices )
{
    SDepthStencilViewCreateInfo CreateInfo( SDepthStencilViewCreateInfo::EType::Texture2DArray );
    CreateInfo.Format = Format;
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return CreateDepthStencilView( CreateInfo );
}

FORCEINLINE CRHIDepthStencilView* CreateDepthStencilView( CRHITextureCube* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip )
{
    SDepthStencilViewCreateInfo CreateInfo( SDepthStencilViewCreateInfo::EType::TextureCube );
    CreateInfo.Format = Format;
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return CreateDepthStencilView( CreateInfo );
}

FORCEINLINE CRHIDepthStencilView* CreateDepthStencilView( CRHITextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip, uint32 ArraySlice )
{
    SDepthStencilViewCreateInfo CreateInfo( SDepthStencilViewCreateInfo::EType::TextureCubeArray );
    CreateInfo.Format = Format;
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace = CubeFace;
    return CreateDepthStencilView( CreateInfo );
}

FORCEINLINE CRHIComputeShader* CreateComputeShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateComputeShader( ShaderCode );
}

FORCEINLINE CRHIVertexShader* CreateVertexShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateVertexShader( ShaderCode );
}

FORCEINLINE CRHIHullShader* CreateHullShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateHullShader( ShaderCode );
}

FORCEINLINE CRHIDomainShader* CreateDomainShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateDomainShader( ShaderCode );
}

FORCEINLINE CRHIGeometryShader* CreateGeometryShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateGeometryShader( ShaderCode );
}

FORCEINLINE CRHIMeshShader* CreateMeshShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateMeshShader( ShaderCode );
}

FORCEINLINE CRHIAmplificationShader* CreateAmplificationShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateAmplificationShader( ShaderCode );
}

FORCEINLINE CRHIPixelShader* CreatePixelShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreatePixelShader( ShaderCode );
}

FORCEINLINE CRHIRayGenShader* CreateRayGenShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateRayGenShader( ShaderCode );
}

FORCEINLINE CRHIRayAnyHitShader* CreateRayAnyHitShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateRayAnyHitShader( ShaderCode );
}

FORCEINLINE CRHIRayClosestHitShader* CreateRayClosestHitShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateRayClosestHitShader( ShaderCode );
}

FORCEINLINE CRHIRayMissShader* CreateRayMissShader( const TArray<uint8>& ShaderCode )
{
    return GRHICore->CreateRayMissShader( ShaderCode );
}

FORCEINLINE CRHIInputLayoutState* CreateInputLayout( const SInputLayoutStateCreateInfo& CreateInfo )
{
    return GRHICore->CreateInputLayout( CreateInfo );
}

FORCEINLINE CRHIDepthStencilState* CreateDepthStencilState( const SDepthStencilStateCreateInfo& CreateInfo )
{
    return GRHICore->CreateDepthStencilState( CreateInfo );
}

FORCEINLINE CRHIRasterizerState* CreateRasterizerState( const SRasterizerStateCreateInfo& CreateInfo )
{
    return GRHICore->CreateRasterizerState( CreateInfo );
}

FORCEINLINE CRHIBlendState* CreateBlendState( const SBlendStateCreateInfo& CreateInfo )
{
    return GRHICore->CreateBlendState( CreateInfo );
}

FORCEINLINE CRHIComputePipelineState* CreateComputePipelineState( const SComputePipelineStateCreateInfo& CreateInfo )
{
    return GRHICore->CreateComputePipelineState( CreateInfo );
}

FORCEINLINE CRHIGraphicsPipelineState* CreateGraphicsPipelineState( const SGraphicsPipelineStateCreateInfo& CreateInfo )
{
    return GRHICore->CreateGraphicsPipelineState( CreateInfo );
}

FORCEINLINE CRHIRayTracingPipelineState* CreateRayTracingPipelineState( const SRayTracingPipelineStateCreateInfo& CreateInfo )
{
    return GRHICore->CreateRayTracingPipelineState( CreateInfo );
}

FORCEINLINE class CGPUProfiler* CreateProfiler()
{
    return GRHICore->CreateProfiler();
}

FORCEINLINE class CRHIViewport* CreateViewport( CCoreWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat )
{
    return GRHICore->CreateViewport( Window, Width, Height, ColorFormat, DepthFormat );
}

FORCEINLINE bool UAVSupportsFormat( EFormat Format )
{
    return GRHICore->UAVSupportsFormat( Format );
}

FORCEINLINE class IRHICommandContext* GetDefaultCommandContext()
{
    return GRHICore->GetDefaultCommandContext();
}

FORCEINLINE CString GetAdapterName()
{
    return GRHICore->GetAdapterName();
}

FORCEINLINE void CheckShadingRateSupport( SShadingRateSupport& OutSupport )
{
    GRHICore->CheckShadingRateSupport( OutSupport );
}

FORCEINLINE void CheckRayTracingSupport( SRayTracingSupport& OutSupport )
{
    GRHICore->CheckRayTracingSupport( OutSupport );
}

FORCEINLINE bool IsRayTracingSupported()
{
    SRayTracingSupport Support;
    CheckRayTracingSupport( Support );

    return (Support.Tier != ERayTracingTier::NotSupported);
}

FORCEINLINE bool IsShadingRateSupported()
{
    SShadingRateSupport Support;
    CheckShadingRateSupport( Support );

    return (Support.Tier != EShadingRateTier::NotSupported);
}
