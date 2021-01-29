#include "Containers/TUniquePtr.h"

#include "Windows/WindowsWindow.h"

#include "D3D12RenderLayer.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12PipelineState.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12RayTracingPipelineState.h"
#include "D3D12RayTracingScene.h"
#include "D3D12RootSignature.h"
#include "D3D12Views.h"
#include "D3D12Helpers.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Shader.h"
#include "D3D12SamplerState.h"
#include "D3D12Viewport.h"

#include <algorithm>

constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;

D3D12RenderLayer::D3D12RenderLayer()
    : GenericRenderLayer(ERenderLayerApi::RenderLayerApi_D3D12)
    , Device(nullptr)
    , DirectCmdContext(nullptr)
{
}

D3D12RenderLayer::~D3D12RenderLayer()
{
    SAFEDELETE(Device);
}

Bool D3D12RenderLayer::Init(Bool EnableDebug)
{
    Device = DBG_NEW D3D12Device(EnableDebug, EnableDebug ? true : false);
    if (!Device->Init())
    {
        return false;
    }

    if (!DefaultRootSignatures.CreateRootSignatures(Device))
    {
        return false;
    }

    DirectCmdContext = DBG_NEW D3D12CommandContext(Device, DefaultRootSignatures);
    if (!DirectCmdContext->Init())
    {
        return false;
    }

    ResourceOfflineDescriptorHeap = DBG_NEW D3D12OfflineDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!ResourceOfflineDescriptorHeap->Init())
    {
        return false;
    }

    RenderTargetOfflineDescriptorHeap = DBG_NEW D3D12OfflineDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if (!RenderTargetOfflineDescriptorHeap->Init())
    {
        return false;
    }

    DepthStencilOfflineDescriptorHeap = DBG_NEW D3D12OfflineDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    if (!DepthStencilOfflineDescriptorHeap->Init())
    {
        return false;
    }

    SamplerOfflineDescriptorHeap = DBG_NEW D3D12OfflineDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!SamplerOfflineDescriptorHeap->Init())
    {
        return false;
    }

    return true;
}

Texture1D* D3D12RenderLayer::CreateTexture1D(
    const ResourceData* InitalData, 
    EFormat Format, 
    UInt32 Usage, 
    UInt32 Width, 
    UInt32 MipLevels, 
    const ClearValue& OptimizedClearValue) const
{
    return CreateTextureResource<D3D12Texture1D>(
        InitalData,
        Format,
        Usage,
        Width,
        MipLevels,
        OptimizedClearValue);
}

Texture1DArray* D3D12RenderLayer::CreateTexture1DArray(
    const ResourceData* InitalData, 
    EFormat Format, 
    UInt32 Usage, 
    UInt32 Width, 
    UInt32 MipLevels, 
    UInt16 ArrayCount, 
    const ClearValue& OptimizedClearValue) const
{
    return CreateTextureResource<D3D12Texture1DArray>(
        InitalData,
        Format, 
        Usage, 
        Width, 
        MipLevels, 
        ArrayCount, 
        OptimizedClearValue);
}

Texture2D* D3D12RenderLayer::CreateTexture2D(
    const ResourceData* InitalData, 
    EFormat Format, 
    UInt32 Usage, 
    UInt32 Width, 
    UInt32 Height, 
    UInt32 MipLevels, 
    UInt32 SampleCount, 
    const ClearValue& OptimizedClearValue) const
{
    return CreateTextureResource<D3D12Texture2D>(
        InitalData,
        Format, 
        Usage, 
        Width, 
        Height, 
        MipLevels, 
        SampleCount, 
        OptimizedClearValue);
}

Texture2DArray* D3D12RenderLayer::CreateTexture2DArray(
    const ResourceData* InitalData, 
    EFormat Format, 
    UInt32 Usage, 
    UInt32 Width, 
    UInt32 Height, 
    UInt32 MipLevels, 
    UInt16 ArrayCount,
    UInt32 SampleCount, 
    const ClearValue& OptimizedClearValue) const
{
    return CreateTextureResource<D3D12Texture2DArray>(
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

TextureCube* D3D12RenderLayer::CreateTextureCube(
    const ResourceData* InitalData, 
    EFormat Format, 
    UInt32 Usage, 
    UInt32 Size, 
    UInt32 MipLevels, 
    UInt32 SampleCount, 
    const ClearValue& OptimizedClearValue) const
{
    return CreateTextureResource<D3D12TextureCube>(
        InitalData,
        Format,
        Usage,
        Size,
        MipLevels,
        SampleCount,
        OptimizedClearValue);
}

TextureCubeArray* D3D12RenderLayer::CreateTextureCubeArray(
    const ResourceData* InitalData, 
    EFormat Format, 
    UInt32 Usage, 
    UInt32 Size, 
    UInt32 MipLevels, 
    UInt16 ArrayCount,
    UInt32 SampleCount, 
    const ClearValue& OptimizedClearValue) const
{
    return CreateTextureResource<D3D12TextureCubeArray>(
        InitalData,
        Format,
        Usage,
        Size,
        MipLevels,
        ArrayCount,
        SampleCount,
        OptimizedClearValue);
}

Texture3D* D3D12RenderLayer::CreateTexture3D(
    const ResourceData* InitalData, 
    EFormat Format, 
    UInt32 Usage, 
    UInt32 Width, 
    UInt32 Height, 
    UInt16 Depth,
    UInt32 MipLevels, 
    const ClearValue& OptimizedClearValue) const
{
    return CreateTextureResource<D3D12Texture3D>(
        InitalData,
        Format,
        Usage,
        Width,
        Height,
        Depth,
        MipLevels,
        OptimizedClearValue);
}

SamplerState* D3D12RenderLayer::CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo) const
{
    D3D12_SAMPLER_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.AddressU       = ConvertSamplerMode(CreateInfo.AddressU);
    Desc.AddressV       = ConvertSamplerMode(CreateInfo.AddressV);
    Desc.AddressW       = ConvertSamplerMode(CreateInfo.AddressW);
    Desc.ComparisonFunc = ConvertComparisonFunc(CreateInfo.ComparisonFunc);
    Desc.Filter         = ConvertSamplerFilter(CreateInfo.Filter);
    Desc.MaxAnisotropy  = CreateInfo.MaxAnisotropy;
    Desc.MaxLOD         = CreateInfo.MaxLOD;
    Desc.MinLOD         = CreateInfo.MinLOD;
    Desc.MipLODBias     = CreateInfo.MipLODBias;
    Memory::Memcpy(Desc.BorderColor, CreateInfo.BorderColor, sizeof(Desc.BorderColor));

    TSharedRef<D3D12SamplerState> Sampler = DBG_NEW D3D12SamplerState(Device, SamplerOfflineDescriptorHeap);
    if (!Sampler->Init())
    {
        return nullptr;
    }

    if (!Sampler->CreateView(Desc))
    {
        return nullptr;
    }
    else
    {
        return Sampler.ReleaseOwnership();
    }
}

VertexBuffer* D3D12RenderLayer::CreateVertexBuffer(
    const ResourceData* InitalData, 
    UInt32 SizeInBytes, 
    UInt32 StrideInBytes, 
    UInt32 Usage) const
{
    D3D12VertexBuffer* NewBuffer = CreateBufferResource<D3D12VertexBuffer>(
        InitalData,
        EResourceState::ResourceState_Common,
        SizeInBytes, 
        StrideInBytes, 
        Usage);
    if (!NewBuffer)
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create VertexBuffer");
        return nullptr;
    }

    D3D12_VERTEX_BUFFER_VIEW View;
    Memory::Memzero(&View, sizeof(D3D12_VERTEX_BUFFER_VIEW));

    View.BufferLocation    = NewBuffer->GetGPUVirtualAddress();
    View.SizeInBytes    = SizeInBytes;
    View.StrideInBytes    = StrideInBytes;

    NewBuffer->VertexBufferView = View;
    return NewBuffer;
}

