#pragma once
#include "GenericRenderLayer.h"

#define ENABLE_API_DEBUGGING 0

class RenderLayer
{
public:
    static Bool Init(ERenderLayerApi InRenderApi);
    static void Release();

    FORCEINLINE static Texture2D* CreateTexture2D(
        const ResourceData* InitalData, 
        EFormat Format, 
        UInt32 Usage, 
        UInt32 Width, 
        UInt32 Height, 
        UInt32 MipLevels, 
        UInt32 SampleCount, 
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTexture2D(
            InitalData, 
            Format, 
            Usage, 
            Width, 
            Height, 
            MipLevels, 
            SampleCount, 
            OptimizedClearValue);
    }

    FORCEINLINE static Texture2DArray* CreateTexture2DArray(
        const ResourceData* InitalData, 
        EFormat Format, 
        UInt32 Usage, 
        UInt32 Width, 
        UInt32 Height, 
        UInt32 MipLevels, 
        UInt16 ArrayCount,
        UInt32 SampleCount, 
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTexture2DArray(
            InitalData, 
            Format, 
            Usage, 
            Width, 
            Height, 
            MipLevels, 
            ArrayCount,
            SampleCount, 
            OptimizedClearValue);
    }

    FORCEINLINE static TextureCube* CreateTextureCube(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Size,
        UInt32 MipLevels,
        UInt32 SampleCount,
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTextureCube(
            InitalData, 
            Format,
            Usage,
            Size,
            MipLevels,
            SampleCount,
            OptimizedClearValue);
    }

    FORCEINLINE static TextureCubeArray* CreateTextureCubeArray(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Size,
        UInt32 MipLevels,
        UInt16 ArrayCount,
        UInt32 SampleCount,
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTextureCubeArray(
            InitalData,
            Format,
            Usage,
            Size,
            MipLevels,
            ArrayCount,
            SampleCount,
            OptimizedClearValue);
    }

    FORCEINLINE static Texture3D* CreateTexture3D(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Width,
        UInt32 Height,
        UInt16 Depth,
        UInt32 MipLevels,
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTexture3D(
            InitalData,
            Format,
            Usage,
            Width,
            Height,
            Depth,
            MipLevels,
            OptimizedClearValue);
    }

    FORCEINLINE static class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateSamplerState(CreateInfo);
    }

    FORCEINLINE static VertexBuffer* CreateVertexBuffer(
        const ResourceData* InitalData,
        UInt32 SizeInBytes,
        UInt32 VertexStride,
        UInt32 Usage)
    {
        return gRenderLayer->CreateVertexBuffer(
            InitalData,
            SizeInBytes,
            VertexStride,
            Usage);
    }

    template<typename T>
    FORCEINLINE static VertexBuffer* CreateVertexBuffer(
        const ResourceData* InitalData, 
        UInt32 VertexCount, 
        UInt32 Usage)
    {
        constexpr UInt32 STRIDE = sizeof(T);
        const UInt32 SizeInByte = STRIDE * VertexCount;
        return CreateVertexBuffer(InitalData, SizeInByte, STRIDE, Usage);
    }

    FORCEINLINE static IndexBuffer* CreateIndexBuffer(
        const ResourceData* InitalData,
        UInt32 SizeInBytes,
        EIndexFormat IndexFormat,
        UInt32 Usage)
    {
        return gRenderLayer->CreateIndexBuffer(
            InitalData,
            SizeInBytes,
            IndexFormat,
            Usage);
    }

    FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
        const ResourceData* InitalData,
        UInt32 SizeInBytes, 
        UInt32 Usage,
        EResourceState InitialState)
    {
        return gRenderLayer->CreateConstantBuffer(
            InitalData, 
            SizeInBytes, 
            Usage, 
            InitialState);
    }

    template<typename T>
    FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
        const ResourceData* InitalData, 
        UInt32 Usage,
        EResourceState InitialState)
    {
        return CreateConstantBuffer(
            InitalData, 
            sizeof(T), 
            Usage, 
            InitialState);
    }

    template<typename T>
    FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
        const ResourceData* InitalData, 
        UInt32 ElementCount, 
        UInt32 Usage,
        EResourceState InitialState)
    {
        return CreateConstantBuffer(
            InitalData, 
            sizeof(T) * ElementCount, 
            Usage, 
            InitialState);
    }

    FORCEINLINE static StructuredBuffer* CreateStructuredBuffer(
        const ResourceData* InitalData,
        UInt32 SizeInBytes,
        UInt32 Stride,
        UInt32 Usage)
    {
        return gRenderLayer->CreateStructuredBuffer(
            InitalData,
            SizeInBytes,
            Stride,
            Usage);
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