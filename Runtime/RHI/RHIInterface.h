#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHICommandList.h"
#include "RHIModule.h"

#include "CoreApplication/Interface/PlatformWindow.h"

struct SResourceData;
struct SClearValue;
class CRHIRayTracingGeometry;
class CRHIRayTracingScene;

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CRHIInterface
{
public:

    virtual ~CRHIInterface() = default;

    virtual bool Init(bool bEnableDebug) = 0;

    virtual CRHITexture2D* CreateTexture2D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMips,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) = 0;

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
        const SClearValue& OptimizedClearValue) = 0;

    virtual CRHITextureCube* CreateTextureCube(
        EFormat Format,
        uint32 Size,
        uint32 NumMips,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) = 0;

    virtual CRHITextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        uint32 Size,
        uint32 NumMips,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) = 0;

    virtual CRHITexture3D* CreateTexture3D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 Depth,
        uint32 NumMips,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) = 0;

    virtual class CRHISamplerState* CreateSamplerState(const struct SSamplerStateCreateInfo& CreateInfo) = 0;

    virtual CRHIVertexBuffer* CreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData) = 0;
    virtual CRHIIndexBuffer* CreateIndexBuffer(EIndexFormat Format, uint32 NumIndices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData) = 0;
    virtual CRHIConstantBuffer* CreateConstantBuffer(uint32 Size, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData) = 0;
    virtual CRHIStructuredBuffer* CreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData) = 0;

    virtual CRHIRayTracingScene* CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances) = 0;
    virtual CRHIRayTracingGeometry* CreateRayTracingGeometry(uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer) = 0;

    virtual CRHIShaderResourceView* CreateShaderResourceView(const SShaderResourceViewCreateInfo& CreateInfo) = 0;
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SUnorderedAccessViewCreateInfo& CreateInfo) = 0;
    virtual CRHIRenderTargetView* CreateRenderTargetView(const SRenderTargetViewCreateInfo& CreateInfo) = 0;
    virtual CRHIDepthStencilView* CreateDepthStencilView(const SDepthStencilViewCreateInfo& CreateInfo) = 0;

    virtual class CRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) = 0;

    virtual class CRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) = 0;

    virtual class CRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) = 0;
    virtual class CRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) = 0;

    virtual class CRHIDepthStencilState* CreateDepthStencilState(const SDepthStencilStateCreateInfo& CreateInfo) = 0;

    virtual class CRHIRasterizerState* CreateRasterizerState(const SRasterizerStateCreateInfo& CreateInfo) = 0;

    virtual class CRHIBlendState* CreateBlendState(const SBlendStateCreateInfo& CreateInfo) = 0;

    virtual class CRHIInputLayoutState* CreateInputLayout(const SInputLayoutStateCreateInfo& CreateInfo) = 0;

    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState(const SGraphicsPipelineStateCreateInfo& CreateInfo) = 0;
    virtual class CRHIComputePipelineState* CreateComputePipelineState(const SComputePipelineStateCreateInfo& CreateInfo) = 0;
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRayTracingPipelineStateCreateInfo& CreateInfo) = 0;

    virtual class CRHITimestampQuery* CreateTimestampQuery() = 0;

    virtual class CRHIViewport* CreateViewport(CPlatformWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat) = 0;

    virtual class IRHICommandContext* GetDefaultCommandContext() = 0;

    virtual CString GetAdapterName() const
    {
        return CString();
    }

    virtual void CheckRayTracingSupport(SRayTracingSupport& OutSupport) const = 0;
    virtual void CheckShadingRateSupport(SShadingRateSupport& OutSupport) const = 0;

    virtual bool UAVSupportsFormat(EFormat Format) const
    {
        UNREFERENCED_VARIABLE(Format);
        return false;
    }

    FORCEINLINE ERHIModule GetApi() const
    {
        return CurrentRHI;
    }

protected:

    FORCEINLINE CRHIInterface(ERHIModule InCurrentRHI)
        : CurrentRHI(InCurrentRHI)
    {
    }

    ERHIModule CurrentRHI;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions

FORCEINLINE CRHITexture2D* RHICreateTexture2D(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 NumMips,
    uint32 NumSamples,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInterface->CreateTexture2D(Format, Width, Height, NumMips, NumSamples, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHITexture2DArray* RHICreateTexture2DArray(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 NumMips,
    uint32 NumSamples,
    uint32 NumArraySlices,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInterface->CreateTexture2DArray(Format, Width, Height, NumMips, NumSamples, NumArraySlices, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHITextureCube* RHICreateTextureCube(
    EFormat Format,
    uint32 Size,
    uint32 NumMips,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInterface->CreateTextureCube(Format, Size, NumMips, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHITextureCubeArray* RHICreateTextureCubeArray(
    EFormat Format,
    uint32 Size,
    uint32 NumMips,
    uint32 NumArraySlices,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInterface->CreateTextureCubeArray(Format, Size, NumMips, NumArraySlices, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHITexture3D* RHICreateTexture3D(
    EFormat Format,
    uint32 Width,
    uint32 Height,
    uint32 Depth,
    uint32 NumMips,
    uint32 Flags,
    EResourceState InitialState,
    const SResourceData* InitialData = nullptr,
    const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInterface->CreateTexture3D(Format, Width, Height, Depth, NumMips, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE class CRHISamplerState* RHICreateSamplerState(const struct SSamplerStateCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateSamplerState(CreateInfo);
}

FORCEINLINE CRHIVertexBuffer* RHICreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData)
{
    return GRHIInterface->CreateVertexBuffer(Stride, NumVertices, Flags, InitialState, InitialData);
}

template<typename T>
FORCEINLINE CRHIVertexBuffer* RHICreateVertexBuffer(uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData)
{
    constexpr uint32 STRIDE = sizeof(T);
    return RHICreateVertexBuffer(STRIDE, NumVertices, Flags, InitialState, InitialData);
}

FORCEINLINE CRHIIndexBuffer* RHICreateIndexBuffer(EIndexFormat Format, uint32 NumIndices, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData)
{
    return GRHIInterface->CreateIndexBuffer(Format, NumIndices, Flags, InitialState, InitialData);
}

FORCEINLINE CRHIConstantBuffer* RHICreateConstantBuffer(uint32 Size, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData)
{
    return GRHIInterface->CreateConstantBuffer(Size, Flags, InitialState, InitialData);
}

template<typename TSize>
FORCEINLINE CRHIConstantBuffer* RHICreateConstantBuffer(uint32 Flags, EResourceState InitialState, const SResourceData* InitialData)
{
    constexpr uint32 SIZE_IN_BYTES = sizeof(TSize);
    return RHICreateConstantBuffer(SIZE_IN_BYTES, Flags, InitialState, InitialData);
}

FORCEINLINE CRHIStructuredBuffer* RHICreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData)
{
    return GRHIInterface->CreateStructuredBuffer(Stride, NumElements, Flags, InitialState, InitialData);
}

template<typename TStride>
FORCEINLINE CRHIStructuredBuffer* RHICreateStructuredBuffer(uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData)
{
    constexpr uint32 STRIDE_IN_BYTES = sizeof(TStride);
    return RHICreateStructuredBuffer(STRIDE_IN_BYTES, NumElements, Flags, InitialState, InitialData);
}

FORCEINLINE CRHIRayTracingScene* RHICreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    return GRHIInterface->CreateRayTracingScene(Flags, Instances, NumInstances);
}

FORCEINLINE CRHIRayTracingGeometry* RHICreateRayTracingGeometry(uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer)
{
    return GRHIInterface->CreateRayTracingGeometry(Flags, VertexBuffer, IndexBuffer);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(const SShaderResourceViewCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture2D* Texture, EFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias)
{
    SShaderResourceViewCreateInfo CreateInfo(SShaderResourceViewCreateInfo::EType::Texture2D);
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format = Format;
    CreateInfo.Texture2D.Mip = Mip;
    CreateInfo.Texture2D.NumMips = NumMips;
    CreateInfo.Texture2D.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 ArraySlice, uint32 NumArraySlices, float MinMipBias)
{
    SShaderResourceViewCreateInfo CreateInfo(SShaderResourceViewCreateInfo::EType::Texture2DArray);
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Format = Format;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.NumMips = NumMips;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    CreateInfo.Texture2DArray.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITextureCube* Texture, EFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias)
{
    SShaderResourceViewCreateInfo CreateInfo(SShaderResourceViewCreateInfo::EType::TextureCube);
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format = Format;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.NumMips = NumMips;
    CreateInfo.TextureCube.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITextureCubeArray* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 ArraySlice, uint32 NumArraySlices, float MinMipBias)
{
    SShaderResourceViewCreateInfo CreateInfo(SShaderResourceViewCreateInfo::EType::TextureCubeArray);
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Format = Format;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.NumMips = NumMips;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    CreateInfo.TextureCubeArray.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 DepthSlice, uint32 NumDepthSlices, float MinMipBias)
{
    SShaderResourceViewCreateInfo CreateInfo(SShaderResourceViewCreateInfo::EType::Texture3D);
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Format = Format;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.NumMips = NumMips;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    CreateInfo.Texture3D.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHIVertexBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices)
{
    SShaderResourceViewCreateInfo CreateInfo(SShaderResourceViewCreateInfo::EType::VertexBuffer);
    CreateInfo.VertexBuffer.Buffer = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHIIndexBuffer* Buffer, uint32 FirstIndex, uint32 NumIndices)
{
    SShaderResourceViewCreateInfo CreateInfo(SShaderResourceViewCreateInfo::EType::IndexBuffer);
    CreateInfo.IndexBuffer.Buffer = Buffer;
    CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
    CreateInfo.IndexBuffer.NumIndices = NumIndices;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHIStructuredBuffer* Buffer, uint32 FirstElement, uint32 NumElements)
{
    SShaderResourceViewCreateInfo CreateInfo(SShaderResourceViewCreateInfo::EType::StructuredBuffer);
    CreateInfo.StructuredBuffer.Buffer = Buffer;
    CreateInfo.StructuredBuffer.FirstElement = FirstElement;
    CreateInfo.StructuredBuffer.NumElements = NumElements;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const SUnorderedAccessViewCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SUnorderedAccessViewCreateInfo CreateInfo(SUnorderedAccessViewCreateInfo::EType::Texture2D);
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format = Format;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SUnorderedAccessViewCreateInfo CreateInfo(SUnorderedAccessViewCreateInfo::EType::Texture2DArray);
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Format = Format;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITextureCube* Texture, EFormat Format, uint32 Mip)
{
    SUnorderedAccessViewCreateInfo CreateInfo(SUnorderedAccessViewCreateInfo::EType::TextureCube);
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format = Format;
    CreateInfo.TextureCube.Mip = Mip;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITextureCubeArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SUnorderedAccessViewCreateInfo CreateInfo(SUnorderedAccessViewCreateInfo::EType::TextureCubeArray);
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Format = Format;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices)
{
    SUnorderedAccessViewCreateInfo CreateInfo(SUnorderedAccessViewCreateInfo::EType::Texture3D);
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Format = Format;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHIVertexBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices)
{
    SUnorderedAccessViewCreateInfo CreateInfo(SUnorderedAccessViewCreateInfo::EType::VertexBuffer);
    CreateInfo.VertexBuffer.Buffer = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHIIndexBuffer* Buffer, uint32 FirstIndex, uint32 NumIndices)
{
    SUnorderedAccessViewCreateInfo CreateInfo(SUnorderedAccessViewCreateInfo::EType::IndexBuffer);
    CreateInfo.IndexBuffer.Buffer = Buffer;
    CreateInfo.IndexBuffer.FirstIndex = FirstIndex;
    CreateInfo.IndexBuffer.NumIndices = NumIndices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHIStructuredBuffer* Buffer, uint32 FirstElement, uint32 NumElements)
{
    SUnorderedAccessViewCreateInfo CreateInfo(SUnorderedAccessViewCreateInfo::EType::StructuredBuffer);
    CreateInfo.StructuredBuffer.Buffer = Buffer;
    CreateInfo.StructuredBuffer.FirstElement = FirstElement;
    CreateInfo.StructuredBuffer.NumElements = NumElements;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(const SRenderTargetViewCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SRenderTargetViewCreateInfo CreateInfo(SRenderTargetViewCreateInfo::EType::Texture2D);
    CreateInfo.Format = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRenderTargetViewCreateInfo CreateInfo(SRenderTargetViewCreateInfo::EType::Texture2DArray);
    CreateInfo.Format = Format;
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITextureCube* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip)
{
    SRenderTargetViewCreateInfo CreateInfo(SRenderTargetViewCreateInfo::EType::TextureCube);
    CreateInfo.Format = Format;
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip, uint32 ArraySlice)
{
    SRenderTargetViewCreateInfo CreateInfo(SRenderTargetViewCreateInfo::EType::TextureCubeArray);
    CreateInfo.Format = Format;
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace = CubeFace;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices)
{
    SRenderTargetViewCreateInfo CreateInfo(SRenderTargetViewCreateInfo::EType::Texture3D);
    CreateInfo.Format = Format;
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(const SDepthStencilViewCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SDepthStencilViewCreateInfo CreateInfo(SDepthStencilViewCreateInfo::EType::Texture2D);
    CreateInfo.Format = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SDepthStencilViewCreateInfo CreateInfo(SDepthStencilViewCreateInfo::EType::Texture2DArray);
    CreateInfo.Format = Format;
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITextureCube* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip)
{
    SDepthStencilViewCreateInfo CreateInfo(SDepthStencilViewCreateInfo::EType::TextureCube);
    CreateInfo.Format = Format;
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, uint32 Mip, uint32 ArraySlice)
{
    SDepthStencilViewCreateInfo CreateInfo(SDepthStencilViewCreateInfo::EType::TextureCubeArray);
    CreateInfo.Format = Format;
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace = CubeFace;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateComputeShader(ShaderCode);
}

FORCEINLINE CRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateVertexShader(ShaderCode);
}

FORCEINLINE CRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateHullShader(ShaderCode);
}

FORCEINLINE CRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateDomainShader(ShaderCode);
}

FORCEINLINE CRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateGeometryShader(ShaderCode);
}

FORCEINLINE CRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateMeshShader(ShaderCode);
}

FORCEINLINE CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateAmplificationShader(ShaderCode);
}

FORCEINLINE CRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreatePixelShader(ShaderCode);
}

FORCEINLINE CRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateRayGenShader(ShaderCode);
}

FORCEINLINE CRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateRayAnyHitShader(ShaderCode);
}

FORCEINLINE CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateRayClosestHitShader(ShaderCode);
}

FORCEINLINE CRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInterface->CreateRayMissShader(ShaderCode);
}

FORCEINLINE CRHIInputLayoutState* RHICreateInputLayout(const SInputLayoutStateCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateInputLayout(CreateInfo);
}

FORCEINLINE CRHIDepthStencilState* RHICreateDepthStencilState(const SDepthStencilStateCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateDepthStencilState(CreateInfo);
}

FORCEINLINE CRHIRasterizerState* RHICreateRasterizerState(const SRasterizerStateCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateRasterizerState(CreateInfo);
}

FORCEINLINE CRHIBlendState* RHICreateBlendState(const SBlendStateCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateBlendState(CreateInfo);
}

FORCEINLINE CRHIComputePipelineState* RHICreateComputePipelineState(const SComputePipelineStateCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateComputePipelineState(CreateInfo);
}

FORCEINLINE CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const SGraphicsPipelineStateCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateGraphicsPipelineState(CreateInfo);
}

FORCEINLINE CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const SRayTracingPipelineStateCreateInfo& CreateInfo)
{
    return GRHIInterface->CreateRayTracingPipelineState(CreateInfo);
}

FORCEINLINE class CRHITimestampQuery* RHICreateTimestampQuery()
{
    return GRHIInterface->CreateTimestampQuery();
}