IndexBuffer* D3D12RenderLayer::CreateIndexBuffer(
    const ResourceData* InitalData, 
    UInt32 SizeInBytes, 
    EIndexFormat IndexFormat, 
    UInt32 Usage) const
{
    D3D12IndexBuffer* NewBuffer = CreateBufferResource<D3D12IndexBuffer>(
        InitalData, 
        EResourceState::ResourceState_Common, 
        SizeInBytes, 
        IndexFormat, 
        Usage);
    if (!NewBuffer)
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create IndexBuffer");
        return nullptr;
    }

    D3D12_INDEX_BUFFER_VIEW View;
    Memory::Memzero(&View, sizeof(D3D12_INDEX_BUFFER_VIEW));

    View.BufferLocation = NewBuffer->GetGPUVirtualAddress();
    View.SizeInBytes    = SizeInBytes;
    if (IndexFormat == EIndexFormat::IndexFormat_UInt16)
    {
        View.Format    = DXGI_FORMAT_R16_UINT;
    }
    else if (IndexFormat == EIndexFormat::IndexFormat_UInt32)
    {
        View.Format = DXGI_FORMAT_R32_UINT;
    }

    NewBuffer->IndexBufferView = View;
    return NewBuffer;
}

ConstantBuffer* D3D12RenderLayer::CreateConstantBuffer(
    const ResourceData* InitalData, 
    UInt32 SizeInBytes, 
    UInt32 Usage,
    EResourceState InitialState) const
{
    TSharedRef<D3D12ConstantBuffer> NewBuffer = CreateBufferResource<D3D12ConstantBuffer>(
        InitalData,
        InitialState,
        ResourceOfflineDescriptorHeap,
        SizeInBytes, 
        Usage);
    if (!NewBuffer)
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create ConstantBuffer");
        return nullptr;
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
    Memory::Memzero(&ViewDesc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));

    ViewDesc.BufferLocation = NewBuffer->GetGPUVirtualAddress();
    ViewDesc.SizeInBytes    = UInt32(NewBuffer->GetAllocatedSize());

    if (!NewBuffer->View.Init())
    {
        return nullptr;
    }

    if (!NewBuffer->View.CreateView(NewBuffer.Get(), ViewDesc))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

StructuredBuffer* D3D12RenderLayer::CreateStructuredBuffer(
    const ResourceData* InitalData, 
    UInt32 SizeInBytes, 
    UInt32 Stride, 
    UInt32 Usage) const
{
    D3D12StructuredBuffer* NewBuffer = CreateBufferResource<D3D12StructuredBuffer>(
        InitalData, 
        EResourceState::ResourceState_Common,
        SizeInBytes, 
        Stride, 
        Usage);
    if (!NewBuffer)
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create StructuredBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer;
    }
}

/*
* RayTracing
*/

RayTracingGeometry* D3D12RenderLayer::CreateRayTracingGeometry() const
{
    return nullptr;
}

RayTracingScene* D3D12RenderLayer::CreateRayTracingScene() const
{
    return nullptr;
}

