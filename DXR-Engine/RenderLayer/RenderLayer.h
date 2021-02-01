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
        UInt32 NumMipLevels, 
        UInt32 NumSamples, 
        UInt32 Usage, 
        EResourceState InitialState,
        const ResourceData* InitialData, 
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTexture2D(Format, Width, Height, NumMipLevels, NumSamples, Usage, InitialState, InitialData, OptimizedClearValue);
    }

    FORCEINLINE static Texture2DArray* CreateTexture2DArray(
        EFormat Format, 
        UInt32 Width, 
        UInt32 Height, 
        UInt32 NumMipLevels, 
        UInt32 NumSamples,
        UInt32 NumArraySlices,
        UInt32 Usage, 
        EResourceState InitialState,
        const ResourceData* InitialData, 
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTexture2DArray(Format, Width, Height, NumMipLevels, NumSamples, NumArraySlices, Usage, InitialState, InitialData, OptimizedClearValue);
    }

    FORCEINLINE static TextureCube* CreateTextureCube(
        EFormat Format, 
        UInt32 Size,
        UInt32 NumMipLevels,
        UInt32 Usage, 
        EResourceState InitialState,
        const ResourceData* InitialData, 
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTextureCube(Format, Size, NumMipLevels, Usage, InitialState, InitialData, OptimizedClearValue);
    }

    FORCEINLINE static TextureCubeArray* CreateTextureCubeArray(
        EFormat Format, 
        UInt32 Size, 
        UInt32 NumMipLevels, 
        UInt32 NumArraySlices,
        UInt32 Usage, 
        EResourceState InitialState,
        const ResourceData* InitialData, 
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTextureCubeArray(Format, Size, NumMipLevels, NumArraySlices, Usage, InitialState, InitialData, OptimizedClearValue);
    }

    FORCEINLINE static Texture3D* CreateTexture3D(
        EFormat Format, 
        UInt32 Width,
        UInt32 Height, 
        UInt32 Depth,
        UInt32 NumMipLevels, 
        UInt32 Usage, 
        EResourceState InitialState,
        const ResourceData* InitialData, 
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTexture3D(Format, Width, Height, Depth, NumMipLevels, Usage, InitialState, InitialData, OptimizedClearValue);
    }

    FORCEINLINE static class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateSamplerState(CreateInfo);
    }

    FORCEINLINE static VertexBuffer* CreateVertexBuffer(UInt32 Stride, UInt32 NumVertices, UInt32 Usage, EResourceState InitialState, const ResourceData* InitialData)
    {
        return gRenderLayer->CreateVertexBuffer(Stride, NumVertices, Usage, InitialState, InitialData);
    }

    template<typename T>
    FORCEINLINE static VertexBuffer* CreateVertexBuffer(UInt32 NumVertices, UInt32 Usage, EResourceState InitialState, const ResourceData* InitalData)
    {
        constexpr UInt32 STRIDE = sizeof(T);
        return CreateVertexBuffer(STRIDE, NumVertices, Usage, InitialState, InitialData);
    }

    FORCEINLINE static IndexBuffer* CreateIndexBuffer(EIndexFormat Format, UInt32 NumIndices, UInt32 Usage, EResourceState InitialState, const ResourceData* InitialData)
    {
        return gRenderLayer->CreateIndexBuffer(Format, NumIndices, Usage, InitialState, InitialData);
    }

    FORCEINLINE static ConstantBuffer* CreateConstantBuffer(UInt32 SizeInBytes, EResourceState InitialState, const ResourceData* InitialData)
    {
        return gRenderLayer->CreateConstantBuffer(SizeInBytes, InitialState, InitialData);
    }

    template<typename T>
    FORCEINLINE static ConstantBuffer* CreateConstantBuffer(EResourceState InitialState, const ResourceData* InitialData)
    {
        constexpr UInt32 SIZE_IN_BYTES = sizeof(T);
        return CreateConstantBuffer(SIZE_IN_BYTES, InitialState, InitialData);
    }

    FORCEINLINE static StructuredBuffer* CreateStructuredBuffer(UInt32 Stride, UInt32 NumElements, UInt32 Usage, EResourceState InitialState, const ResourceData* InitialData)
    {
        return gRenderLayer->CreateStructuredBuffer(Stride, NumElements, Usage, InitialState, InitialData);
    }

    FORCEINLINE static RayTracingScene* CreateRayTracingScene()
    {
        return gRenderLayer->CreateRayTracingScene();
    }

    FORCEINLINE static RayTracingGeometry* CreateRayTracingGeometry()
    {
        return gRenderLayer->CreateRayTracingGeometry();
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(const ShaderResourceViewCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateShaderResourceView(CreateInfo);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(const UnorderedAccessViewCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateUnorderedAccessView(CreateInfo);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(const RenderTargetViewCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateRenderTargetView(CreateInfo);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(const DepthStencilViewCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateDepthStencilView(CreateInfo);
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