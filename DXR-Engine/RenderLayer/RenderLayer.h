#pragma once
#include "GenericRenderLayer.h"

#define ENABLE_API_DEBUGGING 0

class RenderLayer
{
public:
    static Bool Init(ERenderLayerApi InRenderApi);
    static void Release();

    FORCEINLINE static Texture2D* CreateTexture2D(
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

    FORCEINLINE static Texture2DArray* CreateTexture2DArray(
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

    FORCEINLINE static TextureCube* CreateTextureCube(
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

    FORCEINLINE static TextureCubeArray* CreateTextureCubeArray(
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

    FORCEINLINE static Texture3D* CreateTexture3D(
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

    FORCEINLINE static class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateSamplerState(CreateInfo);
    }

    FORCEINLINE static VertexBuffer* CreateVertexBuffer(UInt32 Stride, UInt32 NumVertices, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
    {
        return gRenderLayer->CreateVertexBuffer(Stride, NumVertices, Flags, InitialState, InitialData);
    }

    template<typename T>
    FORCEINLINE static VertexBuffer* CreateVertexBuffer(UInt32 NumVertices, UInt32 Flags, EResourceState InitialState, const ResourceData* InitalData)
    {
        constexpr UInt32 STRIDE = sizeof(T);
        return CreateVertexBuffer(STRIDE, NumVertices, Flags, InitialState, InitialData);
    }

    FORCEINLINE static IndexBuffer* CreateIndexBuffer(EIndexFormat Format, UInt32 NumIndices, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
    {
        return gRenderLayer->CreateIndexBuffer(Format, NumIndices, Flags, InitialState, InitialData);
    }

    FORCEINLINE static ConstantBuffer* CreateConstantBuffer(UInt32 SizeInBytes, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
    {
        return gRenderLayer->CreateConstantBuffer(SizeInBytes, Flags, InitialState, InitialData);
    }

    template<typename T>
    FORCEINLINE static ConstantBuffer* CreateConstantBuffer(UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
    {
        constexpr UInt32 SIZE_IN_BYTES = sizeof(T);
        return CreateConstantBuffer(SIZE_IN_BYTES, Flags, InitialState, InitialData);
    }

    FORCEINLINE static StructuredBuffer* CreateStructuredBuffer(UInt32 Stride, UInt32 NumElements, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
    {
        return gRenderLayer->CreateStructuredBuffer(Stride, NumElements, Flags, InitialState, InitialData);
    }

    FORCEINLINE static RayTracingScene* CreateRayTracingScene()
    {
        return gRenderLayer->CreateRayTracingScene();
    }

    FORCEINLINE static RayTracingGeometry* CreateRayTracingGeometry()
    {
        return gRenderLayer->CreateRayTracingGeometry();
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(Texture2D* Texture, EFormat Format, UInt32 Mip, UInt32 NumMips, Float MinMipBias)
    {
        return gRenderLayer->CreateShaderResourceView(Texture, Format, Mip, NumMips, MinMipBias);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        Texture2DArray* Texture, 
        EFormat Format, 
        UInt32 Mip, 
        UInt32 NumMips, 
        UInt32 ArraySlice, 
        UInt32 NumArraySlices, 
        Float MinMipBias)
    {
        return gRenderLayer->CreateShaderResourceView(Texture, Format, Mip, NumMips, ArraySlice, NumArraySlices, MinMipBias);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(TextureCube* Texture, EFormat Format, UInt32 Mip, UInt32 NumMips, Float MinMipBias)
    {
        return gRenderLayer->CreateShaderResourceView(Texture, Format, Mip, NumMips, MinMipBias);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        TextureCubeArray* Texture, 
        EFormat Format, 
        UInt32 Mip, 
        UInt32 NumMips, 
        UInt32 ArraySlice, 
        UInt32 NumArraySlices, 
        Float MinMipBias)
    {
        return gRenderLayer->CreateShaderResourceView(Texture, Format, Mip, NumMips, ArraySlice, NumArraySlices, MinMipBias);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(Texture3D* Texture, EFormat Format, UInt32 Mip, UInt32 NumMips, UInt32 DepthSlice, UInt32 NumDepthSlices, Float MinMipBias)
    {
        return gRenderLayer->CreateShaderResourceView(Texture, Format, Mip, NumMips, DepthSlice, NumDepthSlices, MinMipBias);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(VertexBuffer* Buffer, UInt32 FirstVertex, UInt32 NumVertices)
    {
        return gRenderLayer->CreateShaderResourceView(Buffer, FirstVertex, NumVertices);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(IndexBuffer* Buffer, UInt32 FirstIndex, UInt32 NumIndices)
    {
        return gRenderLayer->CreateShaderResourceView(Buffer, FirstIndex, NumIndices);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(StructuredBuffer* Buffer, UInt32 FirstElement, UInt32 NumElements)
    {
        return gRenderLayer->CreateShaderResourceView(Buffer, FirstElement, NumElements);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(Texture2D* Texture, EFormat Format, UInt32 Mip)
    {
        return gRenderLayer->CreateUnorderedAccessView(Texture, Format, Mip);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
    {
        return gRenderLayer->CreateUnorderedAccessView(Texture, Format, Mip, ArraySlice, NumArraySlices);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(TextureCube* Texture, EFormat Format, UInt32 Mip)
    {
        return gRenderLayer->CreateUnorderedAccessView(Texture, Format, Mip);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(TextureCubeArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
    {
        return gRenderLayer->CreateUnorderedAccessView(Texture, Format, Mip, ArraySlice, NumArraySlices);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(Texture3D* Texture, EFormat Format, UInt32 Mip, UInt32 DepthSlice, UInt32 NumDepthSlices)
    {
        return gRenderLayer->CreateUnorderedAccessView(Texture, Format, Mip, DepthSlice, NumDepthSlices);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(VertexBuffer* Buffer, UInt32 FirstVertex, UInt32 NumVertices)
    {
        return gRenderLayer->CreateUnorderedAccessView(Buffer, FirstVertex, NumVertices);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(IndexBuffer* Buffer, UInt32 FirstIndex, UInt32 NumIndices)
    {
        return gRenderLayer->CreateUnorderedAccessView(Buffer, FirstIndex, NumIndices);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(StructuredBuffer* Buffer, UInt32 FirstElement, UInt32 NumElements)
    {
        return gRenderLayer->CreateUnorderedAccessView(Buffer, FirstElement, NumElements);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(Texture2D* Texture, EFormat Format, UInt32 Mip)
    {
        return gRenderLayer->CreateRenderTargetView(Texture, Format, Mip);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
    {
        return gRenderLayer->CreateRenderTargetView(Texture, Format, Mip, ArraySlice, NumArraySlices);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(TextureCube* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip)
    {
        return gRenderLayer->CreateRenderTargetView(Texture, Format, CubeFace, Mip);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(TextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip, UInt32 ArraySlice)
    {
        return gRenderLayer->CreateRenderTargetView(Texture, Format, CubeFace, Mip, ArraySlice);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(Texture3D* Texture, EFormat Format, UInt32 Mip, UInt32 DepthSlice, UInt32 NumDepthSlices)
    {
        return gRenderLayer->CreateRenderTargetView(Texture, Format, Mip, DepthSlice, NumDepthSlices);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(Texture2D* Texture, EFormat Format, UInt32 Mip)
    {
        return gRenderLayer->CreateDepthStencilView(Texture, Format, Mip);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(Texture2DArray* Texture, EFormat Format, UInt32 Mip, UInt32 ArraySlice, UInt32 NumArraySlices)
    {
        return gRenderLayer->CreateDepthStencilView(Texture, Format, Mip, ArraySlice, NumArraySlices);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(TextureCube* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip)
    {
        return gRenderLayer->CreateDepthStencilView(Texture, Format, CubeFace, Mip);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(TextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, UInt32 Mip, UInt32 ArraySlice)
    {
        return gRenderLayer->CreateDepthStencilView(Texture, Format, CubeFace, Mip, ArraySlice);
    }

    FORCEINLINE static ComputeShader* CreateComputeShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateComputeShader(ShaderCode);
    }

    FORCEINLINE static VertexShader* CreateVertexShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateVertexShader(ShaderCode);
    }
    
    FORCEINLINE static HullShader* CreateHullShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateHullShader(ShaderCode);
    }
    
    FORCEINLINE static DomainShader* CreateDomainShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateDomainShader(ShaderCode);
    }
    
    FORCEINLINE static GeometryShader* CreateGeometryShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateGeometryShader(ShaderCode);
    }

    FORCEINLINE static MeshShader* CreateMeshShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateMeshShader(ShaderCode);
    }
    
    FORCEINLINE static AmplificationShader* CreateAmplificationShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateAmplificationShader(ShaderCode);
    }

    FORCEINLINE static PixelShader* CreatePixelShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreatePixelShader(ShaderCode);
    }

    FORCEINLINE static RayGenShader* CreateRayGenShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateRayGenShader(ShaderCode);
    }
    
    FORCEINLINE static RayHitShader* CreateRayHitShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateRayHitShader(ShaderCode);
    }

    FORCEINLINE static RayMissShader* CreateRayMissShader(const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateRayMissShader(ShaderCode);
    }

    FORCEINLINE static InputLayoutState* CreateInputLayout(const InputLayoutStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateInputLayout(CreateInfo);
    }

    FORCEINLINE static DepthStencilState* CreateDepthStencilState(const DepthStencilStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateDepthStencilState(CreateInfo);
    }

    FORCEINLINE static RasterizerState* CreateRasterizerState(const RasterizerStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateRasterizerState(CreateInfo);
    }

    FORCEINLINE static BlendState* CreateBlendState(const BlendStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateBlendState(CreateInfo);
    }

    FORCEINLINE static ComputePipelineState* CreateComputePipelineState(const ComputePipelineStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateComputePipelineState(CreateInfo);
    }

    FORCEINLINE static GraphicsPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateGraphicsPipelineState(CreateInfo);
    }

    FORCEINLINE static class Viewport* CreateViewport(GenericWindow* Window, UInt32 Width, UInt32 Height, EFormat ColorFormat, EFormat DepthFormat)
    {
        return gRenderLayer->CreateViewport(Window, Width, Height, ColorFormat, DepthFormat);
    }

    FORCEINLINE static Bool IsRayTracingSupported()
    {
        return gRenderLayer->IsRayTracingSupported();
    }

    FORCEINLINE static Bool UAVSupportsFormat(EFormat Format)
    {
        return gRenderLayer->UAVSupportsFormat(Format);
    }

    FORCEINLINE static class ICommandContext* GetDefaultCommandContext()
    {
        return gRenderLayer->GetDefaultCommandContext();
    }

    FORCEINLINE static ERenderLayerApi GetApi()
    {
        return gRenderLayer->GetApi();
    }

    FORCEINLINE static std::string GetAdapterName()
    {
        return gRenderLayer->GetAdapterName();
    }
};