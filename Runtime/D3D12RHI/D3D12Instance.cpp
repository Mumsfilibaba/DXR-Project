#include "CoreApplication/Windows/WindowsWindow.h"

#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12RootSignature.h"
#include "D3D12Core.h"
#include "D3D12Instance.h"
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
// CD3D12Instance

CD3D12Instance* GD3D12RHIInstance = nullptr;

CRHIInstance* CD3D12Instance::CreateInstance() 
{ 
    return dbg_new CD3D12Instance(); 
}

CD3D12Instance::CD3D12Instance()
    : CRHIInstance(ERHIInstanceApi::D3D12)
    , Device(nullptr)
    , DirectCommandContext(nullptr)
{
    GD3D12RHIInstance = this;
}

CD3D12Instance::~CD3D12Instance()
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

bool CD3D12Instance::Initialize(bool bEnableDebug)
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

    Device = CD3D12Device::CreateDevice(bEnableDebug, bGPUBasedValidationOn, bDREDOn);
    if (!Device)
    {
        D3D12_ERROR_ALWAYS("Failed to create device");
        return false;
    }

    // RootSignature cache
    RootSignatureCache = dbg_new CD3D12RootSignatureCache(GetDevice());
    if (!RootSignatureCache->Init())
    {
        return false;
    }

    // Init Offline Descriptor heaps
    ResourceOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!ResourceOfflineDescriptorHeap->Init())
    {
        return false;
    }

    RenderTargetOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if (!RenderTargetOfflineDescriptorHeap->Init())
    {
        return false;
    }

    DepthStencilOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    if (!DepthStencilOfflineDescriptorHeap->Init())
    {
        return false;
    }

    SamplerOfflineDescriptorHeap = dbg_new CD3D12OfflineDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!SamplerOfflineDescriptorHeap->Init())
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
    if (!GenerateMipsTex2D_PSO->Init())
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
    if (!GenerateMipsTexCube_PSO->Init())
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
D3D12TextureType* CD3D12Instance::CreateTexture(EFormat Format, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint32 NumMips, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
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
        ClearValue.Format = (OptimalClearValue.GetFormat() != EFormat::Unknown) ? ConvertFormat(OptimalClearValue.GetFormat()) : Desc.Format;
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

    TSharedRef<CD3D12Resource> Resource = dbg_new CD3D12Resource(GetDevice(), Desc, D3D12_HEAP_TYPE_DEFAULT);
    if (!Resource->Init(D3D12_RESOURCE_STATE_COMMON, ClearValuePtr))
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

CRHITexture2D* CD3D12Instance::CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12Texture2D>(Format, Width, Height, 1, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITexture2DArray* CD3D12Instance::CreateTexture2DArray(EFormat Format,uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12Texture2DArray>(Format, Width, Height, NumArraySlices, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITextureCube* CD3D12Instance::CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMips, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12TextureCube>(Format, Size, Size, TEXTURE_CUBE_FACE_COUNT, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITextureCubeArray* CD3D12Instance::CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMips, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    const uint32 ArraySlices = NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
    return CreateTexture<CD3D12TextureCubeArray>(Format, Size, Size, ArraySlices, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITexture3D* CD3D12Instance::CreateTexture3D(EFormat Format, uint32 Width, uint32 Height, uint32 Depth, uint32 NumMips, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12Texture3D>(Format, Width, Height, Depth, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHISamplerState* CD3D12Instance::CreateSamplerState(const SRHISamplerStateInfo& CreateInfo)
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

    TSharedRef<CD3D12SamplerState> Sampler = dbg_new CD3D12SamplerState(GetDevice(), SamplerOfflineDescriptorHeap);
    if (!Sampler->Init(Desc))
    {
        return nullptr;
    }
    else
    {
        return Sampler.ReleaseOwnership();
    }
}

template<typename D3D12BufferType>
bool CD3D12Instance::CreateBuffer(D3D12BufferType* Buffer, uint32 SizeInBytes, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    D3D12_ERROR(Buffer != nullptr, "Buffer cannot be nullptr");

    D3D12_RESOURCE_DESC Desc;
    CMemory::Memzero(&Desc);

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
    if (Flags & BufferFlag_Dynamic)
    {
        DxHeapType     = D3D12_HEAP_TYPE_UPLOAD;
        DxInitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    TSharedRef<CD3D12Resource> Resource = dbg_new CD3D12Resource(GetDevice(), Desc, DxHeapType);
    if (!Resource->Init(DxInitialState, nullptr))
    {
        return false;
    }
    else
    {
        Buffer->SetResource(Resource.ReleaseOwnership());
    }

    D3D12_ERROR(Buffer->GetSizeInBytes() <= SizeInBytes, "Size of InitialData is larger than the allocated memory");

    if (InitialData)
    {
        D3D12_ERROR(InitialData->GetSizeInBytes() <= SizeInBytes, "Size of InitialData is larger than the allocated memory");

        if (Buffer->IsDynamic())
        {
            void* HostData = Buffer->Map(0, 0);
            if (!HostData)
            {
                return false;
            }

            // Copy over relevant data
            const uint32 InitialDataSize = InitialData->GetSizeInBytes();
            CMemory::Memcpy(HostData, InitialData->GetData(), InitialDataSize);
            // Set the remaining, unused memory to zero
            CMemory::Memzero(reinterpret_cast<uint8*>(HostData) + InitialDataSize, SizeInBytes - InitialDataSize);

            Buffer->Unmap(0, 0);
        }
        else
        {
            DirectCommandContext->Begin();

            DirectCommandContext->TransitionBuffer(Buffer, ERHIResourceState::Common, ERHIResourceState::CopyDest);
            DirectCommandContext->UpdateBuffer(Buffer, 0, InitialData->GetSizeInBytes(), InitialData->GetData());

            // NOTE: Transfer to the initial state
            DirectCommandContext->TransitionBuffer(Buffer, ERHIResourceState::CopyDest, InitialState);

            DirectCommandContext->End();
        }
    }
    else
    {
        if (InitialState != ERHIResourceState::Common && !Buffer->IsDynamic())
        {
            DirectCommandContext->Begin();
            DirectCommandContext->TransitionBuffer(Buffer, ERHIResourceState::Common, InitialState);
            DirectCommandContext->End();
        }
    }

    return true;
}

CRHIVertexBuffer* CD3D12Instance::CreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    const uint32 SizeInBytes = NumVertices * Stride;

    TSharedRef<CD3D12VertexBuffer> NewBuffer = dbg_new CD3D12VertexBuffer(GetDevice(), NumVertices, Stride, Flags);
    if (!CreateBuffer<CD3D12VertexBuffer>(NewBuffer.Get(), SizeInBytes, Flags, InitialState, InitialData))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to create VertexBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIIndexBuffer* CD3D12Instance::CreateIndexBuffer(ERHIIndexFormat Format, uint32 NumIndices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    const uint32 SizeInBytes = NumIndices * GetStrideFromIndexFormat(Format);
    const uint32 AlignedSizeInBytes = NMath::AlignUp<uint32>(SizeInBytes, sizeof(uint32));

    TSharedRef<CD3D12IndexBuffer> NewBuffer = dbg_new CD3D12IndexBuffer(GetDevice(), Format, NumIndices, Flags);
    if (!CreateBuffer<CD3D12IndexBuffer>(NewBuffer.Get(), AlignedSizeInBytes, Flags, InitialState, InitialData))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to create IndexBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIConstantBuffer* CD3D12Instance::CreateConstantBuffer(uint32 Size, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    Assert(!(Flags & BufferFlag_UAV) && !(Flags & BufferFlag_SRV));

    const uint32 AlignedSizeInBytes = NMath::AlignUp<uint32>(Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    TSharedRef<CD3D12ConstantBuffer> NewBuffer = dbg_new CD3D12ConstantBuffer(GetDevice(), ResourceOfflineDescriptorHeap, Size, Flags);
    if (!CreateBuffer<CD3D12ConstantBuffer>(NewBuffer.Get(), AlignedSizeInBytes, Flags, InitialState, InitialData))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to create ConstantBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIStructuredBuffer* CD3D12Instance::CreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    const uint32 SizeInBytes = NumElements * Stride;

    TSharedRef<CD3D12StructuredBuffer> NewBuffer = dbg_new CD3D12StructuredBuffer(GetDevice(), NumElements, Stride, Flags);
    if (!CreateBuffer<CD3D12StructuredBuffer>(NewBuffer.Get(), SizeInBytes, Flags, InitialState, InitialData))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to create StructuredBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIRayTracingGeometry* CD3D12Instance::CreateRayTracingGeometry(uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer)
{
    CD3D12VertexBuffer* DxVertexBuffer = static_cast<CD3D12VertexBuffer*>(VertexBuffer);
    CD3D12IndexBuffer* DxIndexBuffer = static_cast<CD3D12IndexBuffer*>(IndexBuffer);

    TSharedRef<CD3D12RHIRayTracingGeometry> Geometry = dbg_new CD3D12RHIRayTracingGeometry(GetDevice(), Flags);
    Geometry->VertexBuffer = MakeSharedRef<CD3D12VertexBuffer>(DxVertexBuffer);
    Geometry->IndexBuffer = MakeSharedRef<CD3D12IndexBuffer>(DxIndexBuffer);

    DirectCommandContext->Begin();

    if (!Geometry->Build(*DirectCommandContext, false))
    {
        CDebug::DebugBreak();
        Geometry.Reset();
    }

    DirectCommandContext->End();

    return Geometry.ReleaseOwnership();
}

CRHIRayTracingScene* CD3D12Instance::CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    TSharedRef<CD3D12RHIRayTracingScene> Scene = dbg_new CD3D12RHIRayTracingScene(GetDevice(), Flags);

    DirectCommandContext->Begin();

    if (!Scene->Build(*DirectCommandContext, Instances, NumInstances, false))
    {
        CDebug::DebugBreak();
        Scene.Reset();
    }

    DirectCommandContext->End();

    return Scene.ReleaseOwnership();
}

CRHIShaderResourceView* CD3D12Instance::CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    // TODO: Expose in ShaderResourceViewCreateInfo
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    CD3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::Texture2D)
    {
        CRHITexture2D*    Texture    = CreateInfo.Texture2D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsSRV() && CreateInfo.Texture2D.Format != EFormat::Unknown);

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
        CRHITexture2DArray* Texture   = CreateInfo.Texture2DArray.Texture;
        CD3D12BaseTexture*  DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsSRV() && CreateInfo.Texture2DArray.Format != EFormat::Unknown);

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
        CRHITextureCube*   Texture   = CreateInfo.TextureCube.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsSRV() && CreateInfo.TextureCube.Format != EFormat::Unknown);

        Desc.Format                          = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        Desc.TextureCube.MipLevels           = CreateInfo.TextureCube.NumMips;
        Desc.TextureCube.MostDetailedMip     = CreateInfo.TextureCube.Mip;
        Desc.TextureCube.ResourceMinLODClamp = CreateInfo.TextureCube.MinMipBias;
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::TextureCubeArray)
    {
        CRHITextureCubeArray* Texture   = CreateInfo.TextureCubeArray.Texture;
        CD3D12BaseTexture*    DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsSRV() && CreateInfo.TextureCubeArray.Format != EFormat::Unknown);

        Desc.Format                               = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        Desc.TextureCubeArray.MipLevels           = CreateInfo.TextureCubeArray.NumMips;
        Desc.TextureCubeArray.MostDetailedMip     = CreateInfo.TextureCubeArray.Mip;
        Desc.TextureCubeArray.ResourceMinLODClamp = CreateInfo.TextureCubeArray.MinMipBias;
        Desc.TextureCubeArray.First2DArrayFace    = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT; // ArraySlice * 6 to get the first Texture2D face
        Desc.TextureCubeArray.NumCubes            = CreateInfo.TextureCubeArray.NumArraySlices;
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::Texture3D)
    {
        CRHITexture3D*     Texture   = CreateInfo.Texture3D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsSRV() && CreateInfo.Texture3D.Format != EFormat::Unknown);

        Desc.Format                        = ConvertFormat(CreateInfo.Texture3D.Format);
        Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipLevels           = CreateInfo.Texture3D.NumMips;
        Desc.Texture3D.MostDetailedMip     = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.ResourceMinLODClamp = CreateInfo.Texture3D.MinMipBias;
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::VertexBuffer)
    {
        CRHIVertexBuffer* Buffer   = CreateInfo.VertexBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        Assert(Buffer->IsSRV());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements         = CreateInfo.VertexBuffer.NumVertices;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::IndexBuffer)
    {
        CRHIIndexBuffer*  Buffer   = CreateInfo.IndexBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        Assert(Buffer->IsSRV());
        Assert(Buffer->GetFormat() != ERHIIndexFormat::uint16);

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements         = CreateInfo.IndexBuffer.NumIndices;
        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::StructuredBuffer)
    {
        CRHIStructuredBuffer* Buffer   = CreateInfo.StructuredBuffer.Buffer;
        CD3D12BaseBuffer*     DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        Assert(Buffer->IsSRV());

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
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

CRHIUnorderedAccessView* CD3D12Instance::CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::Texture2D)
    {
        CRHITexture2D*     Texture   = CreateInfo.Texture2D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsUAV() && CreateInfo.Texture2D.Format != EFormat::Unknown);

        Desc.Format               = ConvertFormat(CreateInfo.Texture2D.Format);
        Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        Desc.Texture2D.MipSlice   = CreateInfo.Texture2D.Mip;
        Desc.Texture2D.PlaneSlice = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::Texture2DArray)
    {
        CRHITexture2DArray* Texture   = CreateInfo.Texture2DArray.Texture;
        CD3D12BaseTexture*  DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsUAV() && CreateInfo.Texture2DArray.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.Texture2DArray.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.Texture2DArray.Mip;
        Desc.Texture2DArray.ArraySize       = CreateInfo.Texture2DArray.NumArraySlices;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.Texture2DArray.ArraySlice;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::TextureCube)
    {
        CRHITextureCube*   Texture   = CreateInfo.TextureCube.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsUAV() && CreateInfo.TextureCube.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.TextureCube.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = 0;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::TextureCubeArray)
    {
        CRHITextureCubeArray* Texture   = CreateInfo.TextureCubeArray.Texture;
        CD3D12BaseTexture*    DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsUAV() && CreateInfo.TextureCubeArray.Format != EFormat::Unknown);

        Desc.Format                         = ConvertFormat(CreateInfo.TextureCubeArray.Format);
        Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = CreateInfo.TextureCubeArray.NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::Texture3D)
    {
        CRHITexture3D*     Texture   = CreateInfo.Texture3D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsUAV() && CreateInfo.Texture3D.Format != EFormat::Unknown);

        Desc.Format                = ConvertFormat(CreateInfo.Texture3D.Format);
        Desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = CreateInfo.Texture3D.DepthSlice;
        Desc.Texture3D.WSize       = CreateInfo.Texture3D.NumDepthSlices;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::VertexBuffer)
    {
        CRHIVertexBuffer* Buffer   = CreateInfo.VertexBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        Assert(Buffer->IsUAV());

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.VertexBuffer.FirstVertex;
        Desc.Buffer.NumElements         = CreateInfo.VertexBuffer.NumVertices;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::IndexBuffer)
    {
        CRHIIndexBuffer*  Buffer   = CreateInfo.IndexBuffer.Buffer;
        CD3D12BaseBuffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        Assert(Buffer->IsUAV());

        Desc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = CreateInfo.IndexBuffer.FirstIndex;
        Desc.Buffer.NumElements  = CreateInfo.IndexBuffer.NumIndices;

        // TODO: What if the index type is 16-bit?
        Assert(Buffer->GetFormat() != ERHIIndexFormat::uint16);

        Desc.Format                     = DXGI_FORMAT_R32_TYPELESS;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_RAW;
        Desc.Buffer.StructureByteStride = 0;
    }
    else if (CreateInfo.Type == SRHIUnorderedAccessViewInfo::EType::StructuredBuffer)
    {
        CRHIStructuredBuffer* Buffer   = CreateInfo.StructuredBuffer.Buffer;
        CD3D12BaseBuffer*     DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        Assert(Buffer->IsUAV());

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    TSharedRef<CD3D12UnorderedAccessView> DxView = dbg_new CD3D12UnorderedAccessView(GetDevice(), ResourceOfflineDescriptorHeap);
    if (!DxView->AllocateHandle())
    {
        return nullptr;
    }

    Assert(Resource != nullptr);

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

CRHIRenderTargetView* CD3D12Instance::CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo)
{
    D3D12_RENDER_TARGET_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat(CreateInfo.Format);
    Assert(CreateInfo.Format != EFormat::Unknown);

    if (CreateInfo.Type == SRHIRenderTargetViewInfo::EType::Texture2D)
    {
        CRHITexture2D*     Texture   = CreateInfo.Texture2D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsRTV());

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
        CRHITexture2DArray* Texture   = CreateInfo.Texture2DArray.Texture;
        CD3D12BaseTexture*  DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsRTV());

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
        CRHITextureCube*   Texture   = CreateInfo.TextureCube.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsRTV());

        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIRenderTargetViewInfo::EType::TextureCubeArray)
    {
        CRHITextureCubeArray* Texture   = CreateInfo.TextureCubeArray.Texture;
        CD3D12BaseTexture*    DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsRTV());

        Desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
        Desc.Texture2DArray.PlaneSlice      = 0;
    }
    else if (CreateInfo.Type == SRHIRenderTargetViewInfo::EType::Texture3D)
    {
        CRHITexture3D*     Texture   = CreateInfo.Texture3D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsRTV());

        Desc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
        Desc.Texture3D.MipSlice    = CreateInfo.Texture3D.Mip;
        Desc.Texture3D.FirstWSlice = CreateInfo.Texture3D.DepthSlice;
        Desc.Texture3D.WSize       = CreateInfo.Texture3D.NumDepthSlices;
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

CRHIDepthStencilView* CD3D12Instance::CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    CD3D12Resource* Resource = nullptr;

    Desc.Format = ConvertFormat(CreateInfo.Format);
    Assert(CreateInfo.Format != EFormat::Unknown);

    if (CreateInfo.Type == SRHIDepthStencilViewInfo::EType::Texture2D)
    {
        CRHITexture2D*     Texture   = CreateInfo.Texture2D.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsDSV());

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
        CRHITexture2DArray* Texture   = CreateInfo.Texture2DArray.Texture;
        CD3D12BaseTexture*  DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsDSV());

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
        CRHITextureCube*   Texture   = CreateInfo.TextureCube.Texture;
        CD3D12BaseTexture* DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsDSV());

        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCube.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
    }
    else if (CreateInfo.Type == SRHIDepthStencilViewInfo::EType::TextureCubeArray)
    {
        CRHITextureCubeArray* Texture   = CreateInfo.TextureCubeArray.Texture;
        CD3D12BaseTexture*    DxTexture = D3D12TextureCast(Texture);
        Resource = DxTexture->GetResource();

        Assert(Texture->IsDSV());

        Desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        Desc.Texture2DArray.MipSlice        = CreateInfo.TextureCubeArray.Mip;
        Desc.Texture2DArray.ArraySize       = 1;
        Desc.Texture2DArray.FirstArraySlice = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT + GetCubeFaceIndex(CreateInfo.TextureCube.CubeFace);
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

CRHIComputeShader* CD3D12Instance::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12ComputeShader> Shader = dbg_new CD3D12ComputeShader(GetDevice(), ShaderCode);
    if (!Shader->Init())
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIVertexShader* CD3D12Instance::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12VertexShader> Shader = dbg_new CD3D12VertexShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseShader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIHullShader* CD3D12Instance::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIDomainShader* CD3D12Instance::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIGeometryShader* CD3D12Instance::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIMeshShader* CD3D12Instance::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIAmplificationShader* CD3D12Instance::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIPixelShader* CD3D12Instance::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12PixelShader> Shader = dbg_new CD3D12PixelShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseShader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIRayGenShader* CD3D12Instance::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayGenShader> Shader = dbg_new CD3D12RayGenShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to retrive Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayAnyHitShader* CD3D12Instance::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayAnyHitShader> Shader = dbg_new CD3D12RayAnyHitShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to retrive Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayClosestHitShader* CD3D12Instance::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayClosestHitShader> Shader = dbg_new CD3D12RayClosestHitShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to retrive Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayMissShader* CD3D12Instance::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayMissShader> Shader = dbg_new CD3D12RayMissShader(GetDevice(), ShaderCode);
    if (!CD3D12BaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to retrive Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIDepthStencilState* CD3D12Instance::CreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo)
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

CRHIRasterizerState* CD3D12Instance::CreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo)
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

CRHIBlendState* CD3D12Instance::CreateBlendState(const SRHIBlendStateInfo& CreateInfo)
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

CRHIInputLayoutState* CD3D12Instance::CreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo)
{
    return dbg_new CD3D12InputLayoutState(GetDevice(), CreateInfo);
}

CRHIGraphicsPipelineState* CD3D12Instance::CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo)
{
    TSharedRef<CD3D12GraphicsPipelineState> NewPipelineState = dbg_new CD3D12GraphicsPipelineState(GetDevice());
    if (!NewPipelineState->Init(CreateInfo))
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIComputePipelineState* CD3D12Instance::CreateComputePipelineState(const SRHIComputePipelineStateInfo& Info)
{
    Assert(Info.Shader != nullptr);

    TSharedRef<CD3D12ComputeShader> Shader = MakeSharedRef<CD3D12ComputeShader>(Info.Shader);
    TSharedRef<CD3D12ComputePipelineState> NewPipelineState = dbg_new CD3D12ComputePipelineState(GetDevice(), Shader);
    if (!NewPipelineState->Init())
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIRayTracingPipelineState* CD3D12Instance::CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo)
{
    TSharedRef<CD3D12RayTracingPipelineState> NewPipelineState = dbg_new CD3D12RayTracingPipelineState(GetDevice());
    if (NewPipelineState->Init(CreateInfo))
    {
        return NewPipelineState.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHITimestampQuery* CD3D12Instance::CreateTimestampQuery()
{
    return CD3D12TimestampQuery::Create(GetDevice());
}

CRHIViewport* CD3D12Instance::CreateViewport(CPlatformWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat)
{
    UNREFERENCED_VARIABLE(DepthFormat);

    // TODO: Take DepthFormat into account

    TSharedRef<CWindowsWindow> WinWindow = MakeSharedRef<CWindowsWindow>(Window);
    if (Width == 0)
    {
        Width = WinWindow->GetWidth();
    }

    if (Height == 0)
    {
        Height = WinWindow->GetHeight();
    }

    TSharedRef<CD3D12Viewport> Viewport = dbg_new CD3D12Viewport(GetDevice(), DirectCommandContext.Get(), WinWindow->GetHandle(), ColorFormat, Width, Height);
    if (Viewport->Init())
    {
        return Viewport.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

bool CD3D12Instance::UAVSupportsFormat(EFormat Format) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
    CMemory::Memzero(&FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

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

void CD3D12Instance::CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const
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

void CD3D12Instance::CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const
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
