#include "CoreApplication/Windows/WindowsWindow.h"

#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12RootSignature.h"
#include "D3D12Core.h"
#include "D3D12CoreInterface.h"
#include "D3D12Views.h"
#include "D3D12RayTracing.h"
#include "D3D12PipelineState.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12SamplerState.h"
#include "D3D12Viewport.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12TimestampQuery.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12 Helpers

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITexture2D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITexture2DArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITextureCube>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITextureCubeArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12RHITexture3D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
}

template<typename D3D12TextureType>
inline bool IsTextureCube()
{
    return false;
}

template<>
inline bool IsTextureCube<CD3D12RHITextureCube>()
{
    return true;
}

template<>
inline bool IsTextureCube<CD3D12RHITextureCubeArray>()
{
    return true;
}

CD3D12CoreInterface* GD3D12Instance = nullptr;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12CoreInterface

CD3D12CoreInterface* CD3D12CoreInterface::CreateD3D12Instance()
{ 
    return dbg_new CD3D12CoreInterface(); 
}

CD3D12CoreInterface::CD3D12CoreInterface()
    : CRHICoreInterface(ERHIInstanceType::D3D12)
    , Device(nullptr)
    , DirectCmdContext(nullptr)
{
    GD3D12Instance = this;
}

CD3D12CoreInterface::~CD3D12CoreInterface()
{
    SafeDelete(DirectCmdContext);

    GenerateMipsTex2D_PSO.Reset();
    GenerateMipsTexCube_PSO.Reset();

    SafeDelete(RootSignatureCache);

    SafeRelease(ResourceOfflineDescriptorHeap);
    SafeRelease(RenderTargetOfflineDescriptorHeap);
    SafeRelease(DepthStencilOfflineDescriptorHeap);
    SafeRelease(SamplerOfflineDescriptorHeap);

    SafeDelete(Device);

    GD3D12Instance = nullptr;
}

bool CD3D12CoreInterface::Initialize(bool bEnableDebug)
{
    // NOTE: GPUBasedValidation does not work with ray tracing since it is not supported
    bool bGPUBasedValidationOn =
#if ENABLE_API_GPU_DEBUGGING
        bEnableDebug;
#else
        false;
#endif
    bool bDREDOn =
#if ENABLE_API_GPU_BREADCRUMBS
        bEnableDebug;
#else
        false;
#endif

    Device = dbg_new CD3D12Device(bEnableDebug, bGPUBasedValidationOn, bDREDOn);
    if (!Device->Initialize())
    {
        return false;
    }

    // RootSignature cache
    RootSignatureCache = dbg_new CD3D12RootSignatureCache(Device);
    if (!RootSignatureCache->Initialize())
    {
        return false;
    }

    // Init Offline Descriptor heaps
    ResourceOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!ResourceOfflineDescriptorHeap->Init())
    {
        return false;
    }

    RenderTargetOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if (!RenderTargetOfflineDescriptorHeap->Init())
    {
        return false;
    }

    DepthStencilOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    if (!DepthStencilOfflineDescriptorHeap->Init())
    {
        return false;
    }

    SamplerOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!SamplerOfflineDescriptorHeap->Init())
    {
        return false;
    }

    // Init shader compiler
    GD3D12ShaderCompiler = dbg_new CD3D12RHIShaderCompiler();
    if (!GD3D12ShaderCompiler->Init())
    {
        return false;
    }

    // Init GenerateMips Shaders and pipeline states 
    TArray<uint8> Code;
    if (!GD3D12ShaderCompiler->CompileFromFile("../Runtime/Shaders/GenerateMipsTex2D.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTex2D Shader");

        CDebug::DebugBreak();
        return false;
    }

    TSharedRef<CD3D12ComputeShader> Shader = dbg_new CD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        CDebug::DebugBreak();
        return false;
    }

    GenerateMipsTex2D_PSO = dbg_new CD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTex2D_PSO->Init())
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTex2D PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTex2D_PSO->SetName("GenerateMipsTex2D Gen PSO");
    }

    if (!GD3D12ShaderCompiler->CompileFromFile("../Runtime/Shaders/GenerateMipsTexCube.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTexCube Shader");
        CDebug::DebugBreak();
    }

    Shader = dbg_new CD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        CDebug::DebugBreak();
        return false;
    }

    GenerateMipsTexCube_PSO = dbg_new CD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTexCube_PSO->Init())
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTexCube PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTexCube_PSO->SetName("GenerateMipsTexCube Gen PSO");
    }

    // Init context
    DirectCmdContext = CD3D12CommandContext::Make(Device);
    if (!DirectCmdContext)
    {
        return false;
    }

    return true;
}