FORCEINLINE class CRHIViewport* RHICreateViewport(CPlatformWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat)
{
    return GRHIInterface->CreateViewport(Window, Width, Height, ColorFormat, DepthFormat);
}

FORCEINLINE bool RHIUAVSupportsFormat(EFormat Format)
{
    return GRHIInterface->UAVSupportsFormat(Format);
}

FORCEINLINE class IRHICommandContext* RHIGetDefaultCommandContext()
{
    return GRHIInterface->GetDefaultCommandContext();
}

FORCEINLINE CString RHIGetAdapterName()
{
    return GRHIInterface->GetAdapterName();
}

FORCEINLINE void RHICheckShadingRateSupport(SShadingRateSupport& OutSupport)
{
    GRHIInterface->CheckShadingRateSupport(OutSupport);
}

FORCEINLINE void RHICheckRayTracingSupport(SRayTracingSupport& OutSupport)
{
    GRHIInterface->CheckRayTracingSupport(OutSupport);
}

FORCEINLINE bool RHISupportsRayTracing()
{
    SRayTracingSupport Support;
    RHICheckRayTracingSupport(Support);

    return (Support.Tier != ERayTracingTier::NotSupported);
}

FORCEINLINE bool RHISupportsVariableRateShading()
{
    SShadingRateSupport Support;
    RHICheckShadingRateSupport(Support);

    return (Support.Tier != EShadingRateTier::NotSupported);
}
