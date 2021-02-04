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

D3D12RenderLayer* gD3D12RenderLayer = nullptr;

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

template<typename TD3D12Texture>
inline Bool IsTextureCube()
{
    return false;
}

template<>
inline Bool IsTextureCube<D3D12TextureCube>()
{
    return true;
}

template<>
inline Bool IsTextureCube<D3D12TextureCubeArray>()
{
    return true;
}

D3D12RenderLayer::D3D12RenderLayer()
    : GenericRenderLayer(ERenderLayerApi::D3D12)
    , Device(nullptr)
    , DirectCmdContext(nullptr)
{
    gD3D12RenderLayer = this;
}

D3D12RenderLayer::~D3D12RenderLayer()
{
    SAFEDELETE(Device);

    SAFERELEASE(ResourceOfflineDescriptorHeap);
    SAFERELEASE(RenderTargetOfflineDescriptorHeap);
    SAFERELEASE(DepthStencilOfflineDescriptorHeap);
    SAFERELEASE(SamplerOfflineDescriptorHeap);

    gD3D12RenderLayer = nullptr;
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

template<typename TD3D12Texture>
TD3D12Texture* D3D12RenderLayer::CreateTexture(
    EFormat Format,
    UInt32 SizeX, UInt32 SizeY, UInt32 SizeZ,
    UInt32 NumMips,
    UInt32 NumSamples, 
    UInt32 Flags,
    EResourceState InitialState,
    const ResourceData* InitialData,
    const ClearValue& OptimalClearValue)
{
    TSharedRef<TD3D12Texture> NewTexture = DBG_NEW TD3D12Texture(Device, Format, SizeX, SizeY, SizeZ, NumMips, NumSamples, Flags, OptimalClearValue);

    D3D12_RESOURCE_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.Dimension        = GetD3D12TextureResourceDimension<TD3D12Texture>();
    Desc.Flags            = ConvertTextureFlags(Flags);
    Desc.Format           = ConvertFormat(Format); 
    Desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.Width            = SizeX;
    Desc.Height           = SizeY;
    Desc.DepthOrArraySize = (UINT16)SizeZ;
    Desc.MipLevels        = (UINT16)NumMips;
    Desc.Alignment        = 0;
    Desc.SampleDesc.Count = NumSamples;

    if (NumSamples > 1)
    {
        const Int32 Quality = Device->GetMultisampleQuality(Desc.Format, NumSamples);
        Desc.SampleDesc.Quality = Quality - 1;
    }
    else
    {
        Desc.SampleDesc.Quality = 0;
    }

    D3D12_CLEAR_VALUE* ClearValuePtr = nullptr;
    D3D12_CLEAR_VALUE  ClearValue;
    if (Flags & TextureFlag_RTV || Flags & TextureFlag_DSV)
    {
        ClearValue.Format = OptimalClearValue.GetFormat() != EFormat::Unknown ? ConvertFormat(OptimalClearValue.GetFormat()) : Desc.Format;
        if (OptimalClearValue.GetType() == ClearValue::EType::DepthStencil)
        {
            ClearValue.DepthStencil.Depth   = OptimalClearValue.AsDepthStencil().Depth;
            ClearValue.DepthStencil.Stencil = OptimalClearValue.AsDepthStencil().Stencil;
            ClearValuePtr = &ClearValue;
        }
        else if (OptimalClearValue.GetType() == ClearValue::EType::Color)
        {
            Memory::Memcpy(ClearValue.Color, OptimalClearValue.AsColor().Elements);
            ClearValuePtr = &ClearValue;
        }
    }

    D3D12Resource DxResource = D3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
    if (!DxResource.Init(D3D12_RESOURCE_STATE_COMMON, ClearValuePtr))
    {
        return nullptr;
    }

    NewTexture->SetResource(DxResource);

    if (Flags & TextureFlag_SRV && !(Flags & TextureFlag_NoDefaultSRV))
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc;
        Memory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format                  = CastShaderResourceFormat(Desc.Format);
        ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        {
            if (SizeZ > 6 && IsTextureCube<TD3D12Texture>())
            {
                ViewDesc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                ViewDesc.TextureCubeArray.MipLevels           = NumMips;
                ViewDesc.TextureCubeArray.MostDetailedMip     = 0;
                ViewDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
                ViewDesc.TextureCubeArray.First2DArrayFace    = 0;
                ViewDesc.TextureCubeArray.NumCubes            = SizeZ / TEXTURE_CUBE_FACE_COUNT;
            }
            else if(IsTextureCube<TD3D12Texture>())
            {
                ViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
                ViewDesc.TextureCube.MipLevels           = NumMips;
                ViewDesc.TextureCube.MostDetailedMip     = 0;
                ViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
            }
            else if (SizeZ > 1)
            {
                ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                ViewDesc.Texture2DArray.MipLevels           = NumMips;
                ViewDesc.Texture2DArray.MostDetailedMip     = 0;
                ViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                ViewDesc.Texture2DArray.PlaneSlice          = 0;
                ViewDesc.Texture2DArray.ArraySize           = SizeZ;
                ViewDesc.Texture2DArray.FirstArraySlice     = 0;
            }
            else
            {
                ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                ViewDesc.Texture2D.MipLevels           = NumMips;
                ViewDesc.Texture2D.MostDetailedMip     = 0;
                ViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                ViewDesc.Texture2D.PlaneSlice          = 0;
            }
        }
        else if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture3D.MipLevels           = NumMips;
            ViewDesc.Texture3D.MostDetailedMip     = 0;
            ViewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            VALIDATE(false);
            return nullptr;
        }

        TSharedRef<D3D12ShaderResourceView> SRV = DBG_NEW D3D12ShaderResourceView(Device, ResourceOfflineDescriptorHeap);
        if (!SRV->Init())
        {
            return nullptr;
        }

        if (!SRV->CreateView(NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture->SetShaderResourceView(SRV.Get());
    }

    // TODO: Fix for other resources that Texture2D?
    const Bool IsTexture2D = (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) && (SizeZ == 1);
    if (Flags & TextureFlag_RTV && !(Flags & TextureFlag_NoDefaultRTV) && IsTexture2D)
    {
        D3D12Texture2D* NewTexture2D = static_cast<D3D12Texture2D*>(NewTexture->AsTexture2D());

        D3D12_RENDER_TARGET_VIEW_DESC ViewDesc;
        Memory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format               = Desc.Format;
        ViewDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice   = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<D3D12RenderTargetView> RTV = DBG_NEW D3D12RenderTargetView(Device, RenderTargetOfflineDescriptorHeap);
        if (!RTV->Init())
        {
            return nullptr;
        }

        if (!RTV->CreateView(NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetRenderTargetView(RTV.Get());
    }

    if (Flags & TextureFlag_DSV && !(Flags & TextureFlag_NoDefaultDSV) && IsTexture2D)
    {
        D3D12Texture2D* NewTexture2D = static_cast<D3D12Texture2D*>(NewTexture->AsTexture2D());

        D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc;
        Memory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format             = Desc.Format;
        ViewDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice = 0;

        TSharedRef<D3D12DepthStencilView> DSV = DBG_NEW D3D12DepthStencilView(Device, DepthStencilOfflineDescriptorHeap);
        if (!DSV->Init())
        {
            return nullptr;
        }

        if (!DSV->CreateView(NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetDepthStencilView(DSV.Get());
    }

    if (Flags & TextureFlag_UAV && !(Flags & TextureFlag_NoDefaultUAV) && IsTexture2D)
    {
        D3D12Texture2D* NewTexture2D = static_cast<D3D12Texture2D*>(NewTexture->AsTexture2D());

        D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
        Memory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format               = Desc.Format;
        ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice   = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<D3D12UnorderedAccessView> UAV = DBG_NEW D3D12UnorderedAccessView(Device, ResourceOfflineDescriptorHeap);
        if (!UAV->Init())
        {
            return nullptr;
        }

        if (!UAV->CreateView(nullptr, NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetUnorderedAccessView(UAV.Get());
    }

    if (InitialData)
    {
        // TODO: Support other types than texture 2D

        Texture2D* Texture2D = NewTexture->AsTexture2D();
        if (!Texture2D)
        {
            return nullptr;
        }

        DirectCmdContext->Begin();

        DirectCmdContext->TransitionTexture(Texture2D, EResourceState::Common, EResourceState::CopyDest);
        DirectCmdContext->UpdateTexture2D(Texture2D, SizeX, SizeY, 0, InitialData->GetData());

        // NOTE: Transition into InitialState
        DirectCmdContext->TransitionTexture(Texture2D, EResourceState::CopyDest, InitialState);

        DirectCmdContext->End();
    }
    else
    {
        if (InitialState != EResourceState::Common)
        {
            DirectCmdContext->Begin();
            DirectCmdContext->TransitionTexture(NewTexture.Get(), EResourceState::Common, InitialState);
            DirectCmdContext->End();
        }
    }

    return NewTexture.ReleaseOwnership();
}

Texture2D* D3D12RenderLayer::CreateTexture2D(
    EFormat Format, 
    UInt32 Width, 
    UInt32 Height, 
    UInt32 NumMips, 
    UInt32 NumSamples, 
    UInt32 Flags, 
    EResourceState InitialState, 
    const ResourceData* InitialData, 
    const ClearValue& OptimalClearValue)
{
    return CreateTexture<D3D12Texture2D>(Format, Width, Height, 1, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue);
}

Texture2DArray* D3D12RenderLayer::CreateTexture2DArray(
    EFormat Format, 
    UInt32 Width, 
    UInt32 Height, 
    UInt32 NumMips, 
    UInt32 NumSamples, 
    UInt32 NumArraySlices, 
    UInt32 Flags, 
    EResourceState InitialState, 
    const ResourceData* InitialData, 
    const ClearValue& OptimalClearValue)
{
    return CreateTexture<D3D12Texture2DArray>(Format, Width, Height, NumArraySlices, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue);
}

TextureCube* D3D12RenderLayer::CreateTextureCube(
    EFormat Format, 
    UInt32 Size, 
    UInt32 NumMips, 
    UInt32 Flags, 
    EResourceState InitialState, 
    const ResourceData* InitialData, 
    const ClearValue& OptimalClearValue)
{
    return CreateTexture<D3D12TextureCube>(Format, Size, Size, TEXTURE_CUBE_FACE_COUNT, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

TextureCubeArray* D3D12RenderLayer::CreateTextureCubeArray(
    EFormat Format, 
    UInt32 Size, 
    UInt32 NumMips, 
    UInt32 NumArraySlices, 
    UInt32 Flags,
    EResourceState InitialState, 
    const ResourceData* InitialData, 
    const ClearValue& OptimalClearValue)
{
    const UInt32 ArraySlices = NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
    return CreateTexture<D3D12TextureCubeArray>(Format, Size, Size, ArraySlices, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

Texture3D* D3D12RenderLayer::CreateTexture3D(
    EFormat Format, 
    UInt32 Width, 
    UInt32 Height, 
    UInt32 Depth, 
    UInt32 NumMips, 
    UInt32 Flags, 
    EResourceState InitialState, 
    const ResourceData* InitialData, 
    const ClearValue& OptimalClearValue)
{
    return CreateTexture<D3D12Texture3D>(Format, Width, Height, Depth, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

SamplerState* D3D12RenderLayer::CreateSamplerState(const SamplerStateCreateInfo& CreateInfo)
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

    Memory::Memcpy(Desc.BorderColor, CreateInfo.BorderColor.Elements, sizeof(Desc.BorderColor));

    TSharedRef<D3D12SamplerState> Sampler = DBG_NEW D3D12SamplerState(Device, SamplerOfflineDescriptorHeap);
    if (!Sampler->Init(Desc))
    {
        return nullptr;
    }
    else
    {
        return Sampler.ReleaseOwnership();
    }
}

template<typename TD3D12Buffer>
Bool D3D12RenderLayer::FinalizeBufferResource(TD3D12Buffer* Buffer, UInt32 SizeInBytes, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
{
    D3D12_RESOURCE_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = ConvertBufferFlags(Flags);
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = SizeInBytes;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    D3D12_HEAP_TYPE       DxHeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES DxInitialState = D3D12_RESOURCE_STATE_COMMON;
    if (Flags & BufferFlag_Upload)
    {
        DxHeapType     = D3D12_HEAP_TYPE_UPLOAD;
        DxInitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    D3D12Resource DxResource = D3D12Resource(Device, Desc, DxHeapType);
    if (!DxResource.Init(DxInitialState, nullptr))
    {
        return false;
    }
    else
    {
        Buffer->SetResource(DxResource);
    }

    if (InitialData)
    {
        if (Buffer->IsUpload())
        {
            VALIDATE(DxResource.GetDesc().Width <= SizeInBytes);

            Void* HostData = Buffer->Map(0, 0);
            if (!HostData)
            {
                return false;
            }

            Memory::Memcpy(HostData, InitialData->GetData(), InitialData->GetSizeInBytes());
            Buffer->Unmap(0, 0);
        }
        else
        {
            DirectCmdContext->Begin();

            DirectCmdContext->TransitionBuffer(Buffer, EResourceState::Common, EResourceState::CopyDest);
            DirectCmdContext->UpdateBuffer(Buffer, 0, InitialData->GetSizeInBytes(), InitialData->GetData());
            
            // NOTE: Transfer to the initialstate
            DirectCmdContext->TransitionBuffer(Buffer, EResourceState::CopyDest, InitialState);

            DirectCmdContext->End();
        }
    }
    else
    {
        if (InitialState != EResourceState::Common && !Buffer->IsUpload())
        {
            DirectCmdContext->Begin();
            DirectCmdContext->TransitionBuffer(Buffer, EResourceState::Common, InitialState);
            DirectCmdContext->End();
        }
    }

    return true;
}

VertexBuffer* D3D12RenderLayer::CreateVertexBuffer(UInt32 Stride, UInt32 NumVertices, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
{
    const UInt32 SizeInBytes = NumVertices * Stride;

    TSharedRef<D3D12VertexBuffer> NewBuffer = DBG_NEW D3D12VertexBuffer(Device, NumVertices, Stride, Flags);
    if (!FinalizeBufferResource(NewBuffer.Get(), SizeInBytes, Flags, InitialState, InitialData))
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create VertexBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

IndexBuffer* D3D12RenderLayer::CreateIndexBuffer(EIndexFormat Format, UInt32 NumIndices, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
{
    const UInt32 SizeInBytes = NumIndices * GetStrideFromIndexFormat(Format);

    TSharedRef<D3D12IndexBuffer> NewBuffer = DBG_NEW D3D12IndexBuffer(Device, Format, NumIndices, Flags);
    if (!FinalizeBufferResource(NewBuffer.Get(), SizeInBytes, Flags, InitialState, InitialData))
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create IndexBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

ConstantBuffer* D3D12RenderLayer::CreateConstantBuffer(UInt32 SizeInBytes, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
{
    VALIDATE(!(Flags & BufferFlag_UAV) && !(Flags & BufferFlag_SRV));

    const UInt32 AlignedSizeInBytes = Math::AlignUp<UInt32>(SizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    TSharedRef<D3D12ConstantBuffer> NewBuffer = DBG_NEW D3D12ConstantBuffer(Device, ResourceOfflineDescriptorHeap, SizeInBytes, Flags);
    if (!FinalizeBufferResource(NewBuffer.Get(), AlignedSizeInBytes, Flags, InitialState, InitialData))
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create ConstantBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

StructuredBuffer* D3D12RenderLayer::CreateStructuredBuffer(UInt32 Stride, UInt32 NumElements, UInt32 Flags, EResourceState InitialState, const ResourceData* InitialData)
{
    const UInt32 SizeInBytes = NumElements * Stride;

    TSharedRef<D3D12StructuredBuffer> NewBuffer = DBG_NEW D3D12StructuredBuffer(Device, NumElements, Stride, Flags);
    if (!FinalizeBufferResource(NewBuffer.Get(), SizeInBytes, Flags, InitialState, InitialData))
    {
        LOG_ERROR("[D3D12RenderLayer]: Failed to create StructuredBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

/*
* RayTracing
*/

RayTracingGeometry* D3D12RenderLayer::CreateRayTracingGeometry()
{
    return nullptr;
}

RayTracingScene* D3D12RenderLayer::CreateRayTracingScene()
{
    return nullptr;
}

ShaderResourceView* D3D12RenderLayer::CreateShaderResourceView(const ShaderResourceViewCreateInfo& CreateInfo)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    Memory::Memzero(&Desc);

    // TODO: Expose in ShaderResourceViewCreateInfo
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    D3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == ShaderResourceViewCreateInfo::EType::Texture2D)
    {
        Texture2D*        Texture   = CreateInfo.Texture2D.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsSRV() && CreateInfo.Texture2D.Format != EFormat::Unknown);

        Desc.Format = ConvertFormat(CreateInfo.Texture2D.Format);
        if (!Texture->IsMultiSampled())
        {
            Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipLevels           = CreateInfo.Texture2D.NumMips;
            Desc.Texture2D.MostDetailedMip     = CreateInfo.Texture2D.Mip;
            Desc.Texture2D.ResourceMinLODClamp = CreateInfo.Texture2D.MinMipBias;
            Desc.Texture2D.PlaneSlice          = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (CreateInfo.Type == ShaderResourceViewCreateInfo::EType::Texture2DArray)
    {
        Texture2DArray*   Texture   = CreateInfo.Texture2DArray.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsSRV() && CreateInfo.Texture2DArray.Format != EFormat::Unknown);

        Desc.Format = ConvertFormat(CreateInfo.Texture2DArray.Format);
        if (!Texture->IsMultiSampled())
        {
            Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipLevels           = CreateInfo.Texture2DArray.NumMips;
            Desc.Texture2DArray.MostDetailedMip     = CreateInfo.Texture2DArray.Mip;
            Desc.Texture2DArray.ResourceMinLODClamp = CreateInfo.Texture2DArray.MinMipBias;
            Desc.Texture2DArray.ArraySize           = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice     = CreateInfo.Texture2DArray.ArraySlice;
            Desc.Texture2DArray.PlaneSlice          = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        }
    }
    else if (CreateInfo.Type == ShaderResourceViewCreateInfo::EType::TextureCube)
    {
        TextureCube*      Texture   = CreateInfo.TextureCube.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsSRV() && CreateInfo.TextureCube.Format != EFormat::Unknown);

        Desc.Format                          = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MipLevels           = CreateInfo.TextureCube.NumMips;
        Desc.TextureCube.MostDetailedMip     = CreateInfo.TextureCube.Mip;
        Desc.TextureCube.ResourceMinLODClamp = CreateInfo.TextureCube.MinMipBias;
    }
    else if (CreateInfo.Type == ShaderResourceViewCreateInfo::EType::TextureCubeArray)
    {
        TextureCubeArray* Texture   = CreateInfo.TextureCubeArray.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsSRV() && CreateInfo.TextureCubeArray.Format != EFormat::Unknown);

        Desc.Format                               = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MipLevels           = CreateInfo.TextureCubeArray.NumMips;
        Desc.TextureCubeArray.MostDetailedMip     = CreateInfo.TextureCubeArray.Mip;
        Desc.TextureCubeArray.ResourceMinLODClamp = CreateInfo.TextureCubeArray.MinMipBias;
        // ArraySlice * 6 to get the first Texture2D face
        Desc.TextureCubeArray.First2DArrayFace = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        Desc.TextureCubeArray.NumCubes         = CreateInfo.TextureCubeArray.NumArraySlices;
    }
    else if (CreateInfo.Type == ShaderResourceViewCreateInfo::EType::Texture3D)
    {
        Texture3D*        Texture   = CreateInfo.Texture3D.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsSRV() && CreateInfo.Texture3D.Format != EFormat::Unknown);

        Desc.Format                        = ConvertFormat(CreateInfo.Texture3D.Format);
        Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipLevels           = CreateInfo.Texture3D.NumMips;
        Desc.Texture3D.MostDetailedMip     = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.ResourceMinLODClamp = CreateInfo.Texture3D.MinMipBias;
    }
    else if (CreateInfo.Type == ShaderResourceViewCreateInfo::EType::VertexBuffer)
    {
        VertexBuffer*    Buffer   = CreateInfo.VertexBuffer.Buffer;
        D3D12BaseBuffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        VALIDATE(Buffer->IsSRV());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements         = CreateInfo.VertexBuffer.NumVertices;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if (CreateInfo.Type == ShaderResourceViewCreateInfo::EType::IndexBuffer)
    {
        IndexBuffer*     Buffer   = CreateInfo.IndexBuffer.Buffer;
        D3D12BaseBuffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        VALIDATE(Buffer->IsSRV());

        Desc.ViewDimension       = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements  = CreateInfo.IndexBuffer.NumIndices;

        // TODO: What if the index type is 16-bit?
        VALIDATE(Buffer->GetFormat() != EIndexFormat::UInt16);
        
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (CreateInfo.Type == ShaderResourceViewCreateInfo::EType::StructuredBuffer)
    {
        StructuredBuffer* Buffer   = CreateInfo.StructuredBuffer.Buffer;
        D3D12BaseBuffer*  DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        VALIDATE(Buffer->IsSRV());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = CreateInfo.StructuredBuffer.NumElements;
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

UnorderedAccessView* D3D12RenderLayer::CreateUnorderedAccessView(const UnorderedAccessViewCreateInfo& CreateInfo)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    Memory::Memzero(&Desc);

    D3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == UnorderedAccessViewCreateInfo::EType::Texture2D)
    {
        Texture2D*        Texture   = CreateInfo.Texture2D.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsUAV() && CreateInfo.Texture2D.Format != EFormat::Unknown);

        Desc.Format               = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        Desc.Texture2D.MipSlice   = CreateInfo.Texture2D.Mip;
        Desc.Texture2D.PlaneSlice = 0;
    }
    else if (CreateInfo.Type == UnorderedAccessViewCreateInfo::EType::Texture2DArray)
    {
        Texture2DArray*   Texture   = CreateInfo.Texture2DArray.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsUAV() && CreateInfo.Texture2DArray.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.Texture2DArray.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.Texture2DArray.Mip;
        Desc.Texture2DArray.ArraySize       = CreateInfo.Texture2DArray.NumArraySlices;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == UnorderedAccessViewCreateInfo::EType::TextureCube)
    {
        TextureCube*      Texture   = CreateInfo.TextureCube.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsUAV() && CreateInfo.TextureCube.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.TextureCube.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = 0;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == UnorderedAccessViewCreateInfo::EType::TextureCubeArray)
    {
        TextureCubeArray* Texture   = CreateInfo.TextureCubeArray.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsUAV() && CreateInfo.TextureCubeArray.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.TextureCubeArray.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = CreateInfo.TextureCubeArray.NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == UnorderedAccessViewCreateInfo::EType::Texture3D)
    {
        Texture3D*        Texture   = CreateInfo.Texture3D.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsUAV() && CreateInfo.Texture3D.Format != EFormat::Unknown);

        Desc.Format                = ConvertFormat(CreateInfo.Texture3D.Format);
        Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = CreateInfo.Texture3D.DepthSlice;
        Desc.Texture3D.WSize       = CreateInfo.Texture3D.NumDepthSlices;
    }
    else if (CreateInfo.Type == UnorderedAccessViewCreateInfo::EType::VertexBuffer)
    {
        VertexBuffer*    Buffer   = CreateInfo.VertexBuffer.Buffer;
        D3D12BaseBuffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        VALIDATE(Buffer->IsUAV());

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements         = CreateInfo.VertexBuffer.NumVertices;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if (CreateInfo.Type == UnorderedAccessViewCreateInfo::EType::IndexBuffer)
    {
        IndexBuffer*     Buffer   = CreateInfo.IndexBuffer.Buffer;
        D3D12BaseBuffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        VALIDATE(Buffer->IsUAV());

        Desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements  = CreateInfo.IndexBuffer.NumIndices;

        // TODO: What if the index type is 16-bit?
        VALIDATE(Buffer->GetFormat() != EIndexFormat::UInt16);

        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (CreateInfo.Type == UnorderedAccessViewCreateInfo::EType::StructuredBuffer)
    {
        StructuredBuffer* Buffer   = CreateInfo.StructuredBuffer.Buffer;
        D3D12BaseBuffer*  DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        VALIDATE(Buffer->IsUAV());

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    TSharedRef<D3D12UnorderedAccessView> DxView = DBG_NEW D3D12UnorderedAccessView(Device, ResourceOfflineDescriptorHeap);
    if (!DxView->Init())
    {
        return nullptr;
    }

    VALIDATE(Resource != nullptr);
    
    // TODO: Expose counterresource
    if (DxView->CreateView(nullptr, Resource, Desc))
    {
        return DxView.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

RenderTargetView* D3D12RenderLayer::CreateRenderTargetView(const RenderTargetViewCreateInfo& CreateInfo)
{
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
    Memory::Memzero(&Desc);

    D3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat(CreateInfo.Format);
    VALIDATE(CreateInfo.Format != EFormat::Unknown);

    if (CreateInfo.Type == RenderTargetViewCreateInfo::EType::Texture2D)
    {
        Texture2D*        Texture   = CreateInfo.Texture2D.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsRTV());

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice   = CreateInfo.Texture2D.Mip;
            Desc.Texture2D.PlaneSlice = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (CreateInfo.Type == RenderTargetViewCreateInfo::EType::Texture2DArray)
    {
        Texture2DArray*   Texture   = CreateInfo.Texture2DArray.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsRTV());

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = CreateInfo.Texture2DArray.Mip;
            Desc.Texture2DArray.ArraySize       = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
            Desc.Texture2DArray.PlaneSlice      = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        }
    }
    else if (CreateInfo.Type == RenderTargetViewCreateInfo::EType::TextureCube)
    {
        TextureCube*      Texture   = CreateInfo.TextureCube.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsRTV());

        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == RenderTargetViewCreateInfo::EType::TextureCubeArray)
    {
        TextureCubeArray* Texture   = CreateInfo.TextureCubeArray.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsRTV());

        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == RenderTargetViewCreateInfo::EType::Texture3D)
    {
        Texture3D*        Texture   = CreateInfo.Texture3D.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsRTV());

        Desc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = CreateInfo.Texture3D.DepthSlice;
        Desc.Texture3D.WSize       = CreateInfo.Texture3D.NumDepthSlices;
    }

    TSharedRef<D3D12RenderTargetView> DxView = DBG_NEW D3D12RenderTargetView(Device, RenderTargetOfflineDescriptorHeap);
    if (!DxView->Init())
    {
        return nullptr;
    }

    VALIDATE(Resource != nullptr);

    if (!DxView->CreateView(Resource, Desc))
    {
        return nullptr;
    }
    else
    {
        return DxView.ReleaseOwnership();
    }
}

DepthStencilView* D3D12RenderLayer::CreateDepthStencilView(const DepthStencilViewCreateInfo& CreateInfo)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
    Memory::Memzero(&Desc);

    D3D12Resource* Resource = nullptr;
    
    Desc.Format = ConvertFormat(CreateInfo.Format);
    VALIDATE(CreateInfo.Format != EFormat::Unknown);
    
    if (CreateInfo.Type == DepthStencilViewCreateInfo::EType::Texture2D)
    {
        Texture2D*        Texture   = CreateInfo.Texture2D.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsDSV());

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension        = D3D12_DSV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice   = CreateInfo.Texture2D.Mip;
        }
        else
        {
            Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (CreateInfo.Type == DepthStencilViewCreateInfo::EType::Texture2DArray)
    {
        Texture2DArray*   Texture   = CreateInfo.Texture2DArray.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsDSV());

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = CreateInfo.Texture2DArray.Mip;
            Desc.Texture2DArray.ArraySize       = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = CreateInfo.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        }
    }
    else if (CreateInfo.Type == DepthStencilViewCreateInfo::EType::TextureCube)
    {
        TextureCube*      Texture   = CreateInfo.TextureCube.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsDSV());

        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
    }
    else if (CreateInfo.Type == DepthStencilViewCreateInfo::EType::TextureCubeArray)
    {
        TextureCubeArray* Texture   = CreateInfo.TextureCubeArray.Texture;
        D3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        VALIDATE(Texture->IsDSV());

        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
    }

    TSharedRef<D3D12DepthStencilView> DxView = DBG_NEW D3D12DepthStencilView(Device, DepthStencilOfflineDescriptorHeap);
    if (!DxView->Init())
    {
        return nullptr;
    }

    if (!DxView->CreateView(Resource, Desc))
    {
        return nullptr;
    }
    else
    {
        return DxView.ReleaseOwnership();
    }
}

ComputeShader* D3D12RenderLayer::CreateComputeShader(const TArray<UInt8>& ShaderCode)
{
    D3D12ComputeShader* Shader = DBG_NEW D3D12ComputeShader(Device, ShaderCode);
    Shader->CreateRootSignature();
    return Shader;
}

VertexShader* D3D12RenderLayer::CreateVertexShader(const TArray<UInt8>& ShaderCode)
{
    return DBG_NEW D3D12VertexShader(Device, ShaderCode);
}

HullShader* D3D12RenderLayer::CreateHullShader(const TArray<UInt8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

DomainShader* D3D12RenderLayer::CreateDomainShader(const TArray<UInt8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

GeometryShader* D3D12RenderLayer::CreateGeometryShader(const TArray<UInt8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

MeshShader* D3D12RenderLayer::CreateMeshShader(const TArray<UInt8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

AmplificationShader* D3D12RenderLayer::CreateAmplificationShader(const TArray<UInt8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

PixelShader* D3D12RenderLayer::CreatePixelShader(const TArray<UInt8>& ShaderCode)
{
    return DBG_NEW D3D12PixelShader(Device, ShaderCode);
}

RayGenShader* D3D12RenderLayer::CreateRayGenShader(const TArray<UInt8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

RayHitShader* D3D12RenderLayer::CreateRayHitShader(const TArray<UInt8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

RayMissShader* D3D12RenderLayer::CreateRayMissShader(const TArray<UInt8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

DepthStencilState* D3D12RenderLayer::CreateDepthStencilState(const DepthStencilStateCreateInfo& CreateInfo)
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

RasterizerState* D3D12RenderLayer::CreateRasterizerState(const RasterizerStateCreateInfo& CreateInfo)
{
    D3D12_RASTERIZER_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.AntialiasedLineEnable = CreateInfo.AntialiasedLineEnable;
    Desc.ConservativeRaster    = (CreateInfo.EnableConservativeRaster) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
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

BlendState* D3D12RenderLayer::CreateBlendState(const BlendStateCreateInfo& CreateInfo)
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

InputLayoutState* D3D12RenderLayer::CreateInputLayout(const InputLayoutStateCreateInfo& CreateInfo)
{
    return DBG_NEW D3D12InputLayoutState(Device, CreateInfo);
}

GraphicsPipelineState* D3D12RenderLayer::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo)
{
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
        InputLayoutDesc.pInputElementDescs = nullptr;
        InputLayoutDesc.NumElements        = 0;
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
    SamplerDesc.Count   = CreateInfo.SampleCount;
    SamplerDesc.Quality = CreateInfo.SampleQuality;

    // Create PipelineState
    D3D12_PIPELINE_STATE_STREAM_DESC PipelineStreamDesc;
    Memory::Memzero(&PipelineStreamDesc, sizeof(D3D12_PIPELINE_STATE_STREAM_DESC));

    PipelineStreamDesc.pPipelineStateSubobjectStream = &PipelineStream;
    PipelineStreamDesc.SizeInBytes                   = sizeof(GraphicsPipelineStream);

    TComPtr<ID3D12PipelineState> NewPipelineState;
    HRESULT hResult = Device->CreatePipelineState(&PipelineStreamDesc, IID_PPV_ARGS(&NewPipelineState));
    if (SUCCEEDED(hResult))
    {
        D3D12GraphicsPipelineState* Pipeline = DBG_NEW D3D12GraphicsPipelineState(Device);
        Pipeline->PipelineState = NewPipelineState;

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

ComputePipelineState* D3D12RenderLayer::CreateComputePipelineState(const ComputePipelineStateCreateInfo& Info)
{
    VALIDATE(Info.Shader != nullptr);
    
    // Check if shader contains a rootsignature, or use the default one
    TSharedRef<D3D12ComputeShader> Shader        = MakeSharedRef<D3D12ComputeShader>(Info.Shader);
    TSharedRef<D3D12RootSignature> RootSignature = MakeSharedRef<D3D12RootSignature>(Shader->GetRootSignature());
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

RayTracingPipelineState* D3D12RenderLayer::CreateRayTracingPipelineState()
{
    return nullptr;
}

Viewport* D3D12RenderLayer::CreateViewport(GenericWindow* Window, UInt32 Width, UInt32 Height, EFormat ColorFormat, EFormat DepthFormat)
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

    TSharedRef<D3D12Viewport> Viewport = DBG_NEW D3D12Viewport(Device, DirectCmdContext.Get(), WinWindow->GetHandle(), ColorFormat, Width, Height);
    if (Viewport->Init())
    {
        return Viewport.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

Bool D3D12RenderLayer::IsRayTracingSupported()
{
    return Device->IsRayTracingSupported();
}

Bool D3D12RenderLayer::UAVSupportsFormat(EFormat Format)
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