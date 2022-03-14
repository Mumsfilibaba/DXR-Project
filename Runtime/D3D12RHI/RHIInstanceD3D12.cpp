#include "CoreApplication/Windows/WindowsWindow.h"

#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12RootSignature.h"
#include "D3D12Core.h"
#include "RHIInstanceD3D12.h"
#include "D3D12Views.h"
#include "D3D12RayTracing.h"
#include "D3D12PipelineState.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12SamplerState.h"
#include "D3D12Viewport.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12TimestampQuery.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12 Helpers

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12Texture2D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12Texture2DArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12TextureCube>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12TextureCubeArray>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

template<>
inline D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension<CD3D12Texture3D>()
{
    return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
}

template<typename D3D12TextureType>
inline bool IsTextureCube()
{
    return false;
}

template<>
inline bool IsTextureCube<CD3D12TextureCube>()
{
    return true;
}

template<>
inline bool IsTextureCube<CD3D12TextureCubeArray>()
{
    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIInstanceD3D12

CRHIInstanceD3D12* GD3D12RHIInstance = nullptr;

CRHIInstance* CRHIInstanceD3D12::CreateInstance() 
{ 
    return dbg_new CRHIInstanceD3D12(); 
}

CRHIInstanceD3D12::CRHIInstanceD3D12()
    : CRHIInstance(ERHIType::D3D12)
    , Device(nullptr)
    , DirectCommandContext(nullptr)
{
    GD3D12RHIInstance = this;
}

CRHIInstanceD3D12::~CRHIInstanceD3D12()
{
    DirectCommandContext.Reset();

    GenerateMipsTex2D_PSO.Reset();
    GenerateMipsTexCube_PSO.Reset();

    SafeDelete(RootSignatureCache);

    SafeRelease(ResourceOfflineDescriptorHeap);
    SafeRelease(RenderTargetOfflineDescriptorHeap);
    SafeRelease(DepthStencilOfflineDescriptorHeap);
    SafeRelease(SamplerOfflineDescriptorHeap);

    Device.Reset();

    GD3D12RHIInstance = nullptr;
}

bool CRHIInstanceD3D12::Initialize(bool bEnableDebug)
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

    Device = CD3D12Device::CreateDevice(this, bEnableDebug, bGPUBasedValidationOn, bDREDOn);
    if (!Device)
    {
        D3D12_ERROR_ALWAYS("Failed to create device");
        return false;
    }

    // RootSignature cache
    RootSignatureCache = dbg_new CD3D12RootSignatureCache(GetDevice());
    if (!RootSignatureCache->Initialize())
    {
        return false;
    }

    // Init Offline Descriptor heaps
    ResourceOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!ResourceOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    RenderTargetOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if (!RenderTargetOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    DepthStencilOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    if (!DepthStencilOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    SamplerOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!SamplerOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    // Init shader compiler
    GD3D12ShaderCompiler = dbg_new CD3D12ShaderCompiler();
    if (!GD3D12ShaderCompiler->Initialize())
    {
        return false;
    }

    // Init GenerateMips Shaders and pipeline states 
    TArray<uint8> Code;
    if (!GD3D12ShaderCompiler->CompileFromFile("../Runtime/Shaders/GenerateMipsTex2D.hlsl", "Main", nullptr, ERHIShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        D3D12_ERROR_ALWAYS("Failed to compile GenerateMipsTex2D Shader");
        return false;
    }

    TSharedRef<CD3D12ComputeShader> Shader = dbg_new CD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        CDebug::DebugBreak();
        return false;
    }

    GenerateMipsTex2D_PSO = dbg_new CD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTex2D_PSO->Initialize())
    {
        D3D12_ERROR_ALWAYS("Failed to create GenerateMipsTex2D PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTex2D_PSO->SetName("GenerateMipsTex2D Gen PSO");
    }

    if (!GD3D12ShaderCompiler->CompileFromFile("../Runtime/Shaders/GenerateMipsTexCube.hlsl", "Main", nullptr, ERHIShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        D3D12_ERROR_ALWAYS("Failed to compile GenerateMipsTexCube Shader");
        return false;
    }

    Shader = dbg_new CD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        CDebug::DebugBreak();
        return false;
    }

    GenerateMipsTexCube_PSO = dbg_new CD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTexCube_PSO->Initialize())
    {
        D3D12_ERROR_ALWAYS("Failed to create GenerateMipsTexCube PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTexCube_PSO->SetName("GenerateMipsTexCube Gen PSO");
    }

    // Init context
    DirectCommandContext = CD3D12CommandContext::CreateContext(GetDevice());
    if (!DirectCommandContext)
    {
        return false;
    }

    return true;
}

template<typename D3D12TextureType>
D3D12TextureType* CRHIInstanceD3D12::CreateTexture(ERHIFormat Format, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint32 NumMips, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    TSharedRef<D3D12TextureType> NewTexture = dbg_new D3D12TextureType(GetDevice(), Format, SizeX, SizeY, SizeZ, NumMips, NumSamples, Flags, OptimalClearValue);

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
        const int32 Quality     = Device->GetMultisampleQuality(Desc.Format, NumSamples);
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
        ClearValue.Format = (OptimalClearValue.GetFormat() != ERHIFormat::Unknown) ? ConvertFormat(OptimalClearValue.GetFormat()) : Desc.Format;
        if (OptimalClearValue.GetType() == SClearValue::EType::DepthStencil)
        {
            ClearValue.DepthStencil.Depth   = OptimalClearValue.AsDepthStencil().Depth;
            ClearValue.DepthStencil.Stencil = OptimalClearValue.AsDepthStencil().Stencil;
            ClearValuePtr = &ClearValue;
        }
        else if (OptimalClearValue.GetType() == SClearValue::EType::Color)
        {
            CMemory::Memcpy(ClearValue.Color, OptimalClearValue.AsColor().Elements, sizeof(float[4]));
            ClearValuePtr = &ClearValue;
        }
    }

    CD3D12ResourceRef Resource = CD3D12Resource::CreateResource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, ClearValuePtr);
    if (!Resource)
    {
        return nullptr;
    }
    else
    {
        NewTexture->SetResource(Resource.ReleaseOwnership());
    }

    if ((Flags & TextureFlag_SRV) && !(Flags & TextureFlag_NoDefaultSRV))
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

        TSharedRef<CD3D12ShaderResourceView> SRV = dbg_new CD3D12ShaderResourceView(GetDevice(), ResourceOfflineDescriptorHeap);
        if (!SRV->AllocateHandle())
        {
            return nullptr;
        }

        if (!SRV->CreateView(NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture->SetShaderResourceView(SRV.ReleaseOwnership());
    }

    // TODO: Fix for other resources that Texture2D?
    const bool bIsTexture2D = (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) && (SizeZ == 1);
    if (Flags & TextureFlag_RTV && !(Flags & TextureFlag_NoDefaultRTV) && bIsTexture2D)
    {
        CD3D12Texture2D* NewTexture2D = static_cast<CD3D12Texture2D*>(NewTexture->AsTexture2D());

        D3D12_RENDER_TARGET_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format               = Desc.Format;
        ViewDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice   = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<CD3D12RenderTargetView> RTV = dbg_new CD3D12RenderTargetView(GetDevice(), RenderTargetOfflineDescriptorHeap);
        if (!RTV->AllocateHandle())
        {
            return nullptr;
        }

        if (!RTV->CreateView(NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetRenderTargetView(RTV.ReleaseOwnership());
    }

    if (Flags & TextureFlag_DSV && !(Flags & TextureFlag_NoDefaultDSV) && bIsTexture2D)
    {
        CD3D12Texture2D* NewTexture2D = static_cast<CD3D12Texture2D*>(NewTexture->AsTexture2D());

        D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format             = Desc.Format;
        ViewDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice = 0;

        TSharedRef<CD3D12DepthStencilView> DSV = dbg_new CD3D12DepthStencilView(GetDevice(), DepthStencilOfflineDescriptorHeap);
        if (!DSV->AllocateHandle())
        {
            return nullptr;
        }

        if (!DSV->CreateView(NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetDepthStencilView(DSV.ReleaseOwnership());
    }

    if (Flags & TextureFlag_UAV && !(Flags & TextureFlag_NoDefaultUAV) && bIsTexture2D)
    {
        CD3D12Texture2D* NewTexture2D = static_cast<CD3D12Texture2D*>(NewTexture->AsTexture2D());

        D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format               = Desc.Format;
        ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice   = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<CD3D12UnorderedAccessView> UAV = dbg_new CD3D12UnorderedAccessView(GetDevice(), ResourceOfflineDescriptorHeap);
        if (!UAV->AllocateHandle())
        {
            return nullptr;
        }

        if (!UAV->CreateView(nullptr, NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture2D->SetUnorderedAccessView(UAV.ReleaseOwnership());
    }

    if (InitialData)
    {
        // TODO: Support other types than texture 2D

        CRHITexture2D* Texture2D = NewTexture->AsTexture2D();
        if (!Texture2D)
        {
            return nullptr;
        }

        DirectCommandContext->Begin();

        DirectCommandContext->TransitionTexture(Texture2D, ERHIResourceState::Common, ERHIResourceState::CopyDest);
        DirectCommandContext->UpdateTexture2D(Texture2D, SizeX, SizeY, 0, InitialData->GetData());

        // NOTE: Transition into InitialState
        DirectCommandContext->TransitionTexture(Texture2D, ERHIResourceState::CopyDest, InitialState);

        DirectCommandContext->End();
    }
    else
    {
        if (InitialState != ERHIResourceState::Common)
        {
            DirectCommandContext->Begin();
            DirectCommandContext->TransitionTexture(NewTexture.Get(), ERHIResourceState::Common, InitialState);
            DirectCommandContext->End();
        }
    }

    return NewTexture.ReleaseOwnership();
}

CRHITexture2D* CRHIInstanceD3D12::CreateTexture2D(ERHIFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12Texture2D>(Format, Width, Height, 1, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITexture2DArray* CRHIInstanceD3D12::CreateTexture2DArray(ERHIFormat Format,uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12Texture2DArray>(Format, Width, Height, NumArraySlices, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITextureCube* CRHIInstanceD3D12::CreateTextureCube(ERHIFormat Format, uint32 Size, uint32 NumMips, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12TextureCube>(Format, Size, Size, TEXTURE_CUBE_FACE_COUNT, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITextureCubeArray* CRHIInstanceD3D12::CreateTextureCubeArray(ERHIFormat Format, uint32 Size, uint32 NumMips, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    const uint32 ArraySlices = NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
    return CreateTexture<CD3D12TextureCubeArray>(Format, Size, Size, ArraySlices, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITexture3D* CRHIInstanceD3D12::CreateTexture3D(ERHIFormat Format, uint32 Width, uint32 Height, uint32 Depth, uint32 NumMips, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12Texture3D>(Format, Width, Height, Depth, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHIBufferRef CRHIInstanceD3D12::CreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const SRHIResourceData* InitalData)
{
    CD3D12BufferRef NewBuffer = dbg_new CD3D12Buffer(GetDevice(), BufferDesc);
    if (!(NewBuffer && NewBuffer->Initialize(DirectCommandContext.Get(), InitialState, InitalData)))
    {
        D3D12_ERROR_ALWAYS("Failed to create Buffer");
        return nullptr;
    }
    else
    {
        return NewBuffer;
    }
}

CRHISamplerStateRef CRHIInstanceD3D12::CreateSamplerState(const CRHISamplerStateDesc& CreateInfo)
{
    D3D12_SAMPLER_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.AddressU       = ConvertSamplerMode(CreateInfo.AddressU);
    Desc.AddressV       = ConvertSamplerMode(CreateInfo.AddressV);
    Desc.AddressW       = ConvertSamplerMode(CreateInfo.AddressW);
    Desc.ComparisonFunc = ConvertComparisonFunc(CreateInfo.ComparisonFunc);
    Desc.Filter         = ConvertSamplerFilter(CreateInfo.Filter);
    Desc.MaxAnisotropy  = CreateInfo.MaxAnisotropy;
    Desc.MaxLOD         = CreateInfo.MaxLOD;
    Desc.MinLOD         = CreateInfo.MinLOD;
    Desc.MipLODBias     = CreateInfo.MipLODBias;

    CMemory::Memcpy(Desc.BorderColor, CreateInfo.BorderColor.Elements, sizeof(Desc.BorderColor));

    CD3D12SamplerStateRef NewSampler = dbg_new CD3D12SamplerState(GetDevice(), SamplerOfflineDescriptorHeap);
    if (!(NewSampler && NewSampler->Initialize(Desc)))
    {
        return nullptr;
    }
    else
    {
        return NewSampler;
    }
}


CRHIRayTracingGeometry* CRHIInstanceD3D12::CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices)
{
    TSharedRef<CD3D12RayTracingGeometry> Geometry = dbg_new CD3D12RayTracingGeometry(GetDevice(), Flags);
    Geometry->VertexBuffer = MakeSharedRef<CD3D12Buffer>(VertexBuffer);
    Geometry->VertexCount  = NumVertices;
    Geometry->IndexBuffer  = MakeSharedRef<CD3D12Buffer>(IndexBuffer);
    Geometry->IndexCount   = NumIndices;
    Geometry->IndexFormat  = IndexFormat;

    DirectCommandContext->Begin();

    if (!Geometry->Build(*DirectCommandContext, false))
    {
        CDebug::DebugBreak();
        Geometry.Reset();
    }

    DirectCommandContext->End();

    return Geometry.ReleaseOwnership();
}

CRHIRayTracingScene* CRHIInstanceD3D12::CreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    TSharedRef<CD3D12RayTracingScene> Scene = dbg_new CD3D12RayTracingScene(GetDevice(), Flags);

    DirectCommandContext->Begin();

    if (!Scene->Build(*DirectCommandContext, Instances, NumInstances, false))
    {
        CDebug::DebugBreak();
        Scene.Reset();
    }

    DirectCommandContext->End();

    return Scene.ReleaseOwnership();
}

CRHIShaderResourceView* CRHIInstanceD3D12::CreateShaderResourceView(const SRHIShaderResourceViewDesc& ViewDesc)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    // TODO: Expose in ShaderResourceViewCreateInfo
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    CD3D12Resource* Resource = nullptr;
    if (ViewDesc.Type == SRHIShaderResourceViewDesc::EType::Texture2D)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture2D.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture2D.Texture->IsSRV() && ViewDesc.Texture2D.Format != ERHIFormat::Unknown);

        Desc.Format = ConvertFormat(ViewDesc.Texture2D.Format);
        if (!ViewDesc.Texture2D.Texture->IsMultiSampled())
        {
            Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipLevels           = ViewDesc.Texture2D.NumMips;
            Desc.Texture2D.MostDetailedMip     = ViewDesc.Texture2D.Mip;
            Desc.Texture2D.ResourceMinLODClamp = ViewDesc.Texture2D.MinMipBias;
            Desc.Texture2D.PlaneSlice          = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (ViewDesc.Type == SRHIShaderResourceViewDesc::EType::Texture2DArray)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture2DArray.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture2DArray.Texture->IsSRV() && ViewDesc.Texture2DArray.Format != ERHIFormat::Unknown);

        Desc.Format = ConvertFormat(ViewDesc.Texture2DArray.Format);
        if (!ViewDesc.Texture2DArray.Texture->IsMultiSampled())
        {
            Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipLevels           = ViewDesc.Texture2DArray.NumMips;
            Desc.Texture2DArray.MostDetailedMip     = ViewDesc.Texture2DArray.Mip;
            Desc.Texture2DArray.ResourceMinLODClamp = ViewDesc.Texture2DArray.MinMipBias;
            Desc.Texture2DArray.ArraySize           = ViewDesc.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice     = ViewDesc.Texture2DArray.ArraySlice;
            Desc.Texture2DArray.PlaneSlice          = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = ViewDesc.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = ViewDesc.Texture2DArray.ArraySlice;
        }
    }
    else if (ViewDesc.Type == SRHIShaderResourceViewDesc::EType::TextureCube)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.TextureCube.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.TextureCube.Texture->IsSRV() && ViewDesc.TextureCube.Format != ERHIFormat::Unknown);

        Desc.Format                          = ConvertFormat(ViewDesc.Texture2D.Format);
        Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MipLevels           = ViewDesc.TextureCube.NumMips;
        Desc.TextureCube.MostDetailedMip     = ViewDesc.TextureCube.Mip;
        Desc.TextureCube.ResourceMinLODClamp = ViewDesc.TextureCube.MinMipBias;
    }
    else if (ViewDesc.Type == SRHIShaderResourceViewDesc::EType::TextureCubeArray)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.TextureCubeArray.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.TextureCubeArray.Texture->IsSRV() && ViewDesc.TextureCubeArray.Format != ERHIFormat::Unknown);

        Desc.Format                               = ConvertFormat(ViewDesc.Texture2D.Format);
        Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MipLevels           = ViewDesc.TextureCubeArray.NumMips;
        Desc.TextureCubeArray.MostDetailedMip     = ViewDesc.TextureCubeArray.Mip;
        Desc.TextureCubeArray.ResourceMinLODClamp = ViewDesc.TextureCubeArray.MinMipBias;
        Desc.TextureCubeArray.First2DArrayFace    = ViewDesc.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT; // ArraySlice * 6 to get the first Texture2D face
        Desc.TextureCubeArray.NumCubes            = ViewDesc.TextureCubeArray.NumArraySlices;
    }
    else if (ViewDesc.Type == SRHIShaderResourceViewDesc::EType::Texture3D)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture3D.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture3D.Texture->IsSRV() && ViewDesc.Texture3D.Format != ERHIFormat::Unknown);

        Desc.Format                        = ConvertFormat(ViewDesc.Texture3D.Format);
        Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipLevels           = ViewDesc.Texture3D.NumMips;
        Desc.Texture3D.MostDetailedMip     = ViewDesc.Texture3D.Mip;
        Desc.Texture3D.ResourceMinLODClamp = ViewDesc.Texture3D.MinMipBias;
    }
    else if (ViewDesc.Type == SRHIShaderResourceViewDesc::EType::VertexBuffer)
    {
        CD3D12Buffer* DxBuffer = static_cast<CD3D12Buffer*>(ViewDesc.VertexBuffer.Buffer);
        Resource = DxBuffer->GetResource();

        Assert(DxBuffer->IsSRV());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = ViewDesc.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements         = ViewDesc.VertexBuffer.NumVertices;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = DxBuffer->GetStride();
    }
    else if (ViewDesc.Type == SRHIShaderResourceViewDesc::EType::IndexBuffer)
    {
        CD3D12Buffer* DxBuffer = static_cast<CD3D12Buffer*>(ViewDesc.IndexBuffer.Buffer);
        Resource = DxBuffer->GetResource();

        Assert(DxBuffer->IsSRV());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = ViewDesc.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements         = ViewDesc.IndexBuffer.NumIndices;
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (ViewDesc.Type == SRHIShaderResourceViewDesc::EType::StructuredBuffer)
    {
        CD3D12Buffer* DxBuffer = static_cast<CD3D12Buffer*>(ViewDesc.StructuredBuffer.Buffer);
        Resource = DxBuffer->GetResource();

        Assert(DxBuffer->IsSRV());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = ViewDesc.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = ViewDesc.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = DxBuffer->GetStride();
    }

    Assert(Resource != nullptr);

    TSharedRef<CD3D12ShaderResourceView> DxView = dbg_new CD3D12ShaderResourceView(GetDevice(), ResourceOfflineDescriptorHeap);
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

CRHIUnorderedAccessView* CRHIInstanceD3D12::CreateUnorderedAccessView(const SRHIUnorderedAccessViewDesc& ViewDesc)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;
    if (ViewDesc.Type == SRHIUnorderedAccessViewDesc::EType::Texture2D)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture2D.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture2D.Texture->IsUAV() && ViewDesc.Texture2D.Format != ERHIFormat::Unknown);

        Desc.Format               = ConvertFormat(ViewDesc.Texture2D.Format);
        Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        Desc.Texture2D.MipSlice   = ViewDesc.Texture2D.Mip;
        Desc.Texture2D.PlaneSlice = 0;
    }
    else if (ViewDesc.Type == SRHIUnorderedAccessViewDesc::EType::Texture2DArray)
    {
        CD3D12Texture*  DxTexture = D3D12TextureCast(ViewDesc.Texture2DArray.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture2DArray.Texture->IsUAV() && ViewDesc.Texture2DArray.Format != ERHIFormat::Unknown);

        Desc.Format                         = ConvertFormat(ViewDesc.Texture2DArray.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = ViewDesc.Texture2DArray.Mip;
        Desc.Texture2DArray.ArraySize       = ViewDesc.Texture2DArray.NumArraySlices;
        Desc.Texture2DArray.FirstArraySlice = ViewDesc.Texture2DArray.ArraySlice;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (ViewDesc.Type == SRHIUnorderedAccessViewDesc::EType::TextureCube)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.TextureCube.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.TextureCube.Texture->IsUAV() && ViewDesc.TextureCube.Format != ERHIFormat::Unknown);

        Desc.Format                         = ConvertFormat(ViewDesc.TextureCube.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = ViewDesc.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = 0;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (ViewDesc.Type == SRHIUnorderedAccessViewDesc::EType::TextureCubeArray)
    {
        CD3D12Texture*    DxTexture = D3D12TextureCast(ViewDesc.TextureCubeArray.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.TextureCubeArray.Texture->IsUAV() && ViewDesc.TextureCubeArray.Format != ERHIFormat::Unknown);

        Desc.Format                         = ConvertFormat(ViewDesc.TextureCubeArray.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = ViewDesc.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = ViewDesc.TextureCubeArray.NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = ViewDesc.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (ViewDesc.Type == SRHIUnorderedAccessViewDesc::EType::Texture3D)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture3D.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture3D.Texture->IsUAV() && ViewDesc.Texture3D.Format != ERHIFormat::Unknown);

        Desc.Format                = ConvertFormat(ViewDesc.Texture3D.Format);
        Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = ViewDesc.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = ViewDesc.Texture3D.DepthSlice;
        Desc.Texture3D.WSize       = ViewDesc.Texture3D.NumDepthSlices;
    }
    else if (ViewDesc.Type == SRHIUnorderedAccessViewDesc::EType::VertexBuffer)
    {
        CD3D12Buffer* DxBuffer = static_cast<CD3D12Buffer*>(ViewDesc.VertexBuffer.Buffer);
        Resource = DxBuffer->GetResource();

        Assert(DxBuffer->IsUAV());

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = ViewDesc.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements         = ViewDesc.VertexBuffer.NumVertices;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = DxBuffer->GetStride();
    }
    else if (ViewDesc.Type == SRHIUnorderedAccessViewDesc::EType::IndexBuffer)
    {
        CD3D12Buffer* DxBuffer = static_cast<CD3D12Buffer*>(ViewDesc.IndexBuffer.Buffer);
        Resource = DxBuffer->GetResource();

        Assert(DxBuffer->IsUAV());

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = ViewDesc.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements         = ViewDesc.IndexBuffer.NumIndices;
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (ViewDesc.Type == SRHIUnorderedAccessViewDesc::EType::StructuredBuffer)
    {
        CD3D12Buffer* DxBuffer = static_cast<CD3D12Buffer*>(ViewDesc.StructuredBuffer.Buffer);
        Resource = DxBuffer->GetResource();

        Assert(DxBuffer->IsUAV());

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = ViewDesc.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = ViewDesc.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = DxBuffer->GetStride();
    }

    TSharedRef<CD3D12UnorderedAccessView> DxView = dbg_new CD3D12UnorderedAccessView(GetDevice(), ResourceOfflineDescriptorHeap);
    if (!DxView->AllocateHandle())
    {
        return nullptr;
    }

    Assert(Resource != nullptr);

    // TODO: Expose counter-resource
    if (DxView->CreateView(nullptr, Resource, Desc))
    {
        return DxView.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHIRenderTargetView* CRHIInstanceD3D12::CreateRenderTargetView(const SRHIRenderTargetViewDesc& ViewDesc)
{
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat(ViewDesc.Format);
    Assert(ViewDesc.Format != ERHIFormat::Unknown);

    if (ViewDesc.Type == SRHIRenderTargetViewDesc::EType::Texture2D)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture2D.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture2D.Texture->IsRTV());

        if (ViewDesc.Texture2D.Texture->IsMultiSampled())
        {
            Desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice   = ViewDesc.Texture2D.Mip;
            Desc.Texture2D.PlaneSlice = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (ViewDesc.Type == SRHIRenderTargetViewDesc::EType::Texture2DArray)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture2DArray.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture2DArray.Texture->IsRTV());

        if (ViewDesc.Texture2DArray.Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = ViewDesc.Texture2DArray.Mip;
            Desc.Texture2DArray.ArraySize       = ViewDesc.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice = ViewDesc.Texture2DArray.ArraySlice;
            Desc.Texture2DArray.PlaneSlice      = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = ViewDesc.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = ViewDesc.Texture2DArray.ArraySlice;
        }
    }
    else if (ViewDesc.Type == SRHIRenderTargetViewDesc::EType::TextureCube)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.TextureCube.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.TextureCube.Texture->IsRTV());

        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = ViewDesc.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex(ViewDesc.TextureCube.CubeFace);
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (ViewDesc.Type == SRHIRenderTargetViewDesc::EType::TextureCubeArray)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.TextureCubeArray.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.TextureCubeArray.Texture->IsRTV());

        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = ViewDesc.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = ViewDesc.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex(ViewDesc.TextureCube.CubeFace);
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (ViewDesc.Type == SRHIRenderTargetViewDesc::EType::Texture3D)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture3D.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture3D.Texture->IsRTV());

        Desc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = ViewDesc.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = ViewDesc.Texture3D.DepthSlice;
        Desc.Texture3D.WSize       = ViewDesc.Texture3D.NumDepthSlices;
    }

    TSharedRef<CD3D12RenderTargetView> DxView = dbg_new CD3D12RenderTargetView(GetDevice(), RenderTargetOfflineDescriptorHeap);
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

CRHIDepthStencilView* CRHIInstanceD3D12::CreateDepthStencilView(const SRHIDepthStencilViewDesc& ViewDesc)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat(ViewDesc.Format);
    Assert(ViewDesc.Format != ERHIFormat::Unknown);

    if (ViewDesc.Type == SRHIDepthStencilViewDesc::EType::Texture2D)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture2D.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture2D.Texture->IsDSV());

        if (ViewDesc.Texture2D.Texture->IsMultiSampled())
        {
            Desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice = ViewDesc.Texture2D.Mip;
        }
        else
        {
            Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (ViewDesc.Type == SRHIDepthStencilViewDesc::EType::Texture2DArray)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.Texture2DArray.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.Texture2DArray.Texture->IsDSV());

        if (ViewDesc.Texture2DArray.Texture->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = ViewDesc.Texture2DArray.Mip;
            Desc.Texture2DArray.ArraySize       = ViewDesc.Texture2DArray.NumArraySlices;
            Desc.Texture2DArray.FirstArraySlice = ViewDesc.Texture2DArray.ArraySlice;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.ArraySize       = ViewDesc.Texture2DArray.NumArraySlices;
            Desc.Texture2DMSArray.FirstArraySlice = ViewDesc.Texture2DArray.ArraySlice;
        }
    }
    else if (ViewDesc.Type == SRHIDepthStencilViewDesc::EType::TextureCube)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.TextureCube.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.TextureCube.Texture->IsDSV());

        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = ViewDesc.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex(ViewDesc.TextureCube.CubeFace);
    }
    else if (ViewDesc.Type == SRHIDepthStencilViewDesc::EType::TextureCubeArray)
    {
        CD3D12Texture* DxTexture = D3D12TextureCast(ViewDesc.TextureCubeArray.Texture);
        Resource = DxTexture->GetResource();

        Assert(ViewDesc.TextureCubeArray.Texture->IsDSV());

        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = ViewDesc.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = ViewDesc.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex(ViewDesc.TextureCube.CubeFace);
    }

    TSharedRef<CD3D12DepthStencilView> DxView = dbg_new CD3D12DepthStencilView(GetDevice(), DepthStencilOfflineDescriptorHeap);
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

CRHIComputeShader* CRHIInstanceD3D12::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12ComputeShader> Shader = dbg_new CD3D12ComputeShader(GetDevice(), ShaderCode);
    if (!Shader->Init())
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIVertexShader* CRHIInstanceD3D12::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12VertexShader> Shader = dbg_new CD3D12VertexShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseShader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIHullShader* CRHIInstanceD3D12::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIDomainShader* CRHIInstanceD3D12::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIGeometryShader* CRHIInstanceD3D12::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIMeshShader* CRHIInstanceD3D12::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIAmplificationShader* CRHIInstanceD3D12::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIPixelShader* CRHIInstanceD3D12::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12PixelShader> Shader = dbg_new CD3D12PixelShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseShader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIRayGenShader* CRHIInstanceD3D12::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayGenShader> Shader = dbg_new CD3D12RayGenShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        D3D12_ERROR_ALWAYS("Failed to retrive Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayAnyHitShader* CRHIInstanceD3D12::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayAnyHitShader> Shader = dbg_new CD3D12RayAnyHitShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        D3D12_ERROR_ALWAYS("Failed to retrive Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayClosestHitShader* CRHIInstanceD3D12::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayClosestHitShader> Shader = dbg_new CD3D12RayClosestHitShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        D3D12_ERROR_ALWAYS("Failed to retrive Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayMissShader* CRHIInstanceD3D12::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayMissShader> Shader = dbg_new CD3D12RayMissShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        D3D12_ERROR_ALWAYS("Failed to retrive Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIDepthStencilState* CRHIInstanceD3D12::CreateDepthStencilState(const SRHIDepthStencilStateDesc& CreateInfo)
{
    D3D12_DEPTH_STENCIL_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.DepthEnable      = CreateInfo.bDepthEnable;
    Desc.DepthFunc        = ConvertComparisonFunc(CreateInfo.DepthFunc);
    Desc.DepthWriteMask   = ConvertDepthWriteMask(CreateInfo.DepthWriteMask);
    Desc.StencilEnable    = CreateInfo.bStencilEnable;
    Desc.StencilReadMask  = CreateInfo.StencilReadMask;
    Desc.StencilWriteMask = CreateInfo.StencilWriteMask;
    Desc.FrontFace        = ConvertDepthStencilOp(CreateInfo.FrontFace);
    Desc.BackFace         = ConvertDepthStencilOp(CreateInfo.BackFace);

    return dbg_new CD3D12DepthStencilState(GetDevice(), Desc);
}

CRHIRasterizerState* CRHIInstanceD3D12::CreateRasterizerState(const SRHIRasterizerStateDesc& CreateInfo)
{
    D3D12_RASTERIZER_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.AntialiasedLineEnable = CreateInfo.bAntialiasedLineEnable;
    Desc.ConservativeRaster    = (CreateInfo.bEnableConservativeRaster) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    Desc.CullMode              = ConvertCullMode(CreateInfo.CullMode);
    Desc.DepthBias             = CreateInfo.DepthBias;
    Desc.DepthBiasClamp        = CreateInfo.DepthBiasClamp;
    Desc.DepthClipEnable       = CreateInfo.bDepthClipEnable;
    Desc.SlopeScaledDepthBias  = CreateInfo.SlopeScaledDepthBias;
    Desc.FillMode              = ConvertFillMode(CreateInfo.FillMode);
    Desc.ForcedSampleCount     = CreateInfo.ForcedSampleCount;
    Desc.FrontCounterClockwise = CreateInfo.bFrontCounterClockwise;
    Desc.MultisampleEnable     = CreateInfo.bMultisampleEnable;

    return dbg_new CD3D12RasterizerState(GetDevice(), Desc);
}

CRHIBlendState* CRHIInstanceD3D12::CreateBlendState(const SRHIBlendStateDesc& CreateInfo)
{
    D3D12_BLEND_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.AlphaToCoverageEnable  = CreateInfo.bAlphaToCoverageEnable;
    Desc.IndependentBlendEnable = CreateInfo.bIndependentBlendEnable;
    for (uint32 i = 0; i < 8; i++)
    {
        Desc.RenderTarget[i].BlendEnable           = CreateInfo.RenderTarget[i].bBlendEnable;
        Desc.RenderTarget[i].BlendOp               = ConvertBlendOp(CreateInfo.RenderTarget[i].BlendOp);
        Desc.RenderTarget[i].BlendOpAlpha          = ConvertBlendOp(CreateInfo.RenderTarget[i].BlendOpAlpha);
        Desc.RenderTarget[i].DestBlend             = ConvertBlend(CreateInfo.RenderTarget[i].DestBlend);
        Desc.RenderTarget[i].DestBlendAlpha        = ConvertBlend(CreateInfo.RenderTarget[i].DestBlendAlpha);
        Desc.RenderTarget[i].SrcBlend              = ConvertBlend(CreateInfo.RenderTarget[i].SrcBlend);
        Desc.RenderTarget[i].SrcBlendAlpha         = ConvertBlend(CreateInfo.RenderTarget[i].SrcBlendAlpha);
        Desc.RenderTarget[i].LogicOpEnable         = CreateInfo.RenderTarget[i].bLogicOpEnable;
        Desc.RenderTarget[i].LogicOp               = ConvertLogicOp(CreateInfo.RenderTarget[i].LogicOp);
        Desc.RenderTarget[i].RenderTargetWriteMask = ConvertRenderTargetWriteState(CreateInfo.RenderTarget[i].RenderTargetWriteMask);
    }

    return dbg_new CD3D12BlendState(GetDevice(), Desc);
}

CRHIInputLayoutState* CRHIInstanceD3D12::CreateInputLayout(const SRHIInputLayoutStateDesc& CreateInfo)
{
    return dbg_new CD3D12InputLayoutState(GetDevice(), CreateInfo);
}

CRHIGraphicsPipelineState* CRHIInstanceD3D12::CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& CreateInfo)
{
    TSharedRef<CD3D12GraphicsPipelineState> NewPipelineState = dbg_new CD3D12GraphicsPipelineState(GetDevice());
    if (!NewPipelineState->Initialize(CreateInfo))
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIComputePipelineState* CRHIInstanceD3D12::CreateComputePipelineState(const SRHIComputePipelineStateDesc& Info)
{
    Assert(Info.Shader != nullptr);

    TSharedRef<CD3D12ComputeShader> Shader = MakeSharedRef<CD3D12ComputeShader>(Info.Shader);
    TSharedRef<CD3D12ComputePipelineState> NewPipelineState = dbg_new CD3D12ComputePipelineState(GetDevice(), Shader);
    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIRayTracingPipelineState* CRHIInstanceD3D12::CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& CreateInfo)
{
    TSharedRef<CD3D12RayTracingPipelineState> NewPipelineState = dbg_new CD3D12RayTracingPipelineState(GetDevice());
    if (NewPipelineState->Initialize(CreateInfo))
    {
        return NewPipelineState.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHITimestampQuery* CRHIInstanceD3D12::CreateTimestampQuery()
{
    return CD3D12TimestampQuery::Create(GetDevice());
}

CRHIViewportRef CRHIInstanceD3D12::CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat)
{
    UNREFERENCED_VARIABLE(DepthFormat);

    // TODO: Take DepthFormat into account
    TSharedRef<CD3D12Viewport> Viewport = dbg_new CD3D12Viewport(GetDevice(), DirectCommandContext.Get(), reinterpret_cast<HWND>(WindowHandle), ColorFormat, Width, Height);
    if (Viewport && Viewport->Initialize())
    {
        return Viewport;
    }

    return nullptr;
}

bool CRHIInstanceD3D12::UAVSupportsFormat(ERHIFormat Format) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
    CMemory::Memzero(&FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

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

void CRHIInstanceD3D12::CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const
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

void CRHIInstanceD3D12::CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const
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

    OutSupport.ShadingRateImageTileSize = Device->GetVariableRateShadingTileSize();
}
