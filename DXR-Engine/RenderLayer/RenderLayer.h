#pragma once
#include "GenericRenderLayer.h"

#define ENABLE_API_DEBUGGING 0

class RenderLayer
{
public:
    static Bool Init(ERenderLayerApi InRenderApi);
    static void Release();

    /*
    * Textures
    */

    FORCEINLINE static Texture1D* CreateTexture1D(
        const ResourceData* InitalData, 
        EFormat Format, 
        UInt32 Usage, 
        UInt32 Width, 
        UInt32 MipLevels, 
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTexture1D(
            InitalData, 
            Format, 
            Usage, 
            Width, 
            MipLevels, 
            OptimizedClearValue);
    }

    FORCEINLINE static Texture1DArray* CreateTexture1DArray(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Width,
        UInt32 MipLevels,
        UInt16 ArrayCount,
        const ClearValue& OptimizedClearValue = ClearValue())
    {
        return gRenderLayer->CreateTexture1DArray(
            InitalData,
            Format,
            Usage,
            Width,
            MipLevels,
            ArrayCount,
            OptimizedClearValue);
    }

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

    /*
    * Samplers
    */

    FORCEINLINE static class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateSamplerState(CreateInfo);
    }

    /*
    * Buffers
    */

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

    /*
    * RayTracingScene
    */

    FORCEINLINE static RayTracingScene* CreateRayTracingScene()
    {
        return gRenderLayer->CreateRayTracingScene();
    }

    /*
    * RayTracingGeometry
    */

    FORCEINLINE static RayTracingGeometry* CreateRayTracingGeometry()
    {
        return gRenderLayer->CreateRayTracingGeometry();
    }

    /*
    * ShaderResourceView
    */

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const Buffer* Buffer,
        UInt32 FirstElement,
        UInt32 ElementCount)
    {
        return gRenderLayer->CreateShaderResourceView(
            Buffer,
            FirstElement,
            ElementCount);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const Buffer* Buffer,
        UInt32 FirstElement,
        UInt32 ElementCount,
        UInt32 Stride)
    {
        return gRenderLayer->CreateShaderResourceView(
            Buffer,
            FirstElement,
            ElementCount,
            Stride);
    }

    template<typename T>
    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const Buffer* Buffer, 
        UInt32 FirstElement, 
        UInt32 ElementCount)
    {
        return CreateShaderResourceView(Buffer, FirstElement, ElementCount, sizeof(T));
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const Texture1D* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels) 
    { 
        return gRenderLayer->CreateShaderResourceView(
            Texture,
            Format,
            MostDetailedMip,
            MipLevels);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const Texture1DArray* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateShaderResourceView(
            Texture,
            Format,
            MostDetailedMip,
            MipLevels,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const Texture2D* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels)
    {
        return gRenderLayer->CreateShaderResourceView(
            Texture,
            Format,
            MostDetailedMip,
            MipLevels);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const Texture2DArray* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateShaderResourceView(
            Texture,
            Format,
            MostDetailedMip,
            MipLevels,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const TextureCube* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels)
    {
        return gRenderLayer->CreateShaderResourceView(
            Texture,
            Format,
            MostDetailedMip,
            MipLevels);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const TextureCubeArray* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateShaderResourceView(
            Texture,
            Format,
            MostDetailedMip,
            MipLevels,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
        const Texture3D* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels)
    {
        return gRenderLayer->CreateShaderResourceView(
            Texture,
            Format,
            MostDetailedMip,
            MipLevels);
    }

    /*
    * UnorderedAccessView
    */

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const Buffer* Buffer,
        UInt64 FirstElement,
        UInt32 NumElements,
        EFormat Format,
        UInt64 CounterOffsetInBytes)
    {
        return gRenderLayer->CreateUnorderedAccessView(
            Buffer,
            FirstElement,
            NumElements,
            Format,
            CounterOffsetInBytes);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const Buffer* Buffer,
        UInt64 FirstElement,
        UInt32 NumElements,
        UInt32 StructureByteStride,
        UInt64 CounterOffsetInBytes)
    {
        return gRenderLayer->CreateUnorderedAccessView(
            Buffer,
            FirstElement,
            NumElements,
            StructureByteStride,
            CounterOffsetInBytes);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const Texture1D* Texture, 
        EFormat Format, 
        UInt32 MipSlice)
    {
        return gRenderLayer->CreateUnorderedAccessView(Texture, Format, MipSlice);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const Texture1DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateUnorderedAccessView(
            Texture,
            Format,
            MipSlice,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const Texture2D* Texture, 
        EFormat Format, 
        UInt32 MipSlice)
    {
        return gRenderLayer->CreateUnorderedAccessView(Texture, Format, MipSlice);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const Texture2DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateUnorderedAccessView(
            Texture,
            Format,
            MipSlice,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const TextureCube* Texture, 
        EFormat Format, 
        UInt32 MipSlice)
    {
        return gRenderLayer->CreateUnorderedAccessView(
            Texture,
            Format,
            MipSlice);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const TextureCubeArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 ArraySlice)
    {
        return gRenderLayer->CreateUnorderedAccessView(
            Texture,
            Format,
            MipSlice,
            ArraySlice);
    }

    FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
        const Texture3D* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstDepthSlice,
        UInt32 DepthSlices) 
    {
        return gRenderLayer->CreateUnorderedAccessView(
            Texture,
            Format,
            MipSlice,
            FirstDepthSlice,
            DepthSlices);
    }

    /*
    * RenderTargetView
    */

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(
        const Texture1D* Texture, 
        EFormat Format, 
        UInt32 MipSlice)
    {
        return gRenderLayer->CreateRenderTargetView(Texture, Format, MipSlice);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(
        const Texture1DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateRenderTargetView(
            Texture,
            Format,
            MipSlice,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(
        const Texture2D* Texture, 
        EFormat Format, 
        UInt32 MipSlice)
    {
        return gRenderLayer->CreateRenderTargetView(
            Texture,
            Format,
            MipSlice);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(
        const Texture2DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateRenderTargetView(
            Texture,
            Format,
            MipSlice,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(
        const TextureCube* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FaceIndex)
    {
        return gRenderLayer->CreateRenderTargetView(
            Texture,
            Format,
            MipSlice,
            FaceIndex);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(
        const TextureCubeArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 ArraySlice,
        UInt32 FaceIndex)
    {
        return gRenderLayer->CreateRenderTargetView(
            Texture,
            Format,
            MipSlice,
            ArraySlice,
            FaceIndex);
    }

    FORCEINLINE static RenderTargetView* CreateRenderTargetView(
        const Texture3D* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstDepthSlice,
        UInt32 DepthSlices) 
    {
        return gRenderLayer->CreateRenderTargetView(
            Texture,
            Format,
            MipSlice,
            FirstDepthSlice,
            DepthSlices);
    }

    /*
    * DepthStencilView
    */

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(
        const Texture1D* Texture, 
        EFormat Format, 
        UInt32 MipSlice)
    {
        return gRenderLayer->CreateDepthStencilView(Texture, Format, MipSlice);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(
        const Texture1DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateDepthStencilView(
            Texture,
            Format,
            MipSlice,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(
        const Texture2D* Texture, 
        EFormat Format, 
        UInt32 MipSlice)
    {
        return gRenderLayer->CreateDepthStencilView(Texture, Format, MipSlice);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(
        const Texture2DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize)
    {
        return gRenderLayer->CreateDepthStencilView(
            Texture,
            Format,
            MipSlice,
            FirstArraySlice,
            ArraySize);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(
        const TextureCube* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FaceIndex)
    {
        return gRenderLayer->CreateDepthStencilView(
            Texture,
            Format,
            MipSlice,
            FaceIndex);
    }

    FORCEINLINE static DepthStencilView* CreateDepthStencilView(
        const TextureCubeArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 ArraySlice,
        UInt32 FaceIndex)
    {
        return gRenderLayer->CreateDepthStencilView(
            Texture,
            Format,
            MipSlice,
            ArraySlice,
            FaceIndex);
    }

    /*
    * Pipeline
    */

    FORCEINLINE static ComputeShader* CreateComputeShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateComputeShader(ShaderCode);
    }

    FORCEINLINE static VertexShader* CreateVertexShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateVertexShader(ShaderCode);
    }
    
    FORCEINLINE static HullShader* CreateHullShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateHullShader(ShaderCode);
    }
    
    FORCEINLINE static DomainShader* CreateDomainShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateDomainShader(ShaderCode);
    }
    
    FORCEINLINE static GeometryShader* CreateGeometryShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateGeometryShader(ShaderCode);
    }

    FORCEINLINE static MeshShader* CreateMeshShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateMeshShader(ShaderCode);
    }
    
    FORCEINLINE static AmplificationShader* CreateAmplificationShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateAmplificationShader(ShaderCode);
    }

    FORCEINLINE static PixelShader* CreatePixelShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreatePixelShader(ShaderCode);
    }

    FORCEINLINE static RayGenShader* CreateRayGenShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateRayGenShader(ShaderCode);
    }
    
    FORCEINLINE static RayHitShader* CreateRayHitShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateRayHitShader(ShaderCode);
    }

    FORCEINLINE static RayMissShader* CreateRayMissShader(
        const TArray<UInt8>& ShaderCode)
    {
        return gRenderLayer->CreateRayMissShader(ShaderCode);
    }

    FORCEINLINE static InputLayoutState* CreateInputLayout(
        const InputLayoutStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateInputLayout(CreateInfo);
    }

    FORCEINLINE static DepthStencilState* CreateDepthStencilState(
        const DepthStencilStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateDepthStencilState(CreateInfo);
    }

    FORCEINLINE static RasterizerState* CreateRasterizerState(
        const RasterizerStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateRasterizerState(CreateInfo);
    }

    FORCEINLINE static BlendState* CreateBlendState(
        const BlendStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateBlendState(CreateInfo);
    }


    FORCEINLINE static ComputePipelineState* CreateComputePipelineState(
        const ComputePipelineStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateComputePipelineState(CreateInfo);
    }

    FORCEINLINE static GraphicsPipelineState* CreateGraphicsPipelineState(
        const GraphicsPipelineStateCreateInfo& CreateInfo)
    {
        return gRenderLayer->CreateGraphicsPipelineState(CreateInfo);
    }

    /*
    * Viewport
    */

    FORCEINLINE static class Viewport* CreateViewport(
        GenericWindow* Window,
        UInt32 Width,
        UInt32 Height,
        EFormat ColorFormat,
        EFormat DepthFormat)
    {
        return gRenderLayer->CreateViewport(
            Window, 
            Width,
            Height,
            ColorFormat, 
            DepthFormat);
    }

    /*
    * Support Features
    */

    FORCEINLINE static Bool IsRayTracingSupported()
    {
        return gRenderLayer->IsRayTracingSupported();
    }

    FORCEINLINE static Bool UAVSupportsFormat(EFormat Format)
    {
        return gRenderLayer->UAVSupportsFormat(Format);
    }

    /*
    * Getters
    */

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