template<typename D3D12TextureType>
D3D12TextureType* CD3D12CoreInterface::CreateTexture( EFormat Format
                                                    , uint32 SizeX
                                                    , uint32 SizeY
                                                    , uint32 SizeZ
                                                    , uint32 NumMips
                                                    , uint32 NumSamples
                                                    , ETextureUsageFlags Flags
                                                    , EResourceAccess InitialState
                                                    , const SRHIResourceData* InitialData
                                                    , const CTextureClearValue& ClearValue)
{
    TSharedRef<D3D12TextureType> NewTexture = dbg_new D3D12TextureType(Device, Format, SizeX, SizeY, SizeZ, NumMips, NumSamples, Flags, ClearValue);

    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Dimension        = GetD3D12TextureResourceDimension<D3D12TextureType>();
    Desc.Flags            = ConvertTextureFlags(Flags);
    Desc.Format           = ConvertFormat(Format);
    Desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.Width            = SizeX;
    Desc.Height           = SizeY;
    Desc.DepthOrArraySize = static_cast<UINT16>(SizeZ);
    Desc.MipLevels        = static_cast<UINT16>(NumMips);
    Desc.Alignment        = 0;
    Desc.SampleDesc.Count = NumSamples;

    if (NumSamples > 1)
    {
        const int32 Quality = Device->GetMultisampleQuality(Desc.Format, NumSamples);
        Desc.SampleDesc.Quality = Quality - 1;
    }
    else
    {
        Desc.SampleDesc.Quality = 0;
    }

    D3D12_CLEAR_VALUE* D3D12ClearValuePtr = nullptr;
    D3D12_CLEAR_VALUE  D3D12ClearValue;
    if ((Flags & (ETextureUsageFlags::AllowRTV | ETextureUsageFlags::AllowDSV)) != ETextureUsageFlags::None)
    {
        D3D12ClearValue.Format = (ClearValue.Format != EFormat::Unknown) ? ConvertFormat(ClearValue.Format) : Desc.Format;
        if (ClearValue.IsDepthStencilValue())
        {
            D3D12ClearValue.DepthStencil.Depth   = ClearValue.AsDepthStencil().Depth;
            D3D12ClearValue.DepthStencil.Stencil = ClearValue.AsDepthStencil().Stencil;
            D3D12ClearValuePtr = &D3D12ClearValue;
        }
        else if (ClearValue.IsColorValue())
        {
            CMemory::Memcpy(D3D12ClearValue.Color, ClearValue.AsColor().Data(), sizeof(float[4]));
            D3D12ClearValuePtr = &D3D12ClearValue;
        }
    }

    TSharedRef<CD3D12Resource> Resource = dbg_new CD3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
    if (!Resource->Init(D3D12_RESOURCE_STATE_COMMON, D3D12ClearValuePtr))
    {
        return nullptr;
    }
    else
    {
        NewTexture->SetResource(Resource.ReleaseOwnership());
    }

    if (((Flags & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None) && ((Flags & ETextureUsageFlags::NoDefaultSRV) == ETextureUsageFlags::None))
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format                  = CastShaderResourceFormat(Desc.Format);
        ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        {
            if (SizeZ > 6 && IsTextureCube<D3D12TextureType>())
            {
                ViewDesc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                ViewDesc.TextureCubeArray.MipLevels           = NumMips;
                ViewDesc.TextureCubeArray.MostDetailedMip     = 0;
                ViewDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
                ViewDesc.TextureCubeArray.First2DArrayFace    = 0;
                ViewDesc.TextureCubeArray.NumCubes            = SizeZ / TEXTURE_CUBE_FACE_COUNT;
            }
            else if (IsTextureCube<D3D12TextureType>())
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
            D3D12_ERROR_ALWAYS("Unsupported resource dimension");
            return nullptr;
        }

        TSharedRef<CD3D12ShaderResourceView> SRV = dbg_new CD3D12ShaderResourceView(Device, ResourceOfflineDescriptorHeap);
        if (!SRV->AllocateHandle())
        {
            return nullptr;
        }

        if (!SRV->CreateView(NewTexture->GetD3D12Resource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture->SetShaderResourceView(SRV.ReleaseOwnership());
    }

    // TODO: Fix for other resources than Texture2D?
    const bool bIsTexture2D = (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) && (SizeZ == 1);
    if (((Flags & ETextureUsageFlags::AllowRTV) != ETextureUsageFlags::None) && ((Flags & ETextureUsageFlags::NoDefaultRTV) == ETextureUsageFlags::None) && bIsTexture2D)
    {
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->GetTexture2D());

        D3D12_RENDER_TARGET_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format               = Desc.Format;
        ViewDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice   = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<CD3D12RenderTargetView> RTV = dbg_new CD3D12RenderTargetView(Device, RenderTargetOfflineDescriptorHeap);
        if (!RTV->AllocateHandle())
        {
            return nullptr;
        }

        if (!RTV->CreateView(NewTexture->GetD3D12Resource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetRenderTargetView(RTV.ReleaseOwnership());
    }

    if (((Flags & ETextureUsageFlags::AllowDSV) != ETextureUsageFlags::None) && ((Flags & ETextureUsageFlags::NoDefaultDSV) == ETextureUsageFlags::None) && bIsTexture2D)
    {
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->GetTexture2D());

        D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format             = Desc.Format;
        ViewDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice = 0;

        TSharedRef<CD3D12DepthStencilView> DSV = dbg_new CD3D12DepthStencilView(Device, DepthStencilOfflineDescriptorHeap);
        if (!DSV->AllocateHandle())
        {
            return nullptr;
        }

        if (!DSV->CreateView(NewTexture->GetD3D12Resource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetDepthStencilView(DSV.ReleaseOwnership());
    }

    if (((Flags & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None) && ((Flags & ETextureUsageFlags::NoDefaultUAV) == ETextureUsageFlags::None) && bIsTexture2D)
    {
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->GetTexture2D());

        D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format               = Desc.Format;
        ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice   = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<CD3D12UnorderedAccessView> UAV = dbg_new CD3D12UnorderedAccessView(Device, ResourceOfflineDescriptorHeap);
        if (!UAV->AllocateHandle())
        {
            return nullptr;
        }

        if (!UAV->CreateView(nullptr, NewTexture->GetD3D12Resource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetUnorderedAccessView(UAV.ReleaseOwnership());
    }

    if (InitialData)
    {
        // TODO: Support other types than texture 2D

        CRHITexture2D* Texture2D = NewTexture->GetTexture2D();
        if (!Texture2D)
        {
            return nullptr;
        }

        DirectCmdContext->StartContext();

        DirectCmdContext->TransitionTexture(Texture2D, EResourceAccess::Common, EResourceAccess::CopyDest);
        DirectCmdContext->UpdateTexture2D(Texture2D, SizeX, SizeY, 0, InitialData->GetData());

        // NOTE: Transition into InitialState
        DirectCmdContext->TransitionTexture(Texture2D, EResourceAccess::CopyDest, InitialState);

        DirectCmdContext->FinishContext();
    }
    else
    {
        if (InitialState != EResourceAccess::Common)
        {
            DirectCmdContext->StartContext();
            DirectCmdContext->TransitionTexture(NewTexture.Get(), EResourceAccess::Common, InitialState);
            DirectCmdContext->FinishContext();
        }
    }

    return NewTexture.ReleaseOwnership();
}

CRHITexture2D* CD3D12CoreInterface::CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const CTextureClearValue& ClearValue)
{
    return CreateTexture<CD3D12RHITexture2D>(Format, Width, Height, 1, NumMips, NumSamples, Flags, InitialState, InitialData, ClearValue);
}

CRHITexture2DArray* CD3D12CoreInterface::CreateTexture2DArray(EFormat Format,uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 NumArraySlices, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const CTextureClearValue& ClearValue)
{
    return CreateTexture<CD3D12RHITexture2DArray>(Format, Width, Height, NumArraySlices, NumMips, NumSamples, Flags, InitialState, InitialData, ClearValue);
}

CRHITextureCube* CD3D12CoreInterface::CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMips, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const CTextureClearValue& ClearValue)
{
    return CreateTexture<CD3D12RHITextureCube>(Format, Size, Size, TEXTURE_CUBE_FACE_COUNT, NumMips, 1, Flags, InitialState, InitialData, ClearValue);
}

CRHITextureCubeArray* CD3D12CoreInterface::CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMips, uint32 NumArraySlices, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const CTextureClearValue& ClearValue)
{
    const uint32 ArraySlices = NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
    return CreateTexture<CD3D12RHITextureCubeArray>(Format, Size, Size, ArraySlices, NumMips, 1, Flags, InitialState, InitialData, ClearValue);
}

CRHITexture3D* CD3D12CoreInterface::CreateTexture3D(EFormat Format, uint32 Width, uint32 Height, uint32 Depth, uint32 NumMips, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const CTextureClearValue& ClearValue)
{
    return CreateTexture<CD3D12RHITexture3D>(Format, Width, Height, Depth, NumMips, 1, Flags, InitialState, InitialData, ClearValue);
}

CRHISamplerState* CD3D12CoreInterface::RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer)
{
    D3D12_SAMPLER_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.AddressU       = ConvertSamplerMode(Initializer.AddressU);
    Desc.AddressV       = ConvertSamplerMode(Initializer.AddressV);
    Desc.AddressW       = ConvertSamplerMode(Initializer.AddressW);
    Desc.ComparisonFunc = ConvertComparisonFunc(Initializer.ComparisonFunc);
    Desc.Filter         = ConvertSamplerFilter(Initializer.Filter);
    Desc.MaxAnisotropy  = Initializer.MaxAnisotropy;
    Desc.MaxLOD         = Initializer.MaxLOD;
    Desc.MinLOD         = Initializer.MinLOD;
    Desc.MipLODBias     = Initializer.MipLODBias;

    CMemory::Memcpy(Desc.BorderColor, Initializer.BorderColor.Data(), sizeof(Desc.BorderColor));

    TSharedRef<CD3D12SamplerState> Sampler = dbg_new CD3D12SamplerState(Device, SamplerOfflineDescriptorHeap);
    if (!Sampler->CreateSampler(Desc))
    {
        return nullptr;
    }
    else
    {
        return Sampler.ReleaseOwnership();
    }
}

template<typename D3D12BufferType>
bool CD3D12CoreInterface::CreateBuffer(D3D12BufferType* Buffer, uint32 Size, const CRHIBufferInitializer& Initializer)
{
    D3D12_ERROR(Buffer != nullptr, "Buffer cannot be nullptr");

    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = ConvertBufferFlags(Initializer.UsageFlags);
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = Size;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    D3D12_HEAP_TYPE       D3D12HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES D3D12InitialState = D3D12_RESOURCE_STATE_COMMON;
    if (Initializer.IsDynamic())
    {
        D3D12HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        D3D12InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    // Limit the scope of the new resource
    {
        TSharedRef<CD3D12Resource> D3D12Resource = dbg_new CD3D12Resource(Device, Desc, D3D12HeapType);
        if (!D3D12Resource->Init(D3D12InitialState, nullptr))
        {
            return false;
        }
        else
        {
            Buffer->SetResource(D3D12Resource.ReleaseOwnership());
        }
    }

    CRHIBufferDataInitializer* InitialData = Initializer.InitialData;
    if (InitialData)
    {
        D3D12_ERROR(InitialData->Size <= Size, "Size of InitialData is larger than the allocated memory");

        if (Initializer.IsDynamic())
        {
            CD3D12Resource* D3D12Resource = Buffer->GetD3D12Resource();

            void* BufferData = D3D12Resource->Map(0, 0);
            if (!BufferData)
            {
                return false;
            }

            // Copy over relevant data
            CMemory::Memcpy(BufferData, InitialData->BufferData, InitialData->Size);

            // Set the remaining, unused memory to zero
            CMemory::Memzero(reinterpret_cast<uint8*>(BufferData) + InitialData->Size, Size - InitialData->Size);

            D3D12Resource->Unmap(0, 0);
        }
        else
        {
            DirectCmdContext->StartContext();

            DirectCmdContext->TransitionBuffer(Buffer, EResourceAccess::Common, EResourceAccess::CopyDest);
            DirectCmdContext->UpdateBuffer(Buffer, 0, InitialData->Size, InitialData->BufferData);

            // NOTE: Transfer to the initial state
            DirectCmdContext->TransitionBuffer(Buffer, EResourceAccess::CopyDest, Initializer.InitialState);

            DirectCmdContext->FinishContext();
        }
    }
    else
    {
        if (Initializer.InitialState != EResourceAccess::Common && Initializer.IsDynamic())
        {
            DirectCmdContext->StartContext();
            DirectCmdContext->TransitionBuffer(Buffer, EResourceAccess::Common, Initializer.InitialState);
            DirectCmdContext->FinishContext();
        }
    }

    return true;
}

CRHIVertexBuffer* CD3D12CoreInterface::RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
{
    TSharedRef<CD3D12VertexBuffer> NewBuffer = dbg_new CD3D12VertexBuffer(Device, Initializer);
    if (!CreateBuffer(NewBuffer.Get(), Initializer.GetSize(), Initializer))
    {
        LOG_ERROR("[CD3D12CoreInterface]: Failed to create VertexBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIIndexBuffer* CD3D12CoreInterface::RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
{
    const uint32 SizeInBytes        = Initializer.GetSize();
    const uint32 AlignedSizeInBytes = (Initializer.AllowSRV() || Initializer.AllowUAV()) ? NMath::AlignUp<uint32>(SizeInBytes, sizeof(uint32)) : SizeInBytes;

    TSharedRef<CD3D12IndexBuffer> NewBuffer = dbg_new CD3D12IndexBuffer(Device, Initializer);
    if (!CreateBuffer(NewBuffer.Get(), AlignedSizeInBytes, Initializer))
    {
        LOG_ERROR("[CD3D12CoreInterface]: Failed to create IndexBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIConstantBuffer* CD3D12CoreInterface::RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
{
    Assert(!Initializer.AllowSRV() && Initializer.AllowUAV());

    const uint32 AlignedSizeInBytes = NMath::AlignUp<uint32>(Initializer.Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    TSharedRef<CD3D12ConstantBuffer> NewBuffer = dbg_new CD3D12ConstantBuffer(Device, ResourceOfflineDescriptorHeap, Initializer);
    if (!CreateBuffer(NewBuffer.Get(), AlignedSizeInBytes, Initializer))
    {
        LOG_ERROR("[CD3D12CoreInterface]: Failed to create ConstantBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIGenericBuffer* CD3D12CoreInterface::RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
{
    TSharedRef<CD3D12GenericBuffer> NewBuffer = dbg_new CD3D12GenericBuffer(Device, Initializer);
    if (!CreateBuffer(NewBuffer.Get(), Initializer.Size, Initializer))
    {
        LOG_ERROR("[CD3D12CoreInterface]: Failed to create StructuredBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIRayTracingGeometry* CD3D12CoreInterface::RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
{
    CD3D12VertexBuffer* D3D12VertexBuffer = static_cast<CD3D12VertexBuffer*>(Initializer.VertexBuffer);
    CD3D12IndexBuffer*  D3D12IndexBuffer  = static_cast<CD3D12IndexBuffer*>(Initializer.IndexBuffer);

    TSharedRef<CD3D12RayTracingGeometry> D3D12Geometry = dbg_new CD3D12RayTracingGeometry(Device, Initializer);

    DirectCmdContext->StartContext();
    
    if (!D3D12Geometry->Build(*DirectCmdContext, D3D12VertexBuffer, D3D12IndexBuffer, false))
    {
        CDebug::DebugBreak();
        D3D12Geometry.Reset();
    }

    DirectCmdContext->FinishContext();

    return D3D12Geometry.ReleaseOwnership();
}

CRHIRayTracingScene* CD3D12CoreInterface::RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
{
    TSharedRef<CD3D12RayTracingScene> D3D12Scene = dbg_new CD3D12RayTracingScene(Device, Initializer);

    DirectCmdContext->StartContext();

    if (!D3D12Scene->Build(*DirectCmdContext, Initializer.Instances.CreateView(), false))
    {
        CDebug::DebugBreak();
        D3D12Scene.Reset();
    }

    DirectCmdContext->FinishContext();

    return D3D12Scene.ReleaseOwnership();
}

CRHIShaderResourceView* CD3D12CoreInterface::CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    // TODO: Expose in ShaderResourceViewCreateInfo
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    CD3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::Texture2D)
    {
        CRHITexture2D* Texture      = CreateInfo.Texture2D.Texture;
        CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None) && CreateInfo.Texture2D.Format != EFormat::Unknown);

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
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::Texture2DArray)
    {
        CRHITexture2DArray* Texture      = CreateInfo.Texture2DArray.Texture;
        CD3D12Texture*      D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None) && CreateInfo.Texture2DArray.Format != EFormat::Unknown);

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
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::TextureCube)
    {
        CRHITextureCube* Texture      = CreateInfo.TextureCube.Texture;
        CD3D12Texture*   D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None) && CreateInfo.TextureCube.Format != EFormat::Unknown);

        Desc.Format                          = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MipLevels           = CreateInfo.TextureCube.NumMips;
        Desc.TextureCube.MostDetailedMip     = CreateInfo.TextureCube.Mip;
        Desc.TextureCube.ResourceMinLODClamp = CreateInfo.TextureCube.MinMipBias;
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::TextureCubeArray)
    {
        CRHITextureCubeArray* Texture      = CreateInfo.TextureCubeArray.Texture;
        CD3D12Texture*        D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None) && CreateInfo.TextureCubeArray.Format != EFormat::Unknown);

        Desc.Format                               = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MipLevels           = CreateInfo.TextureCubeArray.NumMips;
        Desc.TextureCubeArray.MostDetailedMip     = CreateInfo.TextureCubeArray.Mip;
        Desc.TextureCubeArray.ResourceMinLODClamp = CreateInfo.TextureCubeArray.MinMipBias;
        // ArraySlice * 6 to get the first Texture2D face
        Desc.TextureCubeArray.First2DArrayFace    = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        Desc.TextureCubeArray.NumCubes            = CreateInfo.TextureCubeArray.NumArraySlices;
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::Texture3D)
    {
        CRHITexture3D* Texture      = CreateInfo.Texture3D.Texture;
        CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None) && CreateInfo.Texture3D.Format != EFormat::Unknown);

        Desc.Format                        = ConvertFormat(CreateInfo.Texture3D.Format);
        Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipLevels           = CreateInfo.Texture3D.NumMips;
        Desc.Texture3D.MostDetailedMip     = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.ResourceMinLODClamp = CreateInfo.Texture3D.MinMipBias;
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::VertexBuffer)
    {
        CRHIVertexBuffer* Buffer      = CreateInfo.VertexBuffer.Buffer;
        CD3D12Buffer*     D3D12Buffer = D3D12BufferCast(Buffer);
        Resource = D3D12Buffer->GetD3D12Resource();

        Assert(((Buffer->GetFlags() & EBufferUsageFlags::AllowSRV) != EBufferUsageFlags::None));

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements         = CreateInfo.VertexBuffer.NumVertices;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::IndexBuffer)
    {
        CRHIIndexBuffer* Buffer      = CreateInfo.IndexBuffer.Buffer;
        CD3D12Buffer*    D3D12Buffer = D3D12BufferCast(Buffer);
        Resource = D3D12Buffer->GetD3D12Resource();

        Assert(((Buffer->GetFlags() & EBufferUsageFlags::AllowSRV) != EBufferUsageFlags::None));
        Assert(Buffer->GetFormat() != EIndexFormat::uint16);

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements         = CreateInfo.IndexBuffer.NumIndices;
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::GenericBuffer)
    {
        CRHIGenericBuffer* Buffer      = CreateInfo.StructuredBuffer.Buffer;
        CD3D12Buffer*      D3D12Buffer = D3D12BufferCast(Buffer);
        Resource = D3D12Buffer->GetD3D12Resource();

        Assert(((Buffer->GetFlags() & EBufferUsageFlags::AllowSRV) != EBufferUsageFlags::None));

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    Assert(Resource != nullptr);

    TSharedRef<CD3D12ShaderResourceView> DxView = dbg_new CD3D12ShaderResourceView(Device, ResourceOfflineDescriptorHeap);
    if (!DxView->AllocateHandle())
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

CRHIUnorderedAccessView* CD3D12CoreInterface::CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::Texture2D)
    {
        CRHITexture2D* Texture      = CreateInfo.Texture2D.Texture;
        CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None) && CreateInfo.Texture2D.Format != EFormat::Unknown);

        Desc.Format               = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        Desc.Texture2D.MipSlice   = CreateInfo.Texture2D.Mip;
        Desc.Texture2D.PlaneSlice = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::Texture2DArray)
    {
        CRHITexture2DArray* Texture      = CreateInfo.Texture2DArray.Texture;
        CD3D12Texture*      D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None) && CreateInfo.Texture2DArray.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.Texture2DArray.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.Texture2DArray.Mip;
        Desc.Texture2DArray.ArraySize       = CreateInfo.Texture2DArray.NumArraySlices;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::TextureCube)
    {
        CRHITextureCube* Texture      = CreateInfo.TextureCube.Texture;
        CD3D12Texture*   D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None) && CreateInfo.TextureCube.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.TextureCube.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = 0;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::TextureCubeArray)
    {
        CRHITextureCubeArray* Texture      = CreateInfo.TextureCubeArray.Texture;
        CD3D12Texture*        D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None) && CreateInfo.TextureCubeArray.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.TextureCubeArray.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = CreateInfo.TextureCubeArray.NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::Texture3D)
    {
        CRHITexture3D* Texture      = CreateInfo.Texture3D.Texture;
        CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None) && CreateInfo.Texture3D.Format != EFormat::Unknown);

        Desc.Format                = ConvertFormat(CreateInfo.Texture3D.Format);
        Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = CreateInfo.Texture3D.DepthSlice;
        Desc.Texture3D.WSize       = CreateInfo.Texture3D.NumDepthSlices;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::VertexBuffer)
    {
        CRHIVertexBuffer* Buffer      = CreateInfo.VertexBuffer.Buffer;
        CD3D12Buffer*     D3D12Buffer = D3D12BufferCast(Buffer);
        Resource = D3D12Buffer->GetD3D12Resource();

        Assert(((Buffer->GetFlags() & EBufferUsageFlags::AllowUAV) != EBufferUsageFlags::None));

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements         = CreateInfo.VertexBuffer.NumVertices;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::IndexBuffer)
    {
        CRHIIndexBuffer* Buffer      = CreateInfo.IndexBuffer.Buffer;
        CD3D12Buffer*    D3D12Buffer = D3D12BufferCast(Buffer);
        Resource = D3D12Buffer->GetD3D12Resource();

        Assert(((Buffer->GetFlags() & EBufferUsageFlags::AllowUAV) != EBufferUsageFlags::None));

        Desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements  = CreateInfo.IndexBuffer.NumIndices;

        // TODO: What if the index type is 16-bit?
        Assert(Buffer->GetFormat() != EIndexFormat::uint16);

        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::GenericBuffer)
    {
        CRHIGenericBuffer* Buffer      = CreateInfo.StructuredBuffer.Buffer;
        CD3D12Buffer*      D3D12Buffer = D3D12BufferCast(Buffer);
        Resource = D3D12Buffer->GetD3D12Resource();

        Assert(((Buffer->GetFlags() & EBufferUsageFlags::AllowUAV) != EBufferUsageFlags::None));

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    TSharedRef<CD3D12UnorderedAccessView> DxView = dbg_new CD3D12UnorderedAccessView(Device, ResourceOfflineDescriptorHeap);
    if (!DxView->AllocateHandle())
    {
        return nullptr;
    }

    Assert(Resource != nullptr);

    // TODO: Expose counter resource
    if (DxView->CreateView(nullptr, Resource, Desc))
    {
        return DxView.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHIRenderTargetView* CD3D12CoreInterface::CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo)
{
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat(CreateInfo.Format);
    Assert(CreateInfo.Format != EFormat::Unknown);

    if (CreateInfo.Type == SRHIRenderTargetViewInfo::EType::Texture2D)
    {
        CRHITexture2D* Texture      = CreateInfo.Texture2D.Texture;
        CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowRTV) != ETextureUsageFlags::None));

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
    else if (CreateInfo.Type == SRHIRenderTargetViewInfo::EType::Texture2DArray)
    {
        CRHITexture2DArray* Texture      = CreateInfo.Texture2DArray.Texture;
        CD3D12Texture*      D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowRTV) != ETextureUsageFlags::None));

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
    else if (CreateInfo.Type == SRHIRenderTargetViewInfo::EType::TextureCube)
    {
        CRHITextureCube* Texture      = CreateInfo.TextureCube.Texture;
        CD3D12Texture*   D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowRTV) != ETextureUsageFlags::None));

        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIRenderTargetViewInfo::EType::TextureCubeArray)
    {
        CRHITextureCubeArray* Texture      = CreateInfo.TextureCubeArray.Texture;
        CD3D12Texture*        D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowRTV) != ETextureUsageFlags::None));

        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIRenderTargetViewInfo::EType::Texture3D)
    {
        CRHITexture3D* Texture      = CreateInfo.Texture3D.Texture;
        CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowRTV) != ETextureUsageFlags::None));

        Desc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = CreateInfo.Texture3D.DepthSlice;
        Desc.Texture3D.WSize       = CreateInfo.Texture3D.NumDepthSlices;
    }

    TSharedRef<CD3D12RenderTargetView> DxView = dbg_new CD3D12RenderTargetView(Device, RenderTargetOfflineDescriptorHeap);
    if (!DxView->AllocateHandle())
    {
        return nullptr;
    }

    Assert(Resource != nullptr);

    if (!DxView->CreateView(Resource, Desc))
    {
        return nullptr;
    }
    else
    {
        return DxView.ReleaseOwnership();
    }
}