ShaderResourceView* D3D12RenderLayer::CreateShaderResourceView(const ShaderResourceViewCreateInfo& CreateInfo) const
{
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    Memory::Memzero(&Desc);

    // TODO: Expose in ShaderResourceViewCreateInfo
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    D3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == EShaderResourceViewType::ShaderResourceViewType_Texture)
    {
        const TextureShaderResourceViewCreateInfo* TexCreateInfo = CreateInfo.AsTextureShaderResourceView();
        
        Texture* Texture = TexCreateInfo->Texture;
        Resource = D3D12TextureCast(Texture);

        VALIDATE(Texture->HasShaderResourceUsage());
        VALIDATE(TexCreateInfo->Format != EFormat::Format_Unknown);

        Desc.Format = ConvertFormat(TexCreateInfo->Format);
        if (Texture->AsTexture1D())
        {
            VALIDATE(TexCreateInfo->ArraySlice = 0 && TexCreateInfo->NumArraySlices = 1);

            Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE1D;
            Desc.Texture1D.MipLevels           = TexCreateInfo->NumMipLevels;
            Desc.Texture1D.MostDetailedMip     = TexCreateInfo->MipLevel;
            Desc.Texture1D.ResourceMinLODClamp = TexCreateInfo->MinMipBias;
        }
        else if (Texture->AsTexture1DArray())
        {
            Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            Desc.Texture1DArray.MipLevels           = TexCreateInfo->NumMipLevels;
            Desc.Texture1DArray.MostDetailedMip     = TexCreateInfo->MipLevel;
            Desc.Texture1DArray.ResourceMinLODClamp = TexCreateInfo->MinMipBias;
            Desc.Texture1DArray.ArraySize           = TexCreateInfo->NumArraySlices;
            Desc.Texture1DArray.FirstArraySlice     = TexCreateInfo->ArraySlice;
        }
        else if (Texture->AsTexture2D())
        {
            VALIDATE(TexCreateInfo->ArraySlice = 0 && TexCreateInfo->NumArraySlices = 1);

            if (!Texture->IsMultiSampled())
            {
                Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                Desc.Texture2D.MipLevels           = TexCreateInfo->NumMipLevels;
                Desc.Texture2D.MostDetailedMip     = TexCreateInfo->MipLevel;
                Desc.Texture2D.ResourceMinLODClamp = TexCreateInfo->MinMipBias;
                Desc.Texture2D.PlaneSlice          = 0;
            }
            else
            {
                Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
        }
        else if (Texture->AsTexture2DArray())
        {
            if (!Texture->IsMultiSampled())
            {
                Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipLevels           = TexCreateInfo->NumMipLevels;
                Desc.Texture2DArray.MostDetailedMip     = TexCreateInfo->MipLevel;
                Desc.Texture2DArray.ResourceMinLODClamp = TexCreateInfo->MinMipBias;
                Desc.Texture2DArray.ArraySize           = TexCreateInfo->NumArraySlices;
                Desc.Texture2DArray.FirstArraySlice     = TexCreateInfo->ArraySlice;
                Desc.Texture2DArray.PlaneSlice          = 0;
            }
            else
            {
                Desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                Desc.Texture2DMSArray.ArraySize       = TexCreateInfo->NumArraySlices;
                Desc.Texture2DMSArray.FirstArraySlice = TexCreateInfo->ArraySlice;
            }
        }
        else if (Texture->AsTexture3D())
        {
            VALIDATE(TexCreateInfo->ArraySlice = 0 && TexCreateInfo->NumArraySlices = 1);

            Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
            Desc.Texture3D.MipLevels           = TexCreateInfo->NumMipLevels;
            Desc.Texture3D.MostDetailedMip     = TexCreateInfo->MipLevel;
            Desc.Texture3D.ResourceMinLODClamp = TexCreateInfo->MinMipBias;
        }
        else if (Texture->AsTextureCube())
        {
            VALIDATE(TexCreateInfo->ArraySlice = 0 && TexCreateInfo->NumArraySlices = 1);

            Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
            Desc.TextureCube.MipLevels           = TexCreateInfo->NumMipLevels;
            Desc.TextureCube.MostDetailedMip     = TexCreateInfo->MipLevel;
            Desc.TextureCube.ResourceMinLODClamp = TexCreateInfo->MinMipBias;
        }
        else if (Texture->AsTextureCubeArray())
        {
            Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            Desc.TextureCubeArray.MipLevels           = TexCreateInfo->NumMipLevels;
            Desc.TextureCubeArray.MostDetailedMip     = TexCreateInfo->MipLevel;
            Desc.TextureCubeArray.ResourceMinLODClamp = TexCreateInfo->MinMipBias;
            // ArraySlice * 6 to get the first Texture2D face
            Desc.TextureCubeArray.First2DArrayFace = TexCreateInfo->ArraySlice * TEXTURE_CUBE_FACE_COUNT;
            Desc.TextureCubeArray.NumCubes         = TexCreateInfo->NumArraySlices;
        }
    }
    else if (CreateInfo.Type == EShaderResourceViewType::ShaderResourceViewType_VertexBuffer)
    {
        const VertexBufferShaderResourceViewCreateInfo* BufferCreateInfo = CreateInfo.AsVertexBufferShaderResourceView();
        
        VertexBuffer* Buffer = BufferCreateInfo->Buffer;
        Resource = D3D12BufferCast(Buffer);

        VALIDATE(Buffer->HasShaderResourceUsage());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = BufferCreateInfo->FirstElement;
        Desc.Buffer.NumElements         = BufferCreateInfo->NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if (CreateInfo.Type == EShaderResourceViewType::ShaderResourceViewType_IndexBuffer)
    {
        const IndexBufferShaderResourceViewCreateInfo* BufferCreateInfo = CreateInfo.AsIndexBufferShaderResourceView();
        
        IndexBuffer* Buffer = BufferCreateInfo->Buffer;
        Resource = D3D12BufferCast(Buffer);

        VALIDATE(Buffer->HasShaderResourceUsage());

        Desc.ViewDimension       = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = BufferCreateInfo->FirstElement;
        Desc.Buffer.NumElements  = BufferCreateInfo->NumElements;

        // TODO: What if the index type is 16-bit?
        VALIDATE(Buffer->GetIndexFormat() != EIndexFormat::IndexFormat_UInt16);
        
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (CreateInfo.Type == EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer)
    {
        const StructuredBufferShaderResourceViewCreateInfo* BufferCreateInfo = CreateInfo.AsStructuredBufferShaderResourceView();
        
        StructuredBuffer* Buffer = BufferCreateInfo->Buffer;
        Resource = D3D12BufferCast(Buffer);

        VALIDATE(Buffer->HasShaderResourceUsage());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = BufferCreateInfo->FirstElement;
        Desc.Buffer.NumElements         = BufferCreateInfo->NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    VALIDATE(Resource != nullptr);

    TSharedRef<D3D12ShaderResourceView> DxView = DBG_NEW D3D12ShaderResourceView(Device, ResourceOfflineDescriptorHeap);
    if (!DxView->Init())
    {
        return nullptr;
    }

    if (DxView->CreateView(Resource, Desc))
    {
        return DxView.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

UnorderedAccessView* D3D12RenderLayer::CreateUnorderedAccessView(const UnorderedAccessViewCreateInfo& CreateInfo) const
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    Memory::Memzero(&Desc);

    D3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
    {
        const TextureUnorderedAccessViewCreateInfo* TexCreateInfo = CreateInfo.AsTextureUnorderedAccessView();
        
        Texture* Texture = TexCreateInfo->Texture;
        Resource = D3D12TextureCast(Texture);

        VALIDATE(Texture->IsMultiSampled() == false && Texture->HasUnorderedAccessUsage());
        VALIDATE(TexCreateInfo->Format != EFormat::Format_Unknown);

        Desc.Format = ConvertFormat(TexCreateInfo->Format);
        if (Texture->AsTexture1D())
        {
            VALIDATE(TexCreateInfo->ArrayOrDepthSlice = 0 && TexCreateInfo->ArrayOrDepthSlice = 1);

            Desc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
            Desc.Texture1D.MipSlice = TexCreateInfo->MipLevel;
        }
        else if (Texture->AsTexture1DArray())
        {
            Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            Desc.Texture1DArray.MipSlice        = TexCreateInfo->MipLevel;
            Desc.Texture1DArray.ArraySize       = TexCreateInfo->ArrayOrDepthSlice;
            Desc.Texture1DArray.FirstArraySlice = TexCreateInfo->ArrayOrDepthSlice;
        }
        else if (Texture->AsTexture2D())
        {
            VALIDATE(TexCreateInfo->ArraySlice = 0 && TexCreateInfo->NumArraySlices = 1);

            Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice   = TexCreateInfo->MipLevel;
            Desc.Texture2D.PlaneSlice = 0;
        }
        else if (Texture->AsTexture2DArray())
        {
            Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = TexCreateInfo->MipLevel;
            Desc.Texture2DArray.ArraySize       = TexCreateInfo->ArrayOrDepthSlice;
            Desc.Texture2DArray.FirstArraySlice = TexCreateInfo->ArrayOrDepthSlice;
            Desc.Texture2DArray.PlaneSlice      = 0;
        }
        else if (Texture->AsTexture3D())
        {
            VALIDATE(TexCreateInfo->ArraySlice = 0 && TexCreateInfo->NumArraySlices = 1);

            Desc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE3D;
            Desc.Texture3D.MipSlice     = TexCreateInfo->MipLevel;
            Desc.Texture3D.FirstWSlice  = TexCreateInfo->ArrayOrDepthSlice;
            Desc.Texture3D.WSize        = TexCreateInfo->NumArrayOrDepthSlices;
        }
        else if (Texture->AsTextureCube())
        {
            VALIDATE(TexCreateInfo->ArraySlice = 0 && TexCreateInfo->NumArraySlices = 1);

            Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = TexCreateInfo->MipLevel;
            Desc.Texture2DArray.ArraySize       = TEXTURE_CUBE_FACE_COUNT;
            Desc.Texture2DArray.FirstArraySlice = 0;
            Desc.Texture2DArray.PlaneSlice      = 0;
        }
        else if (Texture->AsTextureCubeArray())
        {
            Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = TexCreateInfo->MipLevel;
            Desc.Texture2DArray.ArraySize       = TEXTURE_CUBE_FACE_COUNT;
            Desc.Texture2DArray.FirstArraySlice = TexCreateInfo->ArrayOrDepthSlice * TEXTURE_CUBE_FACE_COUNT;
            Desc.Texture2DArray.PlaneSlice      = 0;
        }
    }
    else if (CreateInfo.Type == EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer)
    {
        const VertexBufferUnorderedAccessViewCreateInfo* BufferCreateInfo = CreateInfo.AsVertexBufferUnorderedAccessView();
        
        VertexBuffer* Buffer = BufferCreateInfo->Buffer;
        Resource = D3D12BufferCast(Buffer);

        VALIDATE(Buffer->HasUnorderedAccessUsage());

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = BufferCreateInfo->FirstElement;
        Desc.Buffer.NumElements         = BufferCreateInfo->NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();

        // TODO: Expose counter resource in UnorderedAccessViewCreateInfo
        Desc.Buffer.CounterOffsetInBytes = 0;
    }
    else if (CreateInfo.Type == EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer)
    {
        const IndexBufferUnorderedAccessViewCreateInfo* BufferCreateInfo = CreateInfo.AsIndexBufferUnorderedAccessView();
        
        IndexBuffer* Buffer = BufferCreateInfo->Buffer;
        Resource = D3D12BufferCast(Buffer);

        VALIDATE(Buffer->HasUnorderedAccessUsage());

        Desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = BufferCreateInfo->FirstElement;
        Desc.Buffer.NumElements  = BufferCreateInfo->NumElements;

        // TODO: What if the index type is 16-bit?
        VALIDATE(Buffer->GetIndexFormat() != EIndexFormat::IndexFormat_UInt16);

        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;

        // TODO: Expose counter resource in UnorderedAccessViewCreateInfo
        Desc.Buffer.CounterOffsetInBytes = 0;
    }
    else if (CreateInfo.Type == EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer)
    {
        const StructuredBufferUnorderedAccessViewCreateInfo* BufferCreateInfo = CreateInfo.AsStructuredBufferUnorderedAccessView();
        
        StructuredBuffer* Buffer = BufferCreateInfo->Buffer;
        Resource = D3D12BufferCast(Buffer);

        VALIDATE(Buffer->HasUnorderedAccessUsage());

        Desc.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement         = BufferCreateInfo->FirstElement;
        Desc.Buffer.NumElements          = BufferCreateInfo->NumElements;
        Desc.Format                      = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags                = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride  = Buffer->GetStride();
        
        // TODO: Expose counter resource in UnorderedAccessViewCreateInfo
        Desc.Buffer.CounterOffsetInBytes = 0;
    }

    VALIDATE(Resource != nullptr);

    TSharedRef<D3D12UnorderedAccessView> DxView = DBG_NEW D3D12UnorderedAccessView(Device, ResourceOfflineDescriptorHeap);
    if (!DxView->Init())
    {
        return nullptr;
    }

    // TODO: Expose counterresource
    if (DxView->CreateView(Resource, nullptr, Desc))
    {
        return DxView.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

RenderTargetView* D3D12RenderLayer::CreateRenderTargetView(const RenderTargetViewCreateInfo& CreateInfo) const
{
    Texture* Texture = CreateInfo.Texture;
    D3D12Resource* DxResource = D3D12TextureCast(Texture);

    VALIDATE(Texture->HasRenderTargetUsage());
    VALIDATE(CreateInfo.Format != EFormat::Format_Unknown);

    D3D12_RENDER_TARGET_VIEW_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.Format = ConvertFormat(CreateInfo.Format);
    if (Texture->AsTexture1D())
    {
        VALIDATE(TexCreateInfo->ArrayOrDepthSlice = 0 && TexCreateInfo->ArrayOrDepthSlice = 1);

        Desc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
        Desc.Texture1D.MipSlice = CreateInfo.MipLevel;
    }
    else if (Texture->AsTexture1DArray())
    {
        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        Desc.Texture1DArray.MipSlice        = CreateInfo.MipLevel;
        Desc.Texture1DArray.ArraySize       = CreateInfo.NumArrayOrDepthSlices;
        Desc.Texture1DArray.FirstArraySlice = CreateInfo.ArrayOrDepthSlice;
    }
    else if (Texture->AsTexture2D())
    {
        VALIDATE(CreateInfo.ArrayOrDepthSlice = 0 && CreateInfo.NumArrayOrDepthSlices = 1);

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice     = CreateInfo.MipLevel;
            Desc.Texture2D.PlaneSlice   = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (Texture->AsTexture2DArray())
    {
        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = CreateInfo.MipLevel;
            Desc.Texture2DArray.ArraySize       = CreateInfo.NumArrayOrDepthSlices;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.ArrayOrDepthSlice;
            Desc.Texture2DArray.PlaneSlice      = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = CreateInfo.NumArrayOrDepthSlices;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.ArrayOrDepthSlice;
        }
    }
    else if (Texture->AsTexture3D())
    {
        VALIDATE(CreateInfo.ArrayOrDepthSlice = 0 && CreateInfo.NumArrayOrDepthSlices = 1);

        Desc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice     = CreateInfo.MipLevel;
        Desc.Texture3D.FirstWSlice  = CreateInfo.ArrayOrDepthSlice;
        Desc.Texture3D.WSize        = CreateInfo.NumArrayOrDepthSlices;
    }
    else if (Texture->AsTextureCube())
    {
        VALIDATE(CreateInfo.ArrayOrDepthSlice = 0 && CreateInfo.NumArrayOrDepthSlices = 1);

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = CreateInfo.MipLevel;
            Desc.Texture2DArray.ArraySize       = 1;
            Desc.Texture2DArray.FirstArraySlice = TEXTURE_CUBE_FACE_COUNT + CreateInfo.FaceIndex;
            Desc.Texture2DArray.PlaneSlice      = 0;
        }
        else
        {
            Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = 1;
            Desc.Texture2DMSArray.FirstArraySlice = TEXTURE_CUBE_FACE_COUNT + CreateInfo.FaceIndex;
        }
    }
    else if (Texture->AsTextureCubeArray())
    {
        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = CreateInfo.MipLevel;
            Desc.Texture2DArray.ArraySize       = TEXTURE_CUBE_FACE_COUNT;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.ArrayOrDepthSlice * TEXTURE_CUBE_FACE_COUNT;
            Desc.Texture2DArray.PlaneSlice      = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = 1;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.ArrayOrDepthSlice * TEXTURE_CUBE_FACE_COUNT + CreateInfo.FaceIndex;
        }
    }

    TSharedRef<D3D12RenderTargetView> DxView = DBG_NEW D3D12RenderTargetView(Device, RenderTargetOfflineDescriptorHeap);
    if (!DxView->Init())
    {
        return nullptr;
    }

    if (!DxView->CreateView(DxResource, Desc))
    {
        return nullptr;
    }
    else
    {
        return DxView.ReleaseOwnership();
    }
}

DepthStencilView* D3D12RenderLayer::CreateDepthStencilView(const DepthStencilViewCreateInfo& CreateInfo) const
{
    Texture* Texture = CreateInfo.Texture;
    D3D12Resource* DxResource = D3D12TextureCast(Texture);

    VALIDATE(Texture->HasDepthStencilUsage());
    VALIDATE(CreateInfo.Format != EFormat::Format_Unknown);

    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.Format = ConvertFormat(CreateInfo.Format);
    if (Texture->AsTexture1D())
    {
        VALIDATE(TexCreateInfo->ArrayOrDepthSlice = 0 && TexCreateInfo->ArrayOrDepthSlice = 1);

        Desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE1D;
        Desc.Texture1D.MipSlice = CreateInfo.MipLevel;
    }
    else if (Texture->AsTexture1DArray())
    {
        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        Desc.Texture1DArray.MipSlice        = CreateInfo.MipLevel;
        Desc.Texture1DArray.ArraySize       = CreateInfo.NumArraySlices;
        Desc.Texture1DArray.FirstArraySlice = CreateInfo.ArraySlice;
    }
    else if (Texture->AsTexture2D())
    {
        VALIDATE(CreateInfo.ArraySlice = 0 && CreateInfo.NumArraySlices = 1);

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice = CreateInfo.MipLevel;
        }
        else
        {
            Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (Texture->AsTexture2DArray())
    {
        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = CreateInfo.MipLevel;
            Desc.Texture2DArray.ArraySize       = CreateInfo.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.ArraySlice;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = CreateInfo.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.ArraySlice;
        }
    }
    else if (Texture->AsTexture3D())
    {
        VALIDATE(false);
        return nullptr;
    }
    else if (Texture->AsTextureCube())
    {
        VALIDATE(CreateInfo.ArraySlice = 0 && CreateInfo.NumArraySlices = 1);

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = CreateInfo.MipLevel;
            Desc.Texture2DArray.ArraySize       = 1;
            Desc.Texture2DArray.FirstArraySlice = TEXTURE_CUBE_FACE_COUNT + CreateInfo.FaceIndex;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = 1;
            Desc.Texture2DMSArray.FirstArraySlice = TEXTURE_CUBE_FACE_COUNT + CreateInfo.FaceIndex;
        }
    }
    else if (Texture->AsTextureCubeArray())
    {
        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = CreateInfo.MipLevel;
            Desc.Texture2DArray.ArraySize       = TEXTURE_CUBE_FACE_COUNT;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = 1;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.ArraySlice * TEXTURE_CUBE_FACE_COUNT + CreateInfo.FaceIndex;
        }
    }

    TSharedRef<D3D12DepthStencilView> DxView = DBG_NEW D3D12RenderTargetView(Device, DepthStencilOfflineDescriptorHeap);
    if (!DxView->Init())
    {
        return nullptr;
    }

    if (!DxView->CreateView(DxResource, Desc))
    {
        return nullptr;
    }
    else
    {
        return DxView.ReleaseOwnership();
    }
}

ComputeShader* D3D12RenderLayer::CreateComputeShader(const TArray<UInt8>& ShaderCode) const
{
    D3D12ComputeShader* Shader = DBG_NEW D3D12ComputeShader(Device, ShaderCode);
    Shader->CreateRootSignature();
    return Shader;
}

VertexShader* D3D12RenderLayer::CreateVertexShader(const TArray<UInt8>& ShaderCode) const
{
    return DBG_NEW D3D12VertexShader(Device, ShaderCode);
}

HullShader* D3D12RenderLayer::CreateHullShader(const TArray<UInt8>& ShaderCode) const
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

DomainShader* D3D12RenderLayer::CreateDomainShader(const TArray<UInt8>& ShaderCode) const
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

GeometryShader* D3D12RenderLayer::CreateGeometryShader(const TArray<UInt8>& ShaderCode) const
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

MeshShader* D3D12RenderLayer::CreateMeshShader(const TArray<UInt8>& ShaderCode) const
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

AmplificationShader* D3D12RenderLayer::CreateAmplificationShader(const TArray<UInt8>& ShaderCode) const
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

PixelShader* D3D12RenderLayer::CreatePixelShader(const TArray<UInt8>& ShaderCode) const
{
    return DBG_NEW D3D12PixelShader(Device, ShaderCode);
}

RayGenShader* D3D12RenderLayer::CreateRayGenShader(const TArray<UInt8>& ShaderCode) const
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

RayHitShader* D3D12RenderLayer::CreateRayHitShader(const TArray<UInt8>& ShaderCode) const
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

RayMissShader* D3D12RenderLayer::CreateRayMissShader(const TArray<UInt8>& ShaderCode) const
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

DepthStencilState* D3D12RenderLayer::CreateDepthStencilState(
    const DepthStencilStateCreateInfo& CreateInfo) const
{
    D3D12_DEPTH_STENCIL_DESC Desc;
    Memory::Memzero(&Desc);
    
    Desc.DepthEnable      = CreateInfo.DepthEnable;
    Desc.DepthFunc        = ConvertComparisonFunc(CreateInfo.DepthFunc);
    Desc.DepthWriteMask   = ConvertDepthWriteMask(CreateInfo.DepthWriteMask);
    Desc.StencilEnable    = CreateInfo.StencilEnable;
    Desc.StencilReadMask  = CreateInfo.StencilReadMask;
    Desc.StencilWriteMask = CreateInfo.StencilWriteMask;
    Desc.FrontFace        = ConvertDepthStencilOp(CreateInfo.FrontFace);
    Desc.BackFace         = ConvertDepthStencilOp(CreateInfo.BackFace);

    return DBG_NEW D3D12DepthStencilState(Device, Desc);
}

RasterizerState* D3D12RenderLayer::CreateRasterizerState(
    const RasterizerStateCreateInfo& CreateInfo) const
{
    D3D12_RASTERIZER_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.AntialiasedLineEnable = CreateInfo.AntialiasedLineEnable;
    Desc.ConservativeRaster =
        (CreateInfo.EnableConservativeRaster) ?
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON :
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    Desc.CullMode              = ConvertCullMode(CreateInfo.CullMode);
    Desc.DepthBias             = CreateInfo.DepthBias;
    Desc.DepthBiasClamp        = CreateInfo.DepthBiasClamp;
    Desc.DepthClipEnable       = CreateInfo.DepthClipEnable;
    Desc.SlopeScaledDepthBias  = CreateInfo.SlopeScaledDepthBias;
    Desc.FillMode              = ConvertFillMode(CreateInfo.FillMode);
    Desc.ForcedSampleCount     = CreateInfo.ForcedSampleCount;
    Desc.FrontCounterClockwise = CreateInfo.FrontCounterClockwise;
    Desc.MultisampleEnable     = CreateInfo.MultisampleEnable;

    return DBG_NEW D3D12RasterizerState(Device, Desc);
}

BlendState* D3D12RenderLayer::CreateBlendState(
    const BlendStateCreateInfo& CreateInfo) const
{
    D3D12_BLEND_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.AlphaToCoverageEnable  = CreateInfo.AlphaToCoverageEnable;
    Desc.IndependentBlendEnable = CreateInfo.IndependentBlendEnable;    
    for (UInt32 i = 0; i < 8; i++)
    {
        Desc.RenderTarget[i].BlendEnable           = CreateInfo.RenderTarget[i].BlendEnable;
        Desc.RenderTarget[i].BlendOp               = ConvertBlendOp(CreateInfo.RenderTarget[i].BlendOp);
        Desc.RenderTarget[i].BlendOpAlpha          = ConvertBlendOp(CreateInfo.RenderTarget[i].BlendOpAlpha);
        Desc.RenderTarget[i].DestBlend             = ConvertBlend(CreateInfo.RenderTarget[i].DestBlend);
        Desc.RenderTarget[i].DestBlendAlpha        = ConvertBlend(CreateInfo.RenderTarget[i].DestBlendAlpha);
        Desc.RenderTarget[i].SrcBlend              = ConvertBlend(CreateInfo.RenderTarget[i].SrcBlend);
        Desc.RenderTarget[i].SrcBlendAlpha         = ConvertBlend(CreateInfo.RenderTarget[i].SrcBlendAlpha);
        Desc.RenderTarget[i].LogicOpEnable         = CreateInfo.RenderTarget[i].LogicOpEnable;
        Desc.RenderTarget[i].LogicOp               = ConvertLogicOp(CreateInfo.RenderTarget[i].LogicOp);
        Desc.RenderTarget[i].RenderTargetWriteMask = ConvertRenderTargetWriteState(CreateInfo.RenderTarget[i].RenderTargetWriteMask);
    }

    return DBG_NEW D3D12BlendState(Device, Desc);
}

InputLayoutState* D3D12RenderLayer::CreateInputLayout(
    const InputLayoutStateCreateInfo& CreateInfo) const
{
    return DBG_NEW D3D12InputLayoutState(Device, CreateInfo);
}

GraphicsPipelineState* D3D12RenderLayer::CreateGraphicsPipelineState(
    const GraphicsPipelineStateCreateInfo& CreateInfo) const
{
    using namespace Microsoft::WRL;

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) GraphicsPipelineStream
    {
        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
            ID3D12RootSignature* RootSignature = nullptr;
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE    Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
            D3D12_INPUT_LAYOUT_DESC InputLayout = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type3 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
            D3D12_SHADER_BYTECODE VertexShader = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type4 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
            D3D12_SHADER_BYTECODE PixelShader = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type5 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
            D3D12_RT_FORMAT_ARRAY RenderTargetInfo = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type6 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
            DXGI_FORMAT DepthBufferFormat = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type7 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
            D3D12_RASTERIZER_DESC RasterizerDesc = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type8 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
            D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type9 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
            D3D12_BLEND_DESC BlendStateDesc = { };
        };

        struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
        {
            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type10 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
            DXGI_SAMPLE_DESC SampleDesc = { };
        };
    } PipelineStream;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC& InputLayoutDesc = PipelineStream.InputLayout;
    
    D3D12InputLayoutState* DxInputLayoutState = static_cast<D3D12InputLayoutState*>(CreateInfo.InputLayoutState);
    if (!DxInputLayoutState)
    {
        InputLayoutDesc.pInputElementDescs    = nullptr;
        InputLayoutDesc.NumElements            = 0;
    }
    else
    {
        InputLayoutDesc = DxInputLayoutState->GetDesc();
    }

    // VertexShader
    D3D12VertexShader* DxVertexShader = static_cast<D3D12VertexShader*>(CreateInfo.ShaderState.VertexShader);
    VALIDATE(DxVertexShader != nullptr);

    D3D12_SHADER_BYTECODE& VertexShader = PipelineStream.VertexShader;
    VertexShader = DxVertexShader->GetShaderByteCode();

    // PixelShader
    D3D12PixelShader* DxPixelShader = static_cast<D3D12PixelShader*>(CreateInfo.ShaderState.PixelShader);
    
    D3D12_SHADER_BYTECODE& PixelShader = PipelineStream.PixelShader;
    if (!DxPixelShader)
    {
        PixelShader.pShaderBytecode    = nullptr;
        PixelShader.BytecodeLength    = 0;
    }
    else
    {
        PixelShader = DxPixelShader->GetShaderByteCode();
    }

    // RenderTarget
    D3D12_RT_FORMAT_ARRAY& RenderTargetInfo = PipelineStream.RenderTargetInfo;

    const UInt32 NumRenderTargets = CreateInfo.PipelineFormats.NumRenderTargets;
    for (UInt32 Index = 0; Index < NumRenderTargets; Index++)
    {
        RenderTargetInfo.RTFormats[Index] = ConvertFormat(CreateInfo.PipelineFormats.RenderTargetFormats[Index]);
    }
    RenderTargetInfo.NumRenderTargets = NumRenderTargets;

    // DepthStencil
    PipelineStream.DepthBufferFormat = ConvertFormat(CreateInfo.PipelineFormats.DepthStencilFormat);

    // RasterizerState
    D3D12RasterizerState* DxRasterizerState = static_cast<D3D12RasterizerState*>(CreateInfo.RasterizerState);
    VALIDATE(DxRasterizerState != nullptr);

    D3D12_RASTERIZER_DESC& RasterizerDesc = PipelineStream.RasterizerDesc;
    RasterizerDesc = DxRasterizerState->GetDesc();

    // DepthStencilState
    D3D12DepthStencilState* DxDepthStencilState = static_cast<D3D12DepthStencilState*>(CreateInfo.DepthStencilState);
    VALIDATE(DxDepthStencilState != nullptr);

    D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc = PipelineStream.DepthStencilDesc;
    DepthStencilDesc = DxDepthStencilState->GetDesc();

    // BlendState
    D3D12BlendState* DxBlendState = static_cast<D3D12BlendState*>(CreateInfo.BlendState);
    VALIDATE(DxBlendState != nullptr);

    D3D12_BLEND_DESC& BlendStateDesc = PipelineStream.BlendStateDesc;
    BlendStateDesc = DxBlendState->GetDesc();

    // RootSignature
    VALIDATE(DefaultRootSignatures.Graphics != nullptr);

    D3D12RootSignature* RootSignature = DefaultRootSignatures.Graphics.Get();
    PipelineStream.RootSignature = RootSignature->GetRootSignature();

    // Topology
    PipelineStream.PrimitiveTopologyType = ConvertPrimitiveTopologyType(CreateInfo.PrimitiveTopologyType);

    // MSAA
    DXGI_SAMPLE_DESC& SamplerDesc = PipelineStream.SampleDesc;
    SamplerDesc.Count    = CreateInfo.SampleCount;
    SamplerDesc.Quality    = CreateInfo.SampleQuality;

    // Create PipelineState
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    Memory::Memzero(&PipelineStreamDesc, sizeof(D3D12_PIPELINE_STATE_STREAM_DESC));

    PipelineStreamDesc.pPipelineStateSubobjectStream    = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                        = sizeof(GraphicsPipelineStream);

    ComPtr<ID3D12PipelineState> PipelineState;
    HRESULT hResult = Device->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&PipelineState));
    if (SUCCEEDED(hResult))
    {
        D3D12GraphicsPipelineState* Pipeline = DBG_NEW D3D12GraphicsPipelineState(Device);
        Pipeline->PipelineState = PipelineState;

        // TODO: This should be refcounted
        Pipeline->RootSignature = RootSignature;

        LOG_INFO("[D3D12RenderLayer]: Created GraphicsPipelineState");
        return Pipeline;
    }
    else
    {
        LOG_ERROR("[D3D12RenderLayer]: FAILED to Create GraphicsPipelineState");
        return nullptr;
    }
}

