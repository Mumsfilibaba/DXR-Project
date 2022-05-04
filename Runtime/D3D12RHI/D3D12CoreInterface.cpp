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
        D3D12_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTex2D Shader");
        return false;
    }

    TSharedRef<CD3D12ComputeShader> Shader = dbg_new CD3D12ComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to Create ComputeShader");
        return false;
    }

    GenerateMipsTex2D_PSO = dbg_new CD3D12ComputePipelineState(GetDevice(), Shader);
    if (!GenerateMipsTex2D_PSO->Init())
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTex2D PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTex2D_PSO->SetName("GenerateMipsTex2D Gen PSO");
    }

    if (!GD3D12ShaderCompiler->CompileFromFile("../Runtime/Shaders/GenerateMipsTexCube.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        D3D12_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTexCube Shader");
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
        D3D12_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTexCube PipelineState");
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

template<typename D3D12TextureType, typename InitializerType>
D3D12TextureType* CD3D12CoreInterface::CreateTexture(const InitializerType& Initializer)
{
    TSharedRef<D3D12TextureType> NewTexture = dbg_new D3D12TextureType(GetDevice(), Initializer);

    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero(&Desc);

    Desc.Dimension        = GetD3D12TextureResourceDimension<D3D12TextureType>();
    Desc.Flags            = ConvertTextureFlags(Initializer.UsageFlags);
    Desc.Format           = ConvertFormat(Initializer.Format);
    Desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.MipLevels        = static_cast<UINT16>(Initializer.NumMips);
    Desc.Alignment        = 0;
    
    const CIntVector3 Extent = NewTexture->GetExtent();
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

    D3D12_CLEAR_VALUE* OptimizedClearValue = nullptr;
    
    D3D12_CLEAR_VALUE D3D12ClearValue;
    if (Initializer.AllowRTV() || Initializer.AllowDSV())
    {
        CMemory::Memzero(&D3D12ClearValue);
        OptimizedClearValue = &D3D12ClearValue;

        const auto& ClearValue = Initializer.ClearValue;
        D3D12ClearValue.Format = (ClearValue.Format != EFormat::Unknown) ? ConvertFormat(ClearValue.Format) : Desc.Format;
        if (ClearValue.IsDepthStencilValue())
        {
            D3D12ClearValue.DepthStencil.Depth   = ClearValue.AsDepthStencil().Depth;
            D3D12ClearValue.DepthStencil.Stencil = ClearValue.AsDepthStencil().Stencil;
        }
        else if (ClearValue.IsColorValue())
        {
            CMemory::Memcpy(D3D12ClearValue.Color, ClearValue.AsColor().Data(), sizeof(float[4]));
        }
    }

    TSharedRef<CD3D12Resource> Resource = dbg_new CD3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
    if (!Resource->Init(D3D12_RESOURCE_STATE_COMMON, OptimizedClearValue))
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
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format                  = CastShaderResourceFormat(Desc.Format);
        ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if constexpr (TIsSame<D3D12TextureType, CD3D12TextureCubeArray>::Value)
        {
            ViewDesc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            ViewDesc.TextureCubeArray.MipLevels           = Initializer.NumMips;
            ViewDesc.TextureCubeArray.MostDetailedMip     = 0;
            ViewDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.TextureCubeArray.First2DArrayFace    = 0;
            ViewDesc.TextureCubeArray.NumCubes            = Extent.z;
        }
        else if constexpr (TIsSame<D3D12TextureType, CD3D12TextureCube>::Value)
        {
            ViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
            ViewDesc.TextureCube.MipLevels           = Initializer.NumMips;
            ViewDesc.TextureCube.MostDetailedMip     = 0;
            ViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        }
        else if constexpr (TIsSame<D3D12TextureType, CD3D12Texture2DArray>::Value)
        {
            ViewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            ViewDesc.Texture2DArray.MipLevels           = Initializer.NumMips;
            ViewDesc.Texture2DArray.MostDetailedMip     = 0;
            ViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture2DArray.PlaneSlice          = 0;
            ViewDesc.Texture2DArray.ArraySize           = Extent.z;
            ViewDesc.Texture2DArray.FirstArraySlice     = 0;
        }
        else if constexpr (TIsSame<D3D12TextureType, CD3D12Texture2D>::Value)
        {
            ViewDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipLevels           = Initializer.NumMips;
            ViewDesc.Texture2D.MostDetailedMip     = 0;
            ViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            ViewDesc.Texture2D.PlaneSlice          = 0;
        }
        else if constexpr (TIsSame<D3D12TextureType, CD3D12Texture3D>::Value)
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

        TSharedRef<CD3D12ShaderResourceView> DefaultSRV = dbg_new CD3D12ShaderResourceView(Device, ResourceOfflineDescriptorHeap, NewTexture.Get());
        if (!DefaultSRV->AllocateHandle())
        {
            return nullptr;
        }

        if (!DefaultSRV->CreateView(NewTexture->GetD3D12Resource(), ViewDesc))
        {
            return nullptr;
        }

        NewTexture->SetShaderResourceView(DefaultSRV.ReleaseOwnership());
    }

    // TODO: Fix for other resources than Texture2D
    constexpr bool bIsTexture2D = TIsSame<D3D12TextureType, CD3D12Texture2D>::Value;

    if constexpr (bIsTexture2D)
    {
        if (Initializer.AllowDefaultRTV())
        {
            CD3D12Texture2D* NewTexture2D = static_cast<CD3D12Texture2D*>(NewTexture->GetTexture2D());

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
    }

    if constexpr (bIsTexture2D)
    {
        if (Initializer.AllowDefaultDSV())
        {
            CD3D12Texture2D* NewTexture2D = static_cast<CD3D12Texture2D*>(NewTexture->GetTexture2D());

            D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc;
            CMemory::Memzero(&ViewDesc);

            // TODO: Handle typeless
            ViewDesc.Format             = Desc.Format;
            ViewDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipSlice = 0;

            TSharedRef<CD3D12DepthStencilView> DefaultDSV = dbg_new CD3D12DepthStencilView(Device, DepthStencilOfflineDescriptorHeap);
            if (!DefaultDSV->AllocateHandle())
            {
                return nullptr;
            }

            if (!DefaultDSV->CreateView(NewTexture->GetD3D12Resource(), ViewDesc))
            {
                return nullptr;
            }

            NewTexture2D->SetDepthStencilView(DefaultDSV.ReleaseOwnership());
        }
    }

    if constexpr (bIsTexture2D)
    {
        if (Initializer.AllowDefaultUAV())
        {
            CD3D12Texture2D* NewTexture2D = static_cast<CD3D12Texture2D*>(NewTexture->GetTexture2D());

            D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
            CMemory::Memzero(&ViewDesc);

            // TODO: Handle typeless
            ViewDesc.Format               = Desc.Format;
            ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            ViewDesc.Texture2D.MipSlice   = 0;
            ViewDesc.Texture2D.PlaneSlice = 0;

            TSharedRef<CD3D12UnorderedAccessView> DefaultUAV = dbg_new CD3D12UnorderedAccessView(Device, ResourceOfflineDescriptorHeap, NewTexture2D);
            if (!DefaultUAV->AllocateHandle())
            {
                return nullptr;
            }

            if (!DefaultUAV->CreateView(nullptr, NewTexture->GetD3D12Resource(), ViewDesc))
            {
                return nullptr;
            }

            NewTexture2D->SetUnorderedAccessView(DefaultUAV.ReleaseOwnership());
        }
    }

    if constexpr (bIsTexture2D)
    {
        CRHITextureDataInitializer* InitialData = Initializer.InitialData;
        if (InitialData)
        {
            // TODO: Support other types than texture 2D

            CRHITexture2D* Texture2D = NewTexture->GetTexture2D();
            D3D12_ERROR_COND(Texture2D != nullptr, "Texture was unexpectedly nullptr");

            DirectCmdContext->StartContext();

            DirectCmdContext->TransitionTexture(Texture2D, EResourceAccess::Common, EResourceAccess::CopyDest);
            DirectCmdContext->UpdateTexture2D(Texture2D, Extent.x, Extent.y, 0, InitialData->TextureData);

            // NOTE: Transition into InitialAccess
            DirectCmdContext->TransitionTexture(Texture2D, EResourceAccess::CopyDest, Initializer.InitialAccess);

            DirectCmdContext->FinishContext();

            return NewTexture.ReleaseOwnership();
        }
    }

    if (Initializer.InitialAccess != EResourceAccess::Common)
    {
        DirectCmdContext->StartContext();
        DirectCmdContext->TransitionTexture(NewTexture.Get(), EResourceAccess::Common, Initializer.InitialAccess);
        DirectCmdContext->FinishContext();
    }

    return NewTexture.ReleaseOwnership();
}

CRHITexture2D* CD3D12CoreInterface::RHICreateTexture2D(const CRHITexture2DInitializer& Initializer)
{
    return CreateTexture<CD3D12Texture2D>(Initializer);
}

CRHITexture2DArray* CD3D12CoreInterface::RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
{
    return CreateTexture<CD3D12Texture2DArray>(Initializer);
}

CRHITextureCube* CD3D12CoreInterface::RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer)
{
    return CreateTexture<CD3D12TextureCube>(Initializer);
}

CRHITextureCubeArray* CD3D12CoreInterface::RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer)
{
    return CreateTexture<CD3D12TextureCubeArray>(Initializer);
}

CRHITexture3D* CD3D12CoreInterface::RHICreateTexture3D(const CRHITexture3DInitializer& Initializer)
{
    return CreateTexture<CD3D12Texture3D>(Initializer);
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

template<typename D3D12BufferType, typename InitializerType>
D3D12BufferType* CD3D12CoreInterface::CreateBuffer(const InitializerType& Initializer)
{
    TSharedRef<D3D12BufferType> NewBuffer;
    if constexpr (TIsSame<D3D12BufferType, CD3D12ConstantBuffer>::Value)
    {
        NewBuffer = dbg_new D3D12BufferType(GetDevice(), GetResourceOfflineDescriptorHeap(), Initializer);
    }
    else
    {
        NewBuffer = dbg_new D3D12BufferType(GetDevice(), Initializer);
    }
    
    const uint32 Size = GetBufferAlignedSize<D3D12BufferType>(NewBuffer->GetSize());

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
            return nullptr;
        }
        else
        {
            NewBuffer->SetResource(D3D12Resource.ReleaseOwnership());
        }
    }

    CRHIBufferDataInitializer* InitialData = Initializer.InitialData;
    if (InitialData)
    {
        D3D12_ERROR_COND(InitialData->Size <= Size, "Size of InitialData is larger than the allocated memory");

        if (Initializer.IsDynamic())
        {
            CD3D12Resource* D3D12Resource = NewBuffer->GetD3D12Resource();

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

            DirectCmdContext->TransitionBuffer(NewBuffer.Get(), EResourceAccess::Common, EResourceAccess::CopyDest);
            DirectCmdContext->UpdateBuffer(NewBuffer.Get(), 0, InitialData->Size, InitialData->BufferData);

            // NOTE: Transfer to the initial state
            DirectCmdContext->TransitionBuffer(NewBuffer.Get(), EResourceAccess::CopyDest, Initializer.InitialAccess);

            DirectCmdContext->FinishContext();
        }
    }
    else
    {
        if (Initializer.InitialAccess != EResourceAccess::Common && Initializer.IsDynamic())
        {
            DirectCmdContext->StartContext();
            DirectCmdContext->TransitionBuffer(NewBuffer.Get(), EResourceAccess::Common, Initializer.InitialAccess);
            DirectCmdContext->FinishContext();
        }
    }

    return NewBuffer.ReleaseOwnership();
}

CRHIVertexBuffer* CD3D12CoreInterface::RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
{
    return CreateBuffer<CD3D12VertexBuffer>(Initializer);
}

CRHIIndexBuffer* CD3D12CoreInterface::RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
{
    return CreateBuffer<CD3D12IndexBuffer>(Initializer);
}

CRHIConstantBuffer* CD3D12CoreInterface::RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
{
    Assert(!Initializer.AllowSRV() && !Initializer.AllowUAV());
    return CreateBuffer<CD3D12ConstantBuffer>(Initializer);
}

CRHIGenericBuffer* CD3D12CoreInterface::RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
{
    return CreateBuffer<CD3D12GenericBuffer>(Initializer);
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

CRHIShaderResourceView* CD3D12CoreInterface::RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer)
{
    D3D12_ERROR_COND(Initializer.Texture != nullptr, "Texture cannot be nullptr");

    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    Assert(((Initializer.Texture->GetFlags() & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None) && Initializer.Format != EFormat::Unknown);
    Desc.Format                  = ConvertFormat(Initializer.Format);
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (CD3D12Texture2D* Texture2D = static_cast<CD3D12Texture2D*>(Initializer.Texture->GetTexture2D()))
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
    else if (CD3D12Texture2DArray* Texture2DArray = static_cast<CD3D12Texture2DArray*>(Initializer.Texture->GetTexture2DArray()))
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
    else if (CD3D12TextureCube* TextureCube = static_cast<CD3D12TextureCube*>(Initializer.Texture->GetTextureCube()))
    {
        Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MostDetailedMip     = Initializer.FirstMipLevel;
        Desc.TextureCube.MipLevels           = Initializer.NumMips;
        Desc.TextureCube.ResourceMinLODClamp = Initializer.MinLODClamp;
    }
    else if (CD3D12TextureCubeArray* TextureCubeArray = static_cast<CD3D12TextureCubeArray*>(Initializer.Texture->GetTextureCubeArray()))
    {
        Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MostDetailedMip     = Initializer.FirstMipLevel;
        Desc.TextureCubeArray.MipLevels           = Initializer.NumMips;
        Desc.TextureCubeArray.ResourceMinLODClamp = Initializer.MinLODClamp;
        Desc.TextureCubeArray.First2DArrayFace    = GetDepthOrArraySize<CD3D12TextureCubeArray>(Initializer.FirstArraySlice);
        Desc.TextureCubeArray.NumCubes            = Initializer.NumSlices;
    }
    else if (CD3D12Texture3D* Texture3D = static_cast<CD3D12Texture3D*>(Initializer.Texture->GetTexture3D()))
    {
        Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MostDetailedMip     = Initializer.FirstMipLevel;
        Desc.Texture3D.MipLevels           = Initializer.NumMips;
        Desc.Texture3D.ResourceMinLODClamp = Initializer.MinLODClamp;
    }

    TSharedRef<CD3D12ShaderResourceView> D3D12View = dbg_new CD3D12ShaderResourceView(Device, ResourceOfflineDescriptorHeap, Initializer.Texture);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    CD3D12Resource* D3D12Resource = GetD3D12Resource(Initializer.Texture);
    Assert(D3D12Resource != nullptr);

    if (D3D12View->CreateView(D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHIShaderResourceView* CD3D12CoreInterface::RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer)
{
    CD3D12Buffer* D3D12Buffer = GetD3D12Buffer(Initializer.Buffer);
    if (!D3D12Buffer)
    {
        D3D12_ERROR("Cannot create a ShaderResourceView from a nullptr Buffer");
        return nullptr;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    Assert(((Initializer.Buffer->GetFlags() & EBufferUsageFlags::AllowSRV) != EBufferUsageFlags::None));
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

    TSharedRef<CD3D12ShaderResourceView> D3D12View = dbg_new CD3D12ShaderResourceView(Device, ResourceOfflineDescriptorHeap, Initializer.Buffer);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    CD3D12Resource* D3D12Resource = D3D12Buffer->GetD3D12Resource();
    Assert(D3D12Resource != nullptr);

    if (D3D12View->CreateView(D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHIUnorderedAccessView* CD3D12CoreInterface::RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer)
{
    D3D12_ERROR_COND(Initializer.Texture != nullptr, "Texture cannot be nullptr");

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    Assert(((Initializer.Texture->GetFlags() & ETextureUsageFlags::AllowUAV) != ETextureUsageFlags::None) && Initializer.Format != EFormat::Unknown);
    Desc.Format = ConvertFormat(Initializer.Format);

    if (CD3D12Texture2D* Texture2D = static_cast<CD3D12Texture2D*>(Initializer.Texture->GetTexture2D()))
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
    else if (CD3D12Texture2DArray* Texture2DArray = static_cast<CD3D12Texture2DArray*>(Initializer.Texture->GetTexture2DArray()))
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
    else if (CD3D12TextureCube* TextureCube = static_cast<CD3D12TextureCube*>(Initializer.Texture->GetTextureCube()))
    {
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = Initializer.MipLevel;
        Desc.Texture2DArray.PlaneSlice      = 0;
        Desc.Texture2DArray.FirstArraySlice = Initializer.FirstArraySlice;
        Desc.Texture2DArray.ArraySize       = Initializer.NumSlices;
    }
    else if (CD3D12TextureCubeArray* TextureCubeArray = static_cast<CD3D12TextureCubeArray*>(Initializer.Texture->GetTextureCubeArray()))
    {
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = Initializer.MipLevel;
        Desc.Texture2DArray.PlaneSlice      = 0;
        Desc.Texture2DArray.FirstArraySlice = Initializer.FirstArraySlice;
        Desc.Texture2DArray.ArraySize       = Initializer.NumSlices;
    }
    else if (CD3D12Texture3D* Texture3D = static_cast<CD3D12Texture3D*>(Initializer.Texture->GetTexture3D()))
    {
        Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.FirstWSlice = Initializer.FirstArraySlice;
        Desc.Texture3D.WSize       = Initializer.NumSlices;
        Desc.Texture3D.MipSlice    = Initializer.MipLevel;
    }

    TSharedRef<CD3D12UnorderedAccessView> D3D12View = dbg_new CD3D12UnorderedAccessView(Device, ResourceOfflineDescriptorHeap, Initializer.Texture);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    CD3D12Resource* D3D12Resource = GetD3D12Resource(Initializer.Texture);
    Assert(D3D12Resource != nullptr);

    if (D3D12View->CreateView(nullptr, D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHIUnorderedAccessView* CD3D12CoreInterface::RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer)
{
    CD3D12Buffer* D3D12Buffer = GetD3D12Buffer(Initializer.Buffer);
    if (!D3D12Buffer)
    {
        D3D12_ERROR("Cannot create a UnorderedAccessView from a nullptr Buffer");
        return nullptr;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    Assert(((Initializer.Buffer->GetFlags() & EBufferUsageFlags::AllowSRV) != EBufferUsageFlags::None));
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

    TSharedRef<CD3D12UnorderedAccessView> D3D12View = dbg_new CD3D12UnorderedAccessView(Device, ResourceOfflineDescriptorHeap, Initializer.Buffer);
    if (!D3D12View->AllocateHandle())
    {
        return nullptr;
    }

    CD3D12Resource* D3D12Resource = D3D12Buffer->GetD3D12Resource();
    Assert(D3D12Resource != nullptr);

    if (D3D12View->CreateView(nullptr, D3D12Resource, Desc))
    {
        return D3D12View.ReleaseOwnership();
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
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * kRHINumCubeFaces + GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
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
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * kRHINumCubeFaces + GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
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