CRHIDepthStencilView* CD3D12CoreInterface::CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat(CreateInfo.Format);
    Assert(CreateInfo.Format != EFormat::Unknown);

    if (CreateInfo.Type == SRHIDepthStencilViewInfo::EType::Texture2D)
    {
        CRHITexture2D* Texture      = CreateInfo.Texture2D.Texture;
        CD3D12Texture* D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowDSV) != ETextureUsageFlags::None));

        if (Texture->IsMultiSampled())
        {
            Desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice = CreateInfo.Texture2D.Mip;
        }
        else
        {
            Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (CreateInfo.Type == SRHIDepthStencilViewInfo::EType::Texture2DArray)
    {
        CRHITexture2DArray* Texture      = CreateInfo.Texture2DArray.Texture;
        CD3D12Texture*      D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowDSV) != ETextureUsageFlags::None));

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
    else if (CreateInfo.Type == SRHIDepthStencilViewInfo::EType::TextureCube)
    {
        CRHITextureCube* Texture      = CreateInfo.TextureCube.Texture;
        CD3D12Texture*   D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowDSV) != ETextureUsageFlags::None));

        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
    }
    else if (CreateInfo.Type == SRHIDepthStencilViewInfo::EType::TextureCubeArray)
    {
        CRHITextureCubeArray* Texture      = CreateInfo.TextureCubeArray.Texture;
        CD3D12Texture*        D3D12Texture = D3D12TextureCast(Texture);
        Resource = D3D12Texture->GetD3D12Resource();

        Assert(((Texture->GetFlags() & ETextureUsageFlags::AllowDSV) != ETextureUsageFlags::None));

        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
    }

    TSharedRef<CD3D12DepthStencilView> DxView = dbg_new CD3D12DepthStencilView(Device, DepthStencilOfflineDescriptorHeap);
    if (!DxView->AllocateHandle())
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

CRHIComputeShader* CD3D12CoreInterface::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12ComputeShader> Shader = dbg_new CD3D12ComputeShader(Device, ShaderCode);
    if (!Shader->Init())
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIVertexShader* CD3D12CoreInterface::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12VertexShader> Shader = dbg_new CD3D12VertexShader(Device, ShaderCode);
    if (!CD3D12Shader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIHullShader* CD3D12CoreInterface::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIDomainShader* CD3D12CoreInterface::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIGeometryShader* CD3D12CoreInterface::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIMeshShader* CD3D12CoreInterface::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIAmplificationShader* CD3D12CoreInterface::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIPixelShader* CD3D12CoreInterface::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12PixelShader> Shader = dbg_new CD3D12PixelShader(Device, ShaderCode);
    if (!CD3D12Shader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIRayGenShader* CD3D12CoreInterface::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayGenShader> Shader = dbg_new CD3D12RayGenShader(Device, ShaderCode);
    if (!CD3D12RayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12CoreInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayAnyHitShader* CD3D12CoreInterface::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayAnyHitShader> Shader = dbg_new CD3D12RayAnyHitShader(Device, ShaderCode);
    if (!CD3D12RayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12CoreInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayClosestHitShader* CD3D12CoreInterface::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayClosestHitShader> Shader = dbg_new CD3D12RayClosestHitShader(Device, ShaderCode);
    if (!CD3D12RayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12CoreInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayMissShader* CD3D12CoreInterface::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayMissShader> Shader = dbg_new CD3D12RayMissShader(Device, ShaderCode);
    if (!CD3D12RayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12CoreInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIDepthStencilState* CD3D12CoreInterface::RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer)
{
    D3D12_DEPTH_STENCIL_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.DepthEnable      = Initializer.bDepthEnable;
    Desc.DepthFunc        = ConvertComparisonFunc(Initializer.DepthFunc);
    Desc.DepthWriteMask   = ConvertDepthWriteMask(Initializer.DepthWriteMask);
    Desc.StencilEnable    = Initializer.bStencilEnable;
    Desc.StencilReadMask  = Initializer.StencilReadMask;
    Desc.StencilWriteMask = Initializer.StencilWriteMask;
    Desc.FrontFace        = ConvertDepthStencilOp(Initializer.FrontFace);
    Desc.BackFace         = ConvertDepthStencilOp(Initializer.BackFace);

    return dbg_new CD3D12DepthStencilState(Device, Desc);
}

CRHIRasterizerState* CD3D12CoreInterface::RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer)
{
    D3D12_RASTERIZER_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.AntialiasedLineEnable = Initializer.bAntialiasedLineEnable;
    Desc.ConservativeRaster    = (Initializer.bEnableConservativeRaster) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    Desc.CullMode              = ConvertCullMode(Initializer.CullMode);
    Desc.DepthBias             = Initializer.DepthBias;
    Desc.DepthBiasClamp        = Initializer.DepthBiasClamp;
    Desc.DepthClipEnable       = Initializer.bDepthClipEnable;
    Desc.SlopeScaledDepthBias  = Initializer.SlopeScaledDepthBias;
    Desc.FillMode              = ConvertFillMode(Initializer.FillMode);
    Desc.ForcedSampleCount     = Initializer.ForcedSampleCount;
    Desc.FrontCounterClockwise = Initializer.bFrontCounterClockwise;
    Desc.MultisampleEnable     = Initializer.bMultisampleEnable;

    return dbg_new CD3D12RasterizerState(Device, Desc);
}

CRHIBlendState* CD3D12CoreInterface::RHICreateBlendState(const CRHIBlendStateInitializer& Initializer)
{
    D3D12_BLEND_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.AlphaToCoverageEnable  = Initializer.bAlphaToCoverageEnable;
    Desc.IndependentBlendEnable = Initializer.bIndependentBlendEnable;
    for (uint32 i = 0; i < 8; i++)
    {
        Desc.RenderTarget[i].BlendEnable           = Initializer.RenderTargets[i].bBlendEnable;
        Desc.RenderTarget[i].BlendOp               = ConvertBlendOp(Initializer.RenderTargets[i].BlendOp);
        Desc.RenderTarget[i].BlendOpAlpha          = ConvertBlendOp(Initializer.RenderTargets[i].BlendOpAlpha);
        Desc.RenderTarget[i].DestBlend             = ConvertBlend(Initializer.RenderTargets[i].DstBlend);
        Desc.RenderTarget[i].DestBlendAlpha        = ConvertBlend(Initializer.RenderTargets[i].DstBlendAlpha);
        Desc.RenderTarget[i].SrcBlend              = ConvertBlend(Initializer.RenderTargets[i].SrcBlend);
        Desc.RenderTarget[i].SrcBlendAlpha         = ConvertBlend(Initializer.RenderTargets[i].SrcBlendAlpha);
        Desc.RenderTarget[i].LogicOpEnable         = Initializer.RenderTargets[i].bLogicOpEnable;
        Desc.RenderTarget[i].LogicOp               = ConvertLogicOp(Initializer.RenderTargets[i].LogicOp);
        Desc.RenderTarget[i].RenderTargetWriteMask = ConvertRenderTargetWriteState(Initializer.RenderTargets[i].RenderTargetWriteMask);
    }

    return dbg_new CD3D12BlendState(Device, Desc);
}

CRHIVertexInputLayout* CD3D12CoreInterface::RHICreateVertexInputLayout(const CRHIVertexInputLayoutInitializer& Initializer)
{
    return dbg_new CD3D12VertexInputLayout(Device, Initializer);
}

CRHIGraphicsPipelineState* CD3D12CoreInterface::RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Initializer)
{
    TSharedRef<CD3D12GraphicsPipelineState> NewPipelineState = dbg_new CD3D12GraphicsPipelineState(Device);
    if (!NewPipelineState->Init(Initializer))
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIComputePipelineState* CD3D12CoreInterface::RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer)
{
    Assert(Initializer.Shader != nullptr);

    TSharedRef<CD3D12ComputeShader>        Shader           = MakeSharedRef<CD3D12ComputeShader>(Initializer.Shader);
    TSharedRef<CD3D12ComputePipelineState> NewPipelineState = dbg_new CD3D12ComputePipelineState(Device, Shader);
    if (!NewPipelineState->Init())
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIRayTracingPipelineState* CD3D12CoreInterface::RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Initializer)
{
    TSharedRef<CD3D12RayTracingPipelineState> NewPipelineState = dbg_new CD3D12RayTracingPipelineState(Device);
    if (NewPipelineState->Init(Initializer))
    {
        return NewPipelineState.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHITimestampQuery* CD3D12CoreInterface::RHICreateTimestampQuery()
{
    return CD3D12TimestampQuery::Create(Device);
}

CRHIViewport* CD3D12CoreInterface::RHICreateViewport(const CRHIViewportInitializer& Initializer)
{
    TSharedRef<CD3D12Viewport> Viewport = dbg_new CD3D12Viewport(Device, DirectCmdContext, Initializer);
    if (Viewport->Init())
    {
        return Viewport.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

bool CD3D12CoreInterface::RHIQueryUAVFormatSupport(EFormat Format) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
    CMemory::Memzero(&FeatureData);

    HRESULT Result = Device->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
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

            Result = Device->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
            if (FAILED(Result) || (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0)
            {
                return false;
            }
        }
    }

    return true;
}

void CD3D12CoreInterface::RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport) const
{
    D3D12_RAYTRACING_TIER Tier = Device->GetRayTracingTier();
    if (Tier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    {
        if (Tier == D3D12_RAYTRACING_TIER_1_1)
        {
            OutSupport.Tier = ERHIRayTracingTier::Tier1_1;
        }
        else if (Tier == D3D12_RAYTRACING_TIER_1_0)
        {
            OutSupport.Tier = ERHIRayTracingTier::Tier1;
        }

        OutSupport.MaxRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
    }
    else
    {
        OutSupport.Tier = ERHIRayTracingTier::NotSupported;
    }
}

void CD3D12CoreInterface::RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport) const
{
    D3D12_VARIABLE_SHADING_RATE_TIER Tier = Device->GetVariableRateShadingTier();
    if (Tier == D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
    {
        OutSupport.Tier = ERHIShadingRateTier::NotSupported;
    }
    else if (Tier == D3D12_VARIABLE_SHADING_RATE_TIER_1)
    {
        OutSupport.Tier = ERHIShadingRateTier::Tier1;
    }
    else if (Tier == D3D12_VARIABLE_SHADING_RATE_TIER_2)
    {
        OutSupport.Tier = ERHIShadingRateTier::Tier2;
    }

    OutSupport.ShadingRateImageTileSize = uint8(Device->GetVariableRateShadingTileSize());
}