ComputePipelineState* D3D12RenderLayer::CreateComputePipelineState(
    const ComputePipelineStateCreateInfo& Info) const
{
    VALIDATE(Info.Shader != nullptr);
    
    // Check if shader contains a rootsignature, or use the default one
    TSharedRef<D3D12ComputeShader> Shader            = MakeSharedRef<D3D12ComputeShader>(Info.Shader);
    TSharedRef<D3D12RootSignature> RootSignature    = MakeSharedRef<D3D12RootSignature>(Shader->GetRootSignature());
    if (!RootSignature)
    {
        RootSignature = DefaultRootSignatures.Compute;
    }

    D3D12ComputePipelineState* NewPipelineState = DBG_NEW D3D12ComputePipelineState(Device, Shader, RootSignature);
    if (NewPipelineState->Init())
    {
        return NewPipelineState;
    }
    else
    {
        return nullptr;
    }
}

RayTracingPipelineState* D3D12RenderLayer::CreateRayTracingPipelineState() const
{
    return nullptr;
}

/*
* Viewport
*/

Viewport* D3D12RenderLayer::CreateViewport(
    GenericWindow* Window,
    UInt32 Width,
    UInt32 Height,
    EFormat ColorFormat, 
    EFormat DepthFormat) const
{
    UNREFERENCED_VARIABLE(DepthFormat);

    // TODO: Take DepthFormat into account

    TSharedRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(Window);
    if (Width == 0)
    {
        Width = WinWindow->GetWidth();
    }

    if (Height == 0)
    {
        Height = WinWindow->GetHeight();
    }

    TSharedRef<D3D12Viewport> Viewport = DBG_NEW D3D12Viewport(
        Device, 
        DirectCmdContext.Get(), 
        WinWindow->GetHandle(), 
        Width,
        Height,
        ColorFormat);
    if (Viewport->Init())
    {
        return Viewport.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

/*
* Supported features
*/

Bool D3D12RenderLayer::IsRayTracingSupported() const
{
    return Device->IsRayTracingSupported();
}

Bool D3D12RenderLayer::UAVSupportsFormat(EFormat Format) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
    Memory::Memzero(&FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

    HRESULT Result = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
    if (SUCCEEDED(Result))
    {
        if (FeatureData.TypedUAVLoadAdditionalFormats)
        {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport =
            {
                ConvertFormat(Format),
                D3D12_FORMAT_SUPPORT1_NONE,
                D3D12_FORMAT_SUPPORT2_NONE
            };

            Result = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
            if (FAILED(Result) || (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0)
            {
                return false;
            }
        }
    }

    return true;
}

/*
* Allocate resources
*/

Bool D3D12RenderLayer::AllocateBuffer(
    D3D12Resource& Resource, 
    D3D12_HEAP_TYPE HeapType, 
    D3D12_RESOURCE_STATES InitalState, 
    D3D12_RESOURCE_FLAGS Flags, 
    UInt32 SizeInBytes) const
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    Memory::Memzero(&HeapProperties);

    HeapProperties.Type                    = HeapType;
    HeapProperties.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference    = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.Dimension                = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags                    = Flags;
    Desc.Format                    = DXGI_FORMAT_UNKNOWN;
    Desc.Width                    = SizeInBytes;
    Desc.Height                    = 1;
    Desc.DepthOrArraySize        = 1;
    Desc.Layout                    = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.MipLevels                = 1;
    Desc.SampleDesc.Count        = 1;
    Desc.SampleDesc.Quality        = 0;

    HRESULT Result = Device->CreateCommitedResource(
        &HeapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &Desc, 
        InitalState,
        nullptr, 
        IID_PPV_ARGS(&Resource.NativeResource));
    if (SUCCEEDED(Result))
    {
        Resource.Address        = Resource.NativeResource->GetGPUVirtualAddress();
        Resource.Desc            = Desc;
        Resource.HeapType        = HeapType;
        Resource.ResourceState    = InitalState;
        return true;
    }
    else
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create resource");
        return false;
    }
}

Bool D3D12RenderLayer::AllocateTexture(
    D3D12Resource& Resource, 
    D3D12_HEAP_TYPE HeapType, 
    D3D12_RESOURCE_STATES InitalState,
    D3D12_CLEAR_VALUE* OptimizedClearValue,
    const D3D12_RESOURCE_DESC& Desc) const
{
    D3D12_HEAP_PROPERTIES HeapProperties;
    Memory::Memzero(&HeapProperties);

    HeapProperties.Type                    = HeapType;
    HeapProperties.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference    = D3D12_MEMORY_POOL_UNKNOWN;

    HRESULT Result = Device->CreateCommitedResource(
        &HeapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &Desc, 
        InitalState, 
        OptimizedClearValue,
        IID_PPV_ARGS(&Resource.NativeResource));
    if (SUCCEEDED(Result))
    {
        Resource.Address    = NULL;
        Resource.Desc        = Desc;
        Resource.HeapType    = HeapType;
        Resource.ResourceState = InitalState;
        return true;
    }
    else
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create resource");
        return false;
    }
}


/*
* Resource uploading
*/

Bool D3D12RenderLayer::UploadBuffer(Buffer& Buffer, UInt32 SizeInBytes, const ResourceData* InitalData) const
{
    if (Buffer.HasDynamicUsage())
    {
        VALIDATE(Buffer.GetSizeInBytes() <= SizeInBytes);

        Void* HostData = Buffer.Map(nullptr);
        if (HostData)
        {
            Memory::Memcpy(HostData, InitalData->Data, SizeInBytes);
            Buffer.Unmap(nullptr);

            return true;
        }
    }

    DirectCmdContext->Begin();

    DirectCmdContext->TransitionBuffer(
        &Buffer,
        EResourceState::ResourceState_Common,
        EResourceState::ResourceState_CopyDest);
    
    DirectCmdContext->UpdateBuffer(&Buffer, 0, SizeInBytes, InitalData->Data);

    DirectCmdContext->TransitionBuffer(
        &Buffer,
        EResourceState::ResourceState_CopyDest,
        EResourceState::ResourceState_Common);

    DirectCmdContext->End();

    return true;
}

Bool D3D12RenderLayer::UploadTexture(Texture& Texture, const ResourceData* InitalData) const
{
    // TODO: Support other types than texture 2D
    Texture2D* Texture2D = Texture.AsTexture2D();
    if (!Texture2D)
    {
        return false;
    }

    DirectCmdContext->Begin();

    DirectCmdContext->TransitionTexture(
        Texture2D, 
        EResourceState::ResourceState_Common,
        EResourceState::ResourceState_CopyDest);

    const UInt32 Width     = Texture.GetWidth();
    const UInt32 Height = Texture.GetHeight();
    DirectCmdContext->UpdateTexture2D(
        Texture2D,
        Width,
        Height,
        0,
        InitalData->Data);

    DirectCmdContext->TransitionTexture(
        Texture2D,
        EResourceState::ResourceState_CopyDest,
        EResourceState::ResourceState_Common);

    DirectCmdContext->End();

    return true;
}