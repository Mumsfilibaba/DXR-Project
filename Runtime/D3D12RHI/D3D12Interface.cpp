#include "D3D12CommandList.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Fence.h"
#include "D3D12RootSignature.h"
#include "D3D12Core.h"
#include "D3D12Interface.h"
#include "D3D12ResourceViews.h"
#include "D3D12RayTracing.h"
#include "D3D12PipelineState.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12SamplerState.h"
#include "D3D12Viewport.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12TimestampQuery.h"
#include "DynamicD3D12.h"

#include "Core/Containers/ITextureResourceData.h"
#include "Core/Debug/Console/ConsoleInterface.h"

#include "CoreApplication/Windows/WindowsWindow.h"

TAutoConsoleVariable<bool> CVarEnablePix("D3D12RHI.EnablePIX", false);

FD3D12Interface* FD3D12Interface::GD3D12Interface = nullptr;

FD3D12Interface::FD3D12Interface()
    : FRHIInterface(ERHIInstanceType::D3D12)
    , Device(nullptr)
    , DirectContext(nullptr)
{
    if (!GD3D12Interface)
    {
        GD3D12Interface = this;
    }
}

FD3D12Interface::~FD3D12Interface()
{
    SAFE_DELETE(DirectContext);

    GenerateMipsTex2D_PSO.Reset();
    GenerateMipsTexCube_PSO.Reset();

    SAFE_RELEASE(ResourceOfflineDescriptorHeap);
    SAFE_RELEASE(RenderTargetOfflineDescriptorHeap);
    SAFE_RELEASE(DepthStencilOfflineDescriptorHeap);
    SAFE_RELEASE(SamplerOfflineDescriptorHeap);

    Device.Reset();
    Adapter.Reset();

    FDynamicD3D12::Release();

    if (GD3D12Interface == this)
    {
        GD3D12Interface = nullptr;
    }
}

bool FD3D12Interface::Initialize()
{
    const bool bEnablePIX = CVarEnablePix.GetBool();

    // Load Library and Function-Pointers etc.
    const bool bResult = FDynamicD3D12::Initialize(bEnablePIX);
    if (!bResult)
    {
        return false;
    }

    Adapter = dbg_new FD3D12Adapter();
    if (!Adapter->Initialize())
    {
        return false;
    }

    Device = dbg_new FD3D12Device(Adapter.Get());
    if (!Device->Initialize())
    {
        return false;
    }

    // Initialize Offline Descriptor heaps
    ResourceOfflineDescriptorHeap = dbg_new FD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!ResourceOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    RenderTargetOfflineDescriptorHeap = dbg_new FD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if (!RenderTargetOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    DepthStencilOfflineDescriptorHeap = dbg_new FD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    if (!DepthStencilOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    SamplerOfflineDescriptorHeap = dbg_new FD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!SamplerOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    // Initialize shader compiler
    GD3D12ShaderCompiler = dbg_new FD3D12ShaderCompiler();
    if (!GD3D12ShaderCompiler->Init())
    {
        return false;
    }

    // Initialize GenerateMips Shaders and pipeline states 
    TArray<uint8> Code;

    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTex2D.hlsl", CompileInfo, Code))
        {
            D3D12_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTex2D Shader");
            return false;
        }
    }

    TSharedRef<FD3D12ComputeShader> Shader = dbg_new FD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to Create ComputeShader");
        return false;
    }

    GenerateMipsTex2D_PSO = dbg_new FD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTex2D_PSO->Initialize())
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTex2D PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTex2D_PSO->SetName("GenerateMipsTex2D Gen PSO");
    }

    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTexCube.hlsl", CompileInfo, Code))
        {
            D3D12_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTexCube Shader");
            return false;
        }
    }

    Shader = dbg_new FD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        DEBUG_BREAK();
        return false;
    }

    GenerateMipsTexCube_PSO = dbg_new FD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTexCube_PSO->Initialize())
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTexCube PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTexCube_PSO->SetName("GenerateMipsTexCube Gen PSO");
    }

    // Initialize context
    DirectContext = dbg_new FD3D12CommandContext(GetDevice(), ED3D12CommandQueueType::Direct);
    if (!(DirectContext && DirectContext->Initialize()))
    {
        return false;
    }

    return true;
}

template<typename D3D12TextureType, typename InitializerType>
D3D12TextureType* FD3D12Interface::CreateTexture(const InitializerType& Initializer)
{
    TSharedRef<D3D12TextureType> NewTexture = dbg_new D3D12TextureType(GetDevice(), Initializer);

    D3D12_RESOURCE_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Dimension        = GetD3D12TextureResourceDimension<D3D12TextureType>();
    Desc.Flags            = ConvertTextureFlags(Initializer.UsageFlags);
    Desc.Format           = ConvertFormat(Initializer.Format);
    Desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.MipLevels        = static_cast<UINT16>(Initializer.NumMips);
    Desc.Alignment        = 0;
    
    const FIntVector3 Extent = NewTexture->GetExtent();
    Desc.Width            = Extent.x;
    Desc.Height           = Extent.y;
    Desc.DepthOrArraySize = GetDepthOrArraySize<D3D12TextureType>(Extent.z);

    const uint32 NumSamples = NewTexture->GetNumSamples();
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

    D3D12_CLEAR_VALUE* ClearValue = nullptr;
    D3D12_CLEAR_VALUE  D3D12ClearValue;
    if (Initializer.AllowRTV() || Initializer.AllowDSV())
    {
        FMemory::Memzero(&D3D12ClearValue);
        ClearValue = &D3D12ClearValue;

        D3D12ClearValue.Format = (Initializer.ClearValue.Format != EFormat::Unknown) ? ConvertFormat(Initializer.ClearValue.Format) : Desc.Format;
        if (Initializer.ClearValue.IsDepthStencilValue())
        {
            D3D12ClearValue.DepthStencil.Depth   = Initializer.ClearValue.AsDepthStencil().Depth;
            D3D12ClearValue.DepthStencil.Stencil = Initializer.ClearValue.AsDepthStencil().Stencil;
        }
        else if (Initializer.ClearValue.IsColorValue())
        {
            FMemory::Memcpy(D3D12ClearValue.Color, Initializer.ClearValue.AsColor().GetData(), sizeof(float[4]));
        }
    }

    FD3D12ResourceRef Resource = dbg_new FD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
    if (!Resource->Initialize(D3D12_RESOURCE_STATE_COMMON, ClearValue))
    {
        return nullptr;
    }
    else
    {
        NewTexture->SetResource(Resource.ReleaseOwnership());
    }

    if (Initializer.AllowDefaultSRV())
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc;
        FMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format                  = CastShaderResourceFormat(Desc.Format);
        ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if constexpr (TIsSame<D3D12TextureType, FD3D12TextureCubeArray>::Value)
        {
            ViewDesc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            ViewDesc.TextureCubeArray.MipLevels           = Initializer.NumMips;
            ViewDesc.TextureCubeArray.MostDetailedMip     = 0;
            ViewDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.TextureCubeArray.First2DArrayFace    = 0;
            ViewDesc.TextureCubeArray.NumCubes            = Extent.z;
        }
        else if constexpr (TIsSame<D3D12TextureType, FD3D12TextureCube>::Value)
        {
            ViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
            ViewDesc.TextureCube.MipLevels           = Initializer.NumMips;
            ViewDesc.TextureCube.MostDetailedMip     = 0;
            ViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        }
        else if constexpr (TIsSame<D3D12TextureType, FD3D12Texture2DArray>::Value)
        {
            ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            ViewDesc.Texture2DArray.MipLevels           = Initializer.NumMips;
            ViewDesc.Texture2DArray.MostDetailedMip     = 0;
            ViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture2DArray.PlaneSlice          = 0;
            ViewDesc.Texture2DArray.ArraySize           = Extent.z;
            ViewDesc.Texture2DArray.FirstArraySlice     = 0;
        }
        else if constexpr (TIsSame<D3D12TextureType, FD3D12Texture2D>::Value)
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipLevels           = Initializer.NumMips;
            ViewDesc.Texture2D.MostDetailedMip     = 0;
            ViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture2D.PlaneSlice          = 0;
        }
        else if constexpr (TIsSame<D3D12TextureType, FD3D12Texture3D>::Value)
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture3D.MipLevels           = Initializer.NumMips;
            ViewDesc.Texture3D.MostDetailedMip     = 0;
            ViewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            D3D12_ERROR("Unsupported resource dimension");
            return nullptr;
        }

        FD3D12ShaderResourceViewRef DefaultSRV = dbg_new FD3D12ShaderResourceView(GetDevice(), ResourceOfflineDescriptorHeap, NewTexture.Get());
        if (!DefaultSRV->AllocateHandle())
        {
            return nullptr;
        }

        if (!DefaultSRV->CreateView(NewTexture->GetResource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture->SetShaderResourceView(DefaultSRV.ReleaseOwnership());
    }

    // TODO: Fix for other resources than Texture2D
    constexpr bool bIsTexture2D = TIsSame<D3D12TextureType, FD3D12Texture2D>::Value;

    if constexpr (bIsTexture2D)
    {
        if (Initializer.AllowDefaultUAV())
        {
            FD3D12Texture2D* NewTexture2D = static_cast<FD3D12Texture2D*>(NewTexture->GetTexture2D());

            D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
            FMemory::Memzero(&ViewDesc);

            // TODO: Handle typeless
            ViewDesc.Format               = Desc.Format;
            ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipSlice   = 0;
            ViewDesc.Texture2D.PlaneSlice = 0;

            FD3D12UnorderedAccessViewRef DefaultUAV = dbg_new FD3D12UnorderedAccessView(GetDevice(), ResourceOfflineDescriptorHeap, NewTexture2D);
            if (!DefaultUAV->AllocateHandle())
            {
                return nullptr;
            }

            if (!DefaultUAV->CreateView(nullptr, NewTexture->GetResource(), ViewDesc))
            {
                return nullptr;
            }

            NewTexture2D->SetUnorderedAccessView(DefaultUAV.ReleaseOwnership());
        }
    }

    if constexpr (bIsTexture2D)
    {
        ITextureResourceData* InitialData = Initializer.InitialData;
        if (InitialData)
        {
            // TODO: Support other types than texture 2D

            FRHITexture2D* Texture2D = NewTexture->GetTexture2D();
            D3D12_ERROR_COND(Texture2D != nullptr, "Texture was unexpectedly nullptr");

            DirectContext->StartContext();
            DirectContext->TransitionTexture(Texture2D, EResourceAccess::Common, EResourceAccess::CopyDest);

            // Transfer all the mip-levels
            uint32 Width  = Extent.x;
            uint32 Height = Extent.y;
            for (uint32 Index = 0; Index < Texture2D->GetNumMips(); ++Index)
            {
                DirectContext->UpdateTexture2D(
                    Texture2D, 
                    Width,
                    Height,
                    Index, 
                    InitialData->GetMipData(Index), 
                    (uint32)InitialData->GetMipRowPitch(Index));

                Width  = Width / 2;
                Height = Height / 2;
            }

            // NOTE: Transition into InitialAccess
            DirectContext->TransitionTexture(Texture2D, EResourceAccess::CopyDest, Initializer.InitialAccess);
            DirectContext->FinishContext();

            return NewTexture.ReleaseOwnership();
        }
    }

    if (Initializer.InitialAccess != EResourceAccess::Common)
    {
        DirectContext->StartContext();
        DirectContext->TransitionTexture(NewTexture.Get(), EResourceAccess::Common, Initializer.InitialAccess);
        DirectContext->FinishContext();
    }

    return NewTexture.ReleaseOwnership();
}

FRHITexture2D* FD3D12Interface::RHICreateTexture2D(const FRHITexture2DInitializer& Initializer)
{
    return CreateTexture<FD3D12Texture2D>(Initializer);
}

FRHITexture2DArray* FD3D12Interface::RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer)
{
    return CreateTexture<FD3D12Texture2DArray>(Initializer);
}

FRHITextureCube* FD3D12Interface::RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer)
{
    return CreateTexture<FD3D12TextureCube>(Initializer);
}

FRHITextureCubeArray* FD3D12Interface::RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer)
{
    return CreateTexture<FD3D12TextureCubeArray>(Initializer);
}

FRHITexture3D* FD3D12Interface::RHICreateTexture3D(const FRHITexture3DInitializer& Initializer)
{
    return CreateTexture<FD3D12Texture3D>(Initializer);
}

FRHISamplerState* FD3D12Interface::RHICreateSamplerState(const FRHISamplerStateInitializer& Initializer)
{
    D3D12_SAMPLER_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.AddressU       = ConvertSamplerMode(Initializer.AddressU);
    Desc.AddressV       = ConvertSamplerMode(Initializer.AddressV);
    Desc.AddressW       = ConvertSamplerMode(Initializer.AddressW);
    Desc.ComparisonFunc = ConvertComparisonFunc(Initializer.ComparisonFunc);
    Desc.Filter         = ConvertSamplerFilter(Initializer.Filter);
    Desc.MaxAnisotropy  = Initializer.MaxAnisotropy;
    Desc.MaxLOD         = Initializer.MaxLOD;
    Desc.MinLOD         = Initializer.MinLOD;
    Desc.MipLODBias     = Initializer.MipLODBias;

    FMemory::Memcpy(Desc.BorderColor, Initializer.BorderColor.GetData(), sizeof(Desc.BorderColor));

    FD3D12SamplerStateRef Sampler = dbg_new FD3D12SamplerState(GetDevice(), SamplerOfflineDescriptorHeap, Initializer);
    if (!Sampler->CreateSampler(Desc))
    {
        return nullptr;
    }
    else
    {
        return Sampler.ReleaseOwnership();
    }
}

FRHIBuffer* FD3D12Interface::RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    FD3D12BufferRef NewBuffer = dbg_new FD3D12Buffer(GetDevice(), InDesc);

    const uint64 Alignment   = GetBufferAlignment(InDesc.UsageFlags);
    const uint64 AlignedSize = NMath::AlignUp(InDesc.Size, Alignment);

    D3D12_RESOURCE_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = ConvertBufferFlags(InDesc.UsageFlags);
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = AlignedSize;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    D3D12_HEAP_TYPE       D3D12HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES D3D12InitialState = D3D12_RESOURCE_STATE_COMMON;
    if (InDesc.IsDynamic())
    {
        D3D12HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        D3D12InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    // Limit the scope of the new resource
    {
        FD3D12ResourceRef D3D12Resource = dbg_new FD3D12Resource(GetDevice(), Desc, D3D12HeapType);
        if (!D3D12Resource->Initialize(D3D12InitialState, nullptr))
        {
            return nullptr;
        }
        else
        {
            NewBuffer->SetResource(D3D12Resource.ReleaseOwnership());
        }
    }

    if (InInitialData)
    {
        if (InDesc.IsDynamic())
        {
            FD3D12Resource* D3D12Resource = NewBuffer->GetD3D12Resource();

            void* BufferData = D3D12Resource->MapRange(0, 0);
            if (!BufferData)
            {
                return nullptr;
            }

            // Copy over relevant data
            FMemory::Memcpy(BufferData, InInitialData, InDesc.Size);

            D3D12Resource->UnmapRange(0, 0);
        }
        else
        {
            DirectContext->StartContext();

            DirectContext->TransitionBuffer(NewBuffer.Get(), EResourceAccess::Common, EResourceAccess::CopyDest);
            DirectContext->UpdateBuffer(NewBuffer.Get(), 0, InDesc.Size, InInitialData);

            // NOTE: Transfer to the initial state
            if (InInitialState != EResourceAccess::CopyDest)
            {
                DirectContext->TransitionBuffer(NewBuffer.Get(), EResourceAccess::CopyDest, InInitialState);
            }

            DirectContext->FinishContext();
        }
    }
    else
    {
        if (InInitialState != EResourceAccess::Common && InDesc.IsDynamic())
        {
            DirectContext->StartContext();
            DirectContext->TransitionBuffer(NewBuffer.Get(), EResourceAccess::Common, InInitialState);
            DirectContext->FinishContext();
        }
    }

    return NewBuffer.ReleaseOwnership();
}

FRHIRayTracingGeometry* FD3D12Interface::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
{
    FD3D12Buffer* D3D12VertexBuffer = static_cast<FD3D12Buffer*>(Initializer.VertexBuffer);
    FD3D12Buffer* D3D12IndexBuffer  = static_cast<FD3D12Buffer*>(Initializer.IndexBuffer);

    TSharedRef<FD3D12RayTracingGeometry> D3D12Geometry = dbg_new FD3D12RayTracingGeometry(GetDevice(), Initializer);
    
    DirectContext->StartContext();

    if (!D3D12Geometry->Build(
        *DirectContext,
        D3D12VertexBuffer,
        Initializer.NumVertices,
        D3D12IndexBuffer,
        Initializer.NumIndices,
        Initializer.IndexFormat,
        false))
    {
        DEBUG_BREAK();
        D3D12Geometry.Reset();
    }
    
    DirectContext->FinishContext();

    return D3D12Geometry.ReleaseOwnership();
}

FRHIRayTracingScene* FD3D12Interface::RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
{
    TSharedRef<FD3D12RayTracingScene> D3D12Scene = dbg_new FD3D12RayTracingScene(GetDevice(), Initializer);

    DirectContext->StartContext();

    if (!D3D12Scene->Build(*DirectContext, Initializer.Instances.CreateView(), false))
    {
        DEBUG_BREAK();
        D3D12Scene.Reset();
    }

    DirectContext->FinishContext();

    return D3D12Scene.ReleaseOwnership();
}

FRHIShaderResourceView* FD3D12Interface::RHICreateShaderResourceView(const FRHITextureSRVInitializer& Initializer)
{
    D3D12_ERROR_COND(Initializer.Texture != nullptr, "Texture cannot be nullptr");

    CHECK(IsEnumFlagSet(Initializer.Texture->GetFlags(), ETextureUsageFlags::AllowSRV) && (Initializer.Format != EFormat::Unknown));
    
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Format                  = ConvertFormat(Initializer.Format);
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    if (FD3D12Texture2D* Texture2D = static_cast<FD3D12Texture2D*>(Initializer.Texture->GetTexture2D()))
    {
        if (!Texture2D->IsMultiSampled())
        {
            Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MostDetailedMip     = Initializer.FirstMipLevel;
            Desc.Texture2D.MipLevels           = Initializer.NumMips;
            Desc.Texture2D.ResourceMinLODClamp = Initializer.MinLODClamp;
            Desc.Texture2D.PlaneSlice          = 0;
        }
        else
        {
            Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
    }
    else if (FD3D12Texture2DArray* Texture2DArray = static_cast<FD3D12Texture2DArray*>(Initializer.Texture->GetTexture2DArray()))
    {
        if (!Texture2DArray->IsMultiSampled())
        {
            Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MostDetailedMip     = Initializer.FirstMipLevel;
            Desc.Texture2DArray.MipLevels           = Initializer.NumMips;
            Desc.Texture2DArray.ResourceMinLODClamp = Initializer.MinLODClamp;
            Desc.Texture2DArray.FirstArraySlice     = Initializer.FirstArraySlice;
            Desc.Texture2DArray.ArraySize           = Initializer.NumSlices;
            Desc.Texture2DArray.PlaneSlice          = 0;
        }
        else
        {
            Desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            Desc.Texture2DMSArray.FirstArraySlice = Initializer.FirstArraySlice;
            Desc.Texture2DMSArray.ArraySize       = Initializer.NumSlices;
        }
    }
    else if (FD3D12TextureCube* TextureCube = static_cast<FD3D12TextureCube*>(Initializer.Texture->GetTextureCube()))
    {
        Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MostDetailedMip     = Initializer.FirstMipLevel;
        Desc.TextureCube.MipLevels           = Initializer.NumMips;
        Desc.TextureCube.ResourceMinLODClamp = Initializer.MinLODClamp;
    }
    else if (FD3D12TextureCubeArray* TextureCubeArray = static_cast<FD3D12TextureCubeArray*>(Initializer.Texture->GetTextureCubeArray()))
    {
        Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MostDetailedMip     = Initializer.FirstMipLevel;
        Desc.TextureCubeArray.MipLevels           = Initializer.NumMips;
        Desc.TextureCubeArray.ResourceMinLODClamp = Initializer.MinLODClamp;
        Desc.TextureCubeArray.First2DArrayFace    = GetDepthOrArraySize<FD3D12TextureCubeArray>(Initializer.FirstArraySlice);
        Desc.TextureCubeArray.NumCubes            = Initializer.NumSlices;
    }
    else if (FD3D12Texture3D* Texture3D = static_cast<FD3D12Texture3D*>(Initializer.Texture->GetTexture3D()))
    {
        Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MostDetailedMip     = Initializer.FirstMipLevel;
        Desc.Texture3D.MipLevels           = Initializer.NumMips;
        Desc.Texture3D.ResourceMinLODClamp = Initializer.MinLODClamp;
    }

    FD3D12ShaderResourceViewRef D3D12View = dbg_new FD3D12ShaderResourceView(GetDevice(), ResourceOfflineDescriptorHeap, Initializer.Texture);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = GetD3D12Resource(Initializer.Texture);
    CHECK(D3D12Resource != nullptr);

    if (D3D12View->CreateView(D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHIShaderResourceView* FD3D12Interface::RHICreateShaderResourceView(const FRHIBufferSRVInitializer& Initializer)
{
    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(Initializer.Buffer);
    if (!D3D12Buffer)
    {
        D3D12_ERROR("Cannot create a ShaderResourceView from a nullptr Buffer");
        return nullptr;
    }

    CHECK(IsEnumFlagSet(D3D12Buffer->GetFlags(), EBufferUsageFlags::ShaderResource));

    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    Desc.Buffer.FirstElement     = Initializer.FirstElement;
    Desc.Buffer.NumElements      = Initializer.NumElements;

    if (Initializer.Format == EBufferSRVFormat::None)
    {
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Initializer.Buffer->GetStride();
    }
    else
    {
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }

    FD3D12ShaderResourceViewRef D3D12View = dbg_new FD3D12ShaderResourceView(GetDevice(), ResourceOfflineDescriptorHeap, Initializer.Buffer);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = D3D12Buffer->GetD3D12Resource();
    CHECK(D3D12Resource != nullptr);

    if (D3D12View->CreateView(D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHIUnorderedAccessView* FD3D12Interface::RHICreateUnorderedAccessView(const FRHITextureUAVInitializer& Initializer)
{
    D3D12_ERROR_COND(Initializer.Texture != nullptr, "Texture cannot be nullptr");

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.Format = ConvertFormat(Initializer.Format);
    CHECK(IsEnumFlagSet(Initializer.Texture->GetFlags(), ETextureUsageFlags::AllowUAV) && (Initializer.Format != EFormat::Unknown));

    if (FD3D12Texture2D* Texture2D = static_cast<FD3D12Texture2D*>(Initializer.Texture->GetTexture2D()))
    {
        if (!Texture2D->IsMultiSampled())
        {
            Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            Desc.Texture2D.MipSlice   = Initializer.MipLevel;
            Desc.Texture2D.PlaneSlice = 0;
        }
        else
        {
            D3D12_ERROR("MultiSampled Textures is not supported");
        }
    }
    else if (FD3D12Texture2DArray* Texture2DArray = static_cast<FD3D12Texture2DArray*>(Initializer.Texture->GetTexture2DArray()))
    {
        if (!Texture2DArray->IsMultiSampled())
        {
            Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            Desc.Texture2DArray.MipSlice        = Initializer.MipLevel;
            Desc.Texture2DArray.PlaneSlice      = 0;
            Desc.Texture2DArray.FirstArraySlice = Initializer.FirstArraySlice;
            Desc.Texture2DArray.ArraySize       = Initializer.NumSlices;
        }
        else
        {
            D3D12_ERROR("MultiSampled Textures is not supported");
        }
    }
    else if (FD3D12TextureCube* TextureCube = static_cast<FD3D12TextureCube*>(Initializer.Texture->GetTextureCube()))
    {
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = Initializer.MipLevel;
        Desc.Texture2DArray.PlaneSlice      = 0;
        Desc.Texture2DArray.FirstArraySlice = Initializer.FirstArraySlice;
        Desc.Texture2DArray.ArraySize       = Initializer.NumSlices;
    }
    else if (FD3D12TextureCubeArray* TextureCubeArray = static_cast<FD3D12TextureCubeArray*>(Initializer.Texture->GetTextureCubeArray()))
    {
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = Initializer.MipLevel;
        Desc.Texture2DArray.PlaneSlice      = 0;
        Desc.Texture2DArray.FirstArraySlice = Initializer.FirstArraySlice;
        Desc.Texture2DArray.ArraySize       = Initializer.NumSlices;
    }
    else if (FD3D12Texture3D* Texture3D = static_cast<FD3D12Texture3D*>(Initializer.Texture->GetTexture3D()))
    {
        Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.FirstWSlice = Initializer.FirstArraySlice;
        Desc.Texture3D.WSize       = Initializer.NumSlices;
        Desc.Texture3D.MipSlice    = Initializer.MipLevel;
    }

    FD3D12UnorderedAccessViewRef D3D12View = dbg_new FD3D12UnorderedAccessView(GetDevice(), ResourceOfflineDescriptorHeap, Initializer.Texture);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = GetD3D12Resource(Initializer.Texture);
    CHECK(D3D12Resource != nullptr);

    if (D3D12View->CreateView(nullptr, D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHIUnorderedAccessView* FD3D12Interface::RHICreateUnorderedAccessView(const FRHIBufferUAVInitializer& Initializer)
{
    FD3D12Buffer* D3D12Buffer = GetD3D12Buffer(Initializer.Buffer);
    if (!D3D12Buffer)
    {
        D3D12_ERROR("Cannot create a UnorderedAccessView from a nullptr Buffer");
        return nullptr;
    }

    CHECK(D3D12Buffer->GetDesc().HasUnorderedAccess());

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    Desc.Buffer.FirstElement = Initializer.FirstElement;
    Desc.Buffer.NumElements  = Initializer.NumElements;

    if (Initializer.Format == EBufferUAVFormat::None)
    {
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Initializer.Buffer->GetStride();
    }
    else
    {
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }

    FD3D12UnorderedAccessViewRef D3D12View = dbg_new FD3D12UnorderedAccessView(GetDevice(), ResourceOfflineDescriptorHeap, Initializer.Buffer);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    FD3D12Resource* D3D12Resource = D3D12Buffer->GetD3D12Resource();
    CHECK(D3D12Resource != nullptr);

    if (D3D12View->CreateView(nullptr, D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHIComputeShader* FD3D12Interface::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12ComputeShader> Shader = dbg_new FD3D12ComputeShader(GetDevice(), ShaderCode);
    if (!Shader->Init())
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

FRHIVertexShader* FD3D12Interface::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12VertexShader> Shader = dbg_new FD3D12VertexShader(GetDevice(), ShaderCode);
    if (!FD3D12Shader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

FRHIHullShader* FD3D12Interface::RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIDomainShader* FD3D12Interface::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIGeometryShader* FD3D12Interface::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIMeshShader* FD3D12Interface::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIAmplificationShader* FD3D12Interface::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIPixelShader* FD3D12Interface::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12PixelShader> Shader = dbg_new FD3D12PixelShader(GetDevice(), ShaderCode);
    if (!FD3D12Shader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

FRHIRayGenShader* FD3D12Interface::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayGenShader> Shader = dbg_new FD3D12RayGenShader(GetDevice(), ShaderCode);
    if (!FD3D12RayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        D3D12_ERROR("[CD3D12CoreInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

FRHIRayAnyHitShader* FD3D12Interface::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayAnyHitShader> Shader = dbg_new FD3D12RayAnyHitShader(GetDevice(), ShaderCode);
    if (!FD3D12RayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        D3D12_ERROR("[CD3D12CoreInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

FRHIRayClosestHitShader* FD3D12Interface::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayClosestHitShader> Shader = dbg_new FD3D12RayClosestHitShader(GetDevice(), ShaderCode);
    if (!FD3D12RayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        D3D12_ERROR("[CD3D12CoreInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

FRHIRayMissShader* FD3D12Interface::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<FD3D12RayMissShader> Shader = dbg_new FD3D12RayMissShader(GetDevice(), ShaderCode);
    if (!FD3D12RayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        D3D12_ERROR("[CD3D12CoreInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FD3D12Interface::RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& Initializer)
{
    D3D12_DEPTH_STENCIL_DESC Desc;
    FMemory::Memzero(&Desc);

    Desc.DepthEnable      = Initializer.bDepthEnable;
    Desc.DepthFunc        = ConvertComparisonFunc(Initializer.DepthFunc);
    Desc.DepthWriteMask   = ConvertDepthWriteMask(Initializer.DepthWriteMask);
    Desc.StencilEnable    = Initializer.bStencilEnable;
    Desc.StencilReadMask  = Initializer.StencilReadMask;
    Desc.StencilWriteMask = Initializer.StencilWriteMask;
    Desc.FrontFace        = ConvertDepthStencilOp(Initializer.FrontFace);
    Desc.BackFace         = ConvertDepthStencilOp(Initializer.BackFace);

    return dbg_new FD3D12DepthStencilState(GetDevice(), Desc);
}

FRHIRasterizerState* FD3D12Interface::RHICreateRasterizerState(const FRHIRasterizerStateInitializer& Initializer)
{
    D3D12_RASTERIZER_DESC Desc;
    FMemory::Memzero(&Desc);

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

    return dbg_new FD3D12RasterizerState(GetDevice(), Desc);
}

FRHIBlendState* FD3D12Interface::RHICreateBlendState(const FRHIBlendStateInitializer& Initializer)
{
    D3D12_BLEND_DESC Desc;
    FMemory::Memzero(&Desc);

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

    return dbg_new FD3D12BlendState(GetDevice(), Desc);
}

FRHIVertexInputLayout* FD3D12Interface::RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
{
    return dbg_new FD3D12VertexInputLayout(GetDevice(), Initializer);
}

FRHIGraphicsPipelineState* FD3D12Interface::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    FD3D12GraphicsPipelineStateRef NewPipelineState = dbg_new FD3D12GraphicsPipelineState(GetDevice());
    if (!NewPipelineState->Initialize(Initializer))
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

FRHIComputePipelineState* FD3D12Interface::RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& Initializer)
{
    CHECK(Initializer.Shader != nullptr);

    TSharedRef<FD3D12ComputeShader>        Shader           = MakeSharedRef<FD3D12ComputeShader>(Initializer.Shader);
    FD3D12ComputePipelineStateRef NewPipelineState = dbg_new FD3D12ComputePipelineState(GetDevice(), Shader);
    if (!NewPipelineState->Initialize())
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

FRHIRayTracingPipelineState* FD3D12Interface::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer)
{
    FD3D12RayTracingPipelineStateRef NewPipelineState = dbg_new FD3D12RayTracingPipelineState(GetDevice());
    if (NewPipelineState->Initialize(Initializer))
    {
        return NewPipelineState.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

FRHITimestampQuery* FD3D12Interface::RHICreateTimestampQuery()
{
    return FD3D12TimestampQuery::Create(GetDevice());
}

FRHIViewport* FD3D12Interface::RHICreateViewport(const FRHIViewportInitializer& Initializer)
{
    FD3D12ViewportRef Viewport = dbg_new FD3D12Viewport(GetDevice(), DirectContext, Initializer);
    if (Viewport->Initialize())
    {
        return Viewport.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

bool FD3D12Interface::RHIQueryUAVFormatSupport(EFormat Format) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
    FMemory::Memzero(&FeatureData);

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

void FD3D12Interface::RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport) const
{
    FD3D12RayTracingDesc RayTracingDesc = Device->GetRayTracingDesc();
    if (RayTracingDesc.IsSupported())
    {
        if (RayTracingDesc.Tier == D3D12_RAYTRACING_TIER_1_1)
        {
            OutSupport.Tier = ERHIRayTracingTier::Tier1_1;
        }
        else if (RayTracingDesc.Tier == D3D12_RAYTRACING_TIER_1_0)
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

void FD3D12Interface::RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport) const
{
    FD3D12VariableRateShadingDesc VariableRateShadingDesc = Device->GetVariableRateShadingDesc();
    switch (VariableRateShadingDesc.Tier)
    {
        case D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED: OutSupport.Tier = ERHIShadingRateTier::NotSupported; break;
        case D3D12_VARIABLE_SHADING_RATE_TIER_1:             OutSupport.Tier = ERHIShadingRateTier::Tier1;        break;
        case D3D12_VARIABLE_SHADING_RATE_TIER_2:             OutSupport.Tier = ERHIShadingRateTier::Tier2;        break;
    }

    OutSupport.ShadingRateImageTileSize = static_cast<uint8>(VariableRateShadingDesc.ShadingRateImageTileSize);
}
