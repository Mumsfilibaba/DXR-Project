#pragma once
#include "RenderLayer/GenericRenderLayer.h"

#include "Windows/WindowsWindow.h"

#include "D3D12Device.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"

class D3D12CommandContext;
class D3D12Buffer;

template<typename TD3D12Texture>
D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension();

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<D3D12Texture1D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<D3D12Texture1DArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<D3D12Texture2D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<D3D12Texture2DArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<D3D12TextureCube>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<D3D12TextureCubeArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<D3D12Texture3D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
}

class D3D12RenderLayer : public GenericRenderLayer
{
public:
    D3D12RenderLayer();
    ~D3D12RenderLayer();

    virtual Bool Init(Bool EnableDebug) override final;

    /*
    * Textures
    */

    virtual Texture1D* CreateTexture1D(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Width,
        UInt32 MipLevels,
        const ClearValue& OptimizedClearValue) const override final;

    virtual Texture1DArray* CreateTexture1DArray(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Width,
        UInt32 MipLevels,
        UInt16 ArrayCount,
        const ClearValue& OptimizedClearValue) const override final;

    virtual Texture2D* CreateTexture2D(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Width,
        UInt32 Height,
        UInt32 MipLevels,
        UInt32 SampleCount,
        const ClearValue& OptimizedClearValue) const override final;

    virtual Texture2DArray* CreateTexture2DArray(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Width,
        UInt32 Height,
        UInt32 MipLevels,
        UInt16 ArrayCount,
        UInt32 SampleCount,
        const ClearValue& OptimizedClearValue) const override final;

    virtual TextureCube* CreateTextureCube(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Size,
        UInt32 MipLevels,
        UInt32 SampleCount,
        const ClearValue& OptimizedClearValue) const override final;

    virtual TextureCubeArray* CreateTextureCubeArray(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Size,
        UInt32 MipLevels,
        UInt16 ArrayCount,
        UInt32 SampleCount,
        const ClearValue& OptimizedClearValue) const override final;

    virtual Texture3D* CreateTexture3D(
        const ResourceData* InitalData,
        EFormat Format,
        UInt32 Usage,
        UInt32 Width,
        UInt32 Height,
        UInt16 Depth,
        UInt32 MipLevels,
        const ClearValue& OptimizedClearValue) const override final;

    /*
    * Samplers
    */

    virtual class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo) const override final;

    /*
    * Buffers
    */

    virtual VertexBuffer* CreateVertexBuffer(
        const ResourceData* InitalData,
        UInt32 SizeInBytes,
        UInt32 VertexStride,
        UInt32 Usage) const override final;

    virtual IndexBuffer* CreateIndexBuffer(
        const ResourceData* InitalData,
        UInt32 SizeInBytes,
        EIndexFormat IndexFormat,
        UInt32 Usage) const override final;

    virtual ConstantBuffer* CreateConstantBuffer(
        const ResourceData* InitalData, 
        UInt32 SizeInBytes, 
        UInt32 Usage,
        EResourceState InitialState) const override final;

    virtual StructuredBuffer* CreateStructuredBuffer(
        const ResourceData* InitalData,
        UInt32 SizeInBytes,
        UInt32 Stride,
        UInt32 Usage) const override final;

    /*
    * RayTracing
    */

    virtual class RayTracingGeometry* CreateRayTracingGeometry() const override final;
    virtual class RayTracingScene*    CreateRayTracingScene()    const override final;

    /*
    * ShaderResourceView
    */

    virtual ShaderResourceView* CreateShaderResourceView(
        const Buffer* Buffer,
        UInt32 FirstElement,
        UInt32 ElementCount) const override final;

    virtual ShaderResourceView* CreateShaderResourceView(
        const Buffer* Buffer,
        UInt32 FirstElement,
        UInt32 ElementCount,
        UInt32 Stride) const override final;

    virtual ShaderResourceView* CreateShaderResourceView(
        const Texture1D* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels) const override final;

    virtual ShaderResourceView* CreateShaderResourceView(
        const Texture1DArray* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual ShaderResourceView* CreateShaderResourceView(
        const Texture2D* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels) const override final;

    virtual ShaderResourceView* CreateShaderResourceView(
        const Texture2DArray* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual ShaderResourceView* CreateShaderResourceView(
        const TextureCube* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels) const override final;

    virtual ShaderResourceView* CreateShaderResourceView(
        const TextureCubeArray* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual ShaderResourceView* CreateShaderResourceView(
        const Texture3D* Texture,
        EFormat Format,
        UInt32 MostDetailedMip,
        UInt32 MipLevels) const override final;

    /*
    * UnorderedAccessView
    */

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const Buffer* Buffer,
        UInt64 FirstElement,
        UInt32 NumElements,
        EFormat Format,
        UInt64 CounterOffsetInBytes) const override final;

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const Buffer* Buffer,
        UInt64 FirstElement,
        UInt32 NumElements,
        UInt32 StructureByteStride,
        UInt64 CounterOffsetInBytes) const override final;

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const Texture1D* Texture, 
        EFormat Format, 
        UInt32 MipSlice) const override final;

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const Texture1DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const Texture2D* Texture, 
        EFormat Format, 
        UInt32 MipSlice) const override final;

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const Texture2DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const TextureCube* Texture, 
        EFormat Format, 
        UInt32 MipSlice) const override final;

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const TextureCubeArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 ArraySlice) const override final;

    virtual UnorderedAccessView* CreateUnorderedAccessView(
        const Texture3D* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstDepthSlice,
        UInt32 DepthSlices) const override final;

    /*
    * RenderTargetView
    */

    virtual RenderTargetView* CreateRenderTargetView(
        const Texture1D* Texture, 
        EFormat Format, 
        UInt32 MipSlice) const override final;

    virtual RenderTargetView* CreateRenderTargetView(
        const Texture1DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual RenderTargetView* CreateRenderTargetView(
        const Texture2D* Texture, 
        EFormat Format, 
        UInt32 MipSlice) const override final;

    virtual RenderTargetView* CreateRenderTargetView(
        const Texture2DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual RenderTargetView* CreateRenderTargetView(
        const TextureCube* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FaceIndex) const override final;

    virtual RenderTargetView* CreateRenderTargetView(
        const TextureCubeArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 ArraySlice,
        UInt32 FaceIndex) const override final;

    virtual RenderTargetView* CreateRenderTargetView(
        const Texture3D* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstDepthSlice,
        UInt32 DepthSlices) const override final;

    /*
    * DepthStencilView
    */
    
    virtual DepthStencilView* CreateDepthStencilView(
        const Texture1D* Texture, 
        EFormat Format, 
        UInt32 MipSlice) const override final;

    virtual DepthStencilView* CreateDepthStencilView(
        const Texture1DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual DepthStencilView* CreateDepthStencilView(
        const Texture2D* Texture, 
        EFormat Format, 
        UInt32 MipSlice) const override final;

    virtual DepthStencilView* CreateDepthStencilView(
        const Texture2DArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FirstArraySlice,
        UInt32 ArraySize) const override final;

    virtual DepthStencilView* CreateDepthStencilView(
        const TextureCube* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 FaceIndex) const override final;

    virtual DepthStencilView* CreateDepthStencilView(
        const TextureCubeArray* Texture,
        EFormat Format,
        UInt32 MipSlice,
        UInt32 ArraySlice,
        UInt32 FaceIndex) const override final;

    /*
    * Pipeline
    */

    virtual class ComputeShader* CreateComputeShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class VertexShader* CreateVertexShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class HullShader* CreateHullShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class DomainShader* CreateDomainShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class GeometryShader* CreateGeometryShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class MeshShader* CreateMeshShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class AmplificationShader* CreateAmplificationShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class PixelShader* CreatePixelShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class RayGenShader* CreateRayGenShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class RayHitShader* CreateRayHitShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class RayMissShader* CreateRayMissShader(
        const TArray<UInt8>& ShaderCode) const override final;

    virtual class DepthStencilState* CreateDepthStencilState(
        const DepthStencilStateCreateInfo& CreateInfo) const override final;

    virtual class RasterizerState* CreateRasterizerState(
        const RasterizerStateCreateInfo& CreateInfo) const override final;

    virtual class BlendState* CreateBlendState(
        const BlendStateCreateInfo& CreateInfo) const override final;
    
    virtual class InputLayoutState*    CreateInputLayout(
        const InputLayoutStateCreateInfo& CreateInfo) const override final;

    virtual class GraphicsPipelineState* CreateGraphicsPipelineState(
        const GraphicsPipelineStateCreateInfo& CreateInfo) const override final;
    
    virtual class ComputePipelineState* CreateComputePipelineState(
        const ComputePipelineStateCreateInfo& Info) const override final;
    
    virtual class RayTracingPipelineState* CreateRayTracingPipelineState() const override final;

    /*
    * Viewport
    */

    virtual class Viewport* CreateViewport(
        GenericWindow* Window,
        UInt32 Width,
        UInt32 Height,
        EFormat ColorFormat,
        EFormat DepthFormat) const override final;

    /*
    * Supported features
    */

    virtual Bool IsRayTracingSupported()           const override final;
    virtual Bool UAVSupportsFormat(EFormat Format) const override final;
    
    virtual class ICommandContext* GetDefaultCommandContext() const override final
    {
        return DirectCmdContext.Get();
    }

    virtual std::string GetAdapterName() const override final
    {
        return Device->GetAdapterName();
    }

private:
    Bool AllocateBuffer(
        D3D12Resource& Resource, 
        D3D12_HEAP_TYPE HeapType, 
        D3D12_RESOURCE_STATES InitalState, 
        D3D12_RESOURCE_FLAGS Flags, 
        UInt32 SizeInBytes) const;

    Bool AllocateTexture(
        D3D12Resource& Resource,
        D3D12_HEAP_TYPE HeapType,
        D3D12_RESOURCE_STATES InitalState,
        D3D12_CLEAR_VALUE* OptimizedClearValue,
        const D3D12_RESOURCE_DESC& Desc) const;
    
    Bool UploadBuffer(
        Buffer& Buffer, 
        UInt32 SizeInBytes, 
        const ResourceData* InitalData) const;
    
    Bool UploadTexture(
        Texture& Texture, 
        const ResourceData* InitalData) const;

    /*
    * Creation of basic resources
    */

    template<typename TD3D12Buffer, typename... TArgs>
    FORCEINLINE TD3D12Buffer* CreateBufferResource(
        const ResourceData* InitalData,
        EResourceState InitialState,
        TArgs&&... Args) const
    {
        // Create buffer object and get size to allocate
        TD3D12Buffer* NewBuffer  = DBG_NEW TD3D12Buffer(Device, Forward<TArgs>(Args)...);
        const UInt64 Alignment   = NewBuffer->GetRequiredAlignment();
        const UInt64 SizeInBytes = NewBuffer->GetSizeInBytes();
        const UInt32 AlignedSize = UInt32(Math::AlignUp<UInt64>(SizeInBytes, Alignment));

        // Get properties based on Usage
        const UInt32 Usage = NewBuffer->GetUsage();
        const D3D12_RESOURCE_FLAGS Flags = ConvertBufferUsage(Usage);

        D3D12_HEAP_TYPE HeapType          = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_STATES InitalState = D3D12_RESOURCE_STATE_COMMON;
        if (Usage & BufferUsage_Dynamic)
        {
            InitalState = D3D12_RESOURCE_STATE_GENERIC_READ;
            HeapType    = D3D12_HEAP_TYPE_UPLOAD;
        }

        // Allocate
        if (!AllocateBuffer(*NewBuffer, HeapType, InitalState, Flags, AlignedSize))
        {
            LOG_ERROR("[D3D12RenderLayer]: Failed to allocate buffer");
            return nullptr;
        }

        // Upload initial
        if (InitalData)
        {
            UploadBuffer(*NewBuffer, UInt32(SizeInBytes), InitalData);
        }

        if (InitialState != EResourceState::ResourceState_Common)
        {
            DirectCmdContext->Begin();

            DirectCmdContext->TransitionBuffer(
                NewBuffer,
                EResourceState::ResourceState_Common,
                InitialState);
            
            DirectCmdContext->End();
        }

        return NewBuffer;
    }

    template<typename TD3D12Texture, typename... TArgs>
    FORCEINLINE TD3D12Texture* CreateTextureResource(
        const ResourceData* InitalData,
        TArgs&&... Args) const
    {
        TD3D12Texture* NewTexture = DBG_NEW TD3D12Texture(Device, Forward<TArgs>(Args)...);
        const EFormat Format = NewTexture->GetFormat();
        const UInt32  Usage  = NewTexture->GetUsage();
        const UInt32  Width  = NewTexture->GetWidth();
        const UInt32  Height = NewTexture->GetHeight();
        const UInt16  DepthOrArraySize = std::max(NewTexture->GetDepth(), NewTexture->GetArrayCount());
        const UInt32  MipLevels   = NewTexture->GetMipLevels();
        const UInt32  SampleCount = NewTexture->GetSampleCount();

        D3D12_RESOURCE_DESC Desc;
        Memory::Memzero(&Desc);

        Desc.Dimension        = GetD3D12TextureResourceDimension<TD3D12Texture>();
        Desc.Flags            = ConvertTextureUsage(Usage);
        Desc.Format           = ConvertFormat(Format);
        Desc.Width            = Width;
        Desc.Height           = Height;
        Desc.DepthOrArraySize = DepthOrArraySize;
        Desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Desc.MipLevels        = static_cast<UINT16>(MipLevels);
        Desc.SampleDesc.Count = SampleCount;

        if (SampleCount > 1)
        {
            const Int32 Quality = Device->GetMultisampleQuality(Desc.Format, SampleCount);
            Desc.SampleDesc.Quality = std::max<Int32>(Quality - 1, 0);
        }
        else
        {
            Desc.SampleDesc.Quality = 0;
        }

        D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
        if (Usage & TextureUsage_Dynamic)
        {
            HeapType = D3D12_HEAP_TYPE_UPLOAD;
        }

        D3D12_CLEAR_VALUE DxOptimizedClearValue;
        Memory::Memzero(&DxOptimizedClearValue);

        D3D12_CLEAR_VALUE* DxOptimizedClearValuePtr = nullptr;
        if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
            (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
        {
            Bool IsTypeless = false;
            switch (Desc.Format)
            {
                case DXGI_FORMAT_R32G32B32A32_TYPELESS:
                case DXGI_FORMAT_R32G32B32_TYPELESS:
                case DXGI_FORMAT_R16G16B16A16_TYPELESS:
                case DXGI_FORMAT_R32G32_TYPELESS:
                case DXGI_FORMAT_R32G8X24_TYPELESS:
                case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
                case DXGI_FORMAT_R10G10B10A2_TYPELESS:
                case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                case DXGI_FORMAT_R16G16_TYPELESS:
                case DXGI_FORMAT_R32_TYPELESS:
                case DXGI_FORMAT_R24G8_TYPELESS:
                case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
                case DXGI_FORMAT_R8G8_TYPELESS:
                case DXGI_FORMAT_R16_TYPELESS:
                case DXGI_FORMAT_R8_TYPELESS:
                {
                    IsTypeless = true;
                    break;
                }

                default:
                {
                    IsTypeless = false;
                    break;
                }
            };

            if (!IsTypeless)
            {
                DxOptimizedClearValuePtr        = &DxOptimizedClearValue;
                DxOptimizedClearValue.Format    = Desc.Format;

                const ClearValue& OptimizedClearValue = NewTexture->GetOptimizedClearValue();
                if (OptimizedClearValue.Type == EClearValueType::ClearValueType_Color)
                {
                    Memory::Memcpy(
                        DxOptimizedClearValue.Color, 
                        OptimizedClearValue.Color.RGBA, 
                        sizeof(DxOptimizedClearValue.Color));
                }
                else
                {
                    DxOptimizedClearValue.DepthStencil.Depth   = OptimizedClearValue.DepthStencil.Depth;
                    DxOptimizedClearValue.DepthStencil.Stencil = OptimizedClearValue.DepthStencil.Stencil;
                }
            }
        }

        if (!AllocateTexture(
            *NewTexture, 
            HeapType, 
            D3D12_RESOURCE_STATE_COMMON, 
            DxOptimizedClearValuePtr,
            Desc))
        {
            LOG_ERROR("[D3D12RenderLayer]: Failed to allocate texture");
            return nullptr;
        }

        if (InitalData)
        {
            UploadTexture(*NewTexture, InitalData);
        }

        return NewTexture;
    }

    /*
    * Create resource view helpers
    */

    FORCEINLINE D3D12ShaderResourceView* CreateShaderResourceView(
        const D3D12Resource* Resource, 
        const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc) const
    {
        if (!Resource)
        {
            LOG_ERROR("[D3D12RenderLayer]: Specified resource was invalid for creation of a ShaderResourceView");
            return nullptr;
        }
        else
        {
            return DBG_NEW D3D12ShaderResourceView(Device, Resource, Desc);
        }
    }

    FORCEINLINE D3D12RenderTargetView* CreateRenderTargetView(
        const D3D12Resource* Resource,
        const D3D12_RENDER_TARGET_VIEW_DESC& Desc) const
    {
        if (!Resource)
        {
            LOG_ERROR("[D3D12RenderLayer]: Specified resource was invalid for creation of a RenderTargetView");
            return nullptr;
        }
        else
        {
            return DBG_NEW D3D12RenderTargetView(Device, Resource, Desc);
        }
    }

    FORCEINLINE D3D12DepthStencilView* CreateDepthStencilView(
        const D3D12Resource* Resource,
        const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc) const
    {
        if (!Resource)
        {
            LOG_ERROR("[D3D12RenderLayer]: Specified resource was invalid for creation of a DepthStencilView");
            return nullptr;
        }
        else
        {
            return DBG_NEW D3D12DepthStencilView(Device, Resource, Desc);
        }
    }

    FORCEINLINE D3D12UnorderedAccessView* CreateUnorderedAccessView(
        const D3D12Resource* CounterResource,
        const D3D12Resource* Resource,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc) const
    {
        if (!Resource)
        {
            LOG_ERROR("[D3D12RenderLayer]: Specified resource was invalid for creation of a UnorderedAccessView");
            return nullptr;
        }
        else
        {
            return DBG_NEW D3D12UnorderedAccessView(Device, CounterResource, Resource, Desc);
        }
    }

private:
    D3D12Device* Device;
    TSharedRef<D3D12CommandContext> DirectCmdContext;
    D3D12DefaultRootSignatures      DefaultRootSignatures;
};