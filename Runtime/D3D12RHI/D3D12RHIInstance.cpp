#include "CoreApplication/Windows/WindowsWindow.h"

#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12RootSignature.h"
#include "D3D12Core.h"
#include "D3D12RHIInstance.h"
#include "D3D12RHIViews.h"
#include "D3D12RHIRayTracing.h"
#include "D3D12RHIPipelineState.h"
#include "D3D12RHITexture.h"
#include "D3D12Buffer.h"
#include "D3D12RHISamplerState.h"
#include "D3D12RHIViewport.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12RHITimestampQuery.h"

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

CD3D12RHIInstance* GD3D12RHIInstance = nullptr;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RHIInstance

CD3D12RHIInstance* CD3D12RHIInstance::CreateD3D12Instance()
{ 
    return dbg_new CD3D12RHIInstance(); 
}

CD3D12RHIInstance::CD3D12RHIInstance()
    : CRHIInstance(ERHIInstanceType::D3D12)
    , Device(nullptr)
    , DirectCmdContext(nullptr)
{
    GD3D12RHIInstance = this;
}

CD3D12RHIInstance::~CD3D12RHIInstance()
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

    GD3D12RHIInstance = nullptr;
}

bool CD3D12RHIInstance::Initialize(bool bEnableDebug)
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
    if (!RootSignatureCache->Init())
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

    TSharedRef<CD3D12RHIComputeShader> Shader = dbg_new CD3D12RHIComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        CDebug::DebugBreak();
        return false;
    }

    GenerateMipsTex2D_PSO = dbg_new CD3D12RHIComputePipelineState(GetDevice(), Shader);
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

    Shader = dbg_new CD3D12RHIComputeShader(GetDevice(), Code);
    if (!Shader->Init())
    {
        CDebug::DebugBreak();
        return false;
    }

    GenerateMipsTexCube_PSO = dbg_new CD3D12RHIComputePipelineState(GetDevice(), Shader);
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
D3D12TextureType* CD3D12RHIInstance::CreateTexture(EFormat Format, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint32 NumMips, uint32 NumSamples, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    TSharedRef<D3D12TextureType> NewTexture = dbg_new D3D12TextureType(Device, Format, SizeX, SizeY, SizeZ, NumMips, NumSamples, Flags, OptimalClearValue);

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

    D3D12_CLEAR_VALUE* ClearValuePtr = nullptr;
    D3D12_CLEAR_VALUE  ClearValue;
    if (Flags & TextureFlag_RTV || Flags & TextureFlag_DSV)
    {
        ClearValue.Format = (OptimalClearValue.GetFormat() != EFormat::Unknown) ? ConvertFormat(OptimalClearValue.GetFormat()) : Desc.Format;
        if (OptimalClearValue.GetType() == SClearValue::EType::DepthStencil)
        {
            ClearValue.DepthStencil.Depth = OptimalClearValue.AsDepthStencil().Depth;
            ClearValue.DepthStencil.Stencil = OptimalClearValue.AsDepthStencil().Stencil;
            ClearValuePtr = &ClearValue;
        }
        else if (OptimalClearValue.GetType() == SClearValue::EType::Color)
        {
            CMemory::Memcpy(ClearValue.Color, OptimalClearValue.AsColor().Data(), sizeof(float[4]));
            ClearValuePtr = &ClearValue;
        }
    }

    TSharedRef<CD3D12Resource> Resource = dbg_new CD3D12Resource(Device, Desc, D3D12_HEAP_TYPE_DEFAULT);
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

        TSharedRef<CD3D12RHIShaderResourceView> SRV = dbg_new CD3D12RHIShaderResourceView(Device, ResourceOfflineDescriptorHeap);
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
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->AsTexture2D());

        D3D12_RENDER_TARGET_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format               = Desc.Format;
        ViewDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice   = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<CD3D12RHIRenderTargetView> RTV = dbg_new CD3D12RHIRenderTargetView(Device, RenderTargetOfflineDescriptorHeap);
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
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->AsTexture2D());

        D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format             = Desc.Format;
        ViewDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice = 0;

        TSharedRef<CD3D12RHIDepthStencilView> DSV = dbg_new CD3D12RHIDepthStencilView(Device, DepthStencilOfflineDescriptorHeap);
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
        CD3D12RHITexture2D* NewTexture2D = static_cast<CD3D12RHITexture2D*>(NewTexture->AsTexture2D());

        D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc;
        CMemory::Memzero(&ViewDesc);

        // TODO: Handle typeless
        ViewDesc.Format               = Desc.Format;
        ViewDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        ViewDesc.Texture2D.MipSlice   = 0;
        ViewDesc.Texture2D.PlaneSlice = 0;

        TSharedRef<CD3D12RHIUnorderedAccessView> UAV = dbg_new CD3D12RHIUnorderedAccessView(Device, ResourceOfflineDescriptorHeap);
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

        DirectCmdContext->Begin();

        DirectCmdContext->TransitionTexture(Texture2D, EResourceAccess::Common, EResourceAccess::CopyDest);
        DirectCmdContext->UpdateTexture2D(Texture2D, SizeX, SizeY, 0, InitialData->GetData());

        // NOTE: Transition into InitialState
        DirectCmdContext->TransitionTexture(Texture2D, EResourceAccess::CopyDest, InitialState);

        DirectCmdContext->End();
    }
    else
    {
        if (InitialState != EResourceAccess::Common)
        {
            DirectCmdContext->Begin();
            DirectCmdContext->TransitionTexture(NewTexture.Get(), EResourceAccess::Common, InitialState);
            DirectCmdContext->End();
        }
    }

    return NewTexture.ReleaseOwnership();
}

CRHITexture2D* CD3D12RHIInstance::CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12RHITexture2D>(Format, Width, Height, 1, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITexture2DArray* CD3D12RHIInstance::CreateTexture2DArray(EFormat Format,uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12RHITexture2DArray>(Format, Width, Height, NumArraySlices, NumMips, NumSamples, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITextureCube* CD3D12RHIInstance::CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMips, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12RHITextureCube>(Format, Size, Size, TEXTURE_CUBE_FACE_COUNT, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITextureCubeArray* CD3D12RHIInstance::CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMips, uint32 NumArraySlices, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    const uint32 ArraySlices = NumArraySlices * TEXTURE_CUBE_FACE_COUNT;
    return CreateTexture<CD3D12RHITextureCubeArray>(Format, Size, Size, ArraySlices, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHITexture3D* CD3D12RHIInstance::CreateTexture3D(EFormat Format, uint32 Width, uint32 Height, uint32 Depth, uint32 NumMips, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue)
{
    return CreateTexture<CD3D12RHITexture3D>(Format, Width, Height, Depth, NumMips, 1, Flags, InitialState, InitialData, OptimalClearValue);
}

CRHISamplerState* CD3D12RHIInstance::CreateSamplerState(const SRHISamplerStateInfo& CreateInfo)
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

    CMemory::Memcpy(Desc.BorderColor, CreateInfo.BorderColor.Data(), sizeof(Desc.BorderColor));

    TSharedRef<CD3D12RHISamplerState> Sampler = dbg_new CD3D12RHISamplerState(Device, SamplerOfflineDescriptorHeap);
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
bool CD3D12RHIInstance::CreateBuffer(D3D12BufferType* Buffer, EBufferUsageFlags Flags, uint32 SizeInBytes, EResourceAccess InitialState, const SRHIResourceData* InitialData)
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
    if ((Flags & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None)
    {
        DxHeapType     = D3D12_HEAP_TYPE_UPLOAD;
        DxInitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    TSharedRef<CD3D12Resource> Resource = dbg_new CD3D12Resource(Device, Desc, DxHeapType);
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

        if ((Buffer->GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None)
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
            DirectCmdContext->Begin();

            DirectCmdContext->TransitionBuffer(Buffer, EResourceAccess::Common, EResourceAccess::CopyDest);
            DirectCmdContext->UpdateBuffer(Buffer, 0, InitialData->GetSizeInBytes(), InitialData->GetData());

            // NOTE: Transfer to the initial state
            DirectCmdContext->TransitionBuffer(Buffer, EResourceAccess::CopyDest, InitialState);

            DirectCmdContext->End();
        }
    }
    else
    {
        if (InitialState != EResourceAccess::Common && (Buffer->GetFlags() & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None)
        {
            DirectCmdContext->Begin();
            DirectCmdContext->TransitionBuffer(Buffer, EResourceAccess::Common, InitialState);
            DirectCmdContext->End();
        }
    }

    return true;
}

CRHIVertexBuffer* CD3D12RHIInstance::CreateVertexBuffer(uint32 Stride, uint32 NumVertices, EBufferUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData)
{
    const uint32 SizeInBytes = NumVertices * Stride;

    TSharedRef<CD3D12VertexBuffer> NewBuffer = dbg_new CD3D12VertexBuffer(Device, Flags, NumVertices, Stride);
    if (!CreateBuffer(NewBuffer.Get(), Flags, SizeInBytes, InitialState, InitialData))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to create VertexBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIIndexBuffer* CD3D12RHIInstance::CreateIndexBuffer(EIndexFormat Format, uint32 NumIndices, EBufferUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData)
{
    const uint32 SizeInBytes = NumIndices * GetStrideFromIndexFormat(Format);
    const uint32 AlignedSizeInBytes = NMath::AlignUp<uint32>(SizeInBytes, sizeof(uint32));

    TSharedRef<CD3D12IndexBuffer> NewBuffer = dbg_new CD3D12IndexBuffer(Device, Flags, Format, NumIndices);
    if (!CreateBuffer(NewBuffer.Get(), Flags, AlignedSizeInBytes, InitialState, InitialData))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to create IndexBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIConstantBuffer* CD3D12RHIInstance::CreateConstantBuffer(uint32 Size, EBufferUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData)
{
    Assert(((Flags & EBufferUsageFlags::AllowUAV) == EBufferUsageFlags::None) && ((Flags & EBufferUsageFlags::AllowSRV) == EBufferUsageFlags::None));

    const uint32 AlignedSizeInBytes = NMath::AlignUp<uint32>(Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    TSharedRef<CD3D12ConstantBuffer> NewBuffer = dbg_new CD3D12ConstantBuffer(Device, ResourceOfflineDescriptorHeap, Flags, Size);
    if (!CreateBuffer(NewBuffer.Get(), Flags, AlignedSizeInBytes, InitialState, InitialData))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to create ConstantBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIGenericBuffer* CD3D12RHIInstance::CreateGenericBuffer(uint32 Stride, uint32 NumElements, EBufferUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData)
{
    const uint32 SizeInBytes = NumElements * Stride;

    TSharedRef<CD3D12GenericBuffer> NewBuffer = dbg_new CD3D12GenericBuffer(Device, Flags, NumElements, Stride);
    if (!CreateBuffer(NewBuffer.Get(), Flags, SizeInBytes, InitialState, InitialData))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to create StructuredBuffer");
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

CRHIRayTracingGeometry* CD3D12RHIInstance::CreateRayTracingGeometry(uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer)
{
    CD3D12VertexBuffer* DxVertexBuffer = static_cast<CD3D12VertexBuffer*>(VertexBuffer);
    CD3D12IndexBuffer*  DxIndexBuffer  = static_cast<CD3D12IndexBuffer*>(IndexBuffer);

    TSharedRef<CD3D12RHIRayTracingGeometry> Geometry = dbg_new CD3D12RHIRayTracingGeometry(Device, Flags);
    Geometry->VertexBuffer = MakeSharedRef<CD3D12VertexBuffer>(DxVertexBuffer);
    Geometry->IndexBuffer  = MakeSharedRef<CD3D12IndexBuffer>(DxIndexBuffer);

    DirectCmdContext->Begin();

    if (!Geometry->Build(*DirectCmdContext, false))
    {
        CDebug::DebugBreak();
        Geometry.Reset();
    }

    DirectCmdContext->End();

    return Geometry.ReleaseOwnership();
}

CRHIRayTracingScene* CD3D12RHIInstance::CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    TSharedRef<CD3D12RHIRayTracingScene> Scene = dbg_new CD3D12RHIRayTracingScene(Device, Flags);

    DirectCmdContext->Begin();

    if (!Scene->Build(*DirectCmdContext, Instances, NumInstances, false))
    {
        CDebug::DebugBreak();
        Scene.Reset();
    }

    DirectCmdContext->End();

    return Scene.ReleaseOwnership();
}

CRHIShaderResourceView* CD3D12RHIInstance::CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
    CMemory::Memzero(&Desc);

    // TODO: Expose in ShaderResourceViewCreateInfo
    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    CD3D12Resource* Resource = nullptr;
    if (CreateInfo.Type == SRHIShaderResourceViewInfo::EType::Texture2D)
    {
        CRHITexture2D*     Texture   = CreateInfo.Texture2D.Texture;
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
        // ArraySlice * 6 to get the first Texture2D face
        Desc.TextureCubeArray.First2DArrayFace    = CreateInfo.TextureCubeArray.ArraySlice * TEXTURE_CUBE_FACE_COUNT;
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
        CD3D12Buffer*     DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

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
        CRHIIndexBuffer* Buffer   = CreateInfo.IndexBuffer.Buffer;
        CD3D12Buffer*    DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

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
        CRHIGenericBuffer* Buffer   = CreateInfo.StructuredBuffer.Buffer;
        CD3D12Buffer*      DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        Assert(((Buffer->GetFlags() & EBufferUsageFlags::AllowSRV) != EBufferUsageFlags::None));

        Desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    Assert(Resource != nullptr);

    TSharedRef<CD3D12RHIShaderResourceView> DxView = dbg_new CD3D12RHIShaderResourceView(Device, ResourceOfflineDescriptorHeap);
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

CRHIUnorderedAccessView* CD3D12RHIInstance::CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo)
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
        CD3D12Buffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

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
        CRHIIndexBuffer*  Buffer   = CreateInfo.IndexBuffer.Buffer;
        CD3D12Buffer* DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

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
        CRHIGenericBuffer* Buffer   = CreateInfo.StructuredBuffer.Buffer;
        CD3D12Buffer*     DxBuffer = D3D12BufferCast(Buffer);
        Resource = DxBuffer->GetResource();

        Assert(((Buffer->GetFlags() & EBufferUsageFlags::AllowUAV) != EBufferUsageFlags::None));

        Desc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement        = CreateInfo.StructuredBuffer.FirstElement;
        Desc.Buffer.NumElements         = CreateInfo.StructuredBuffer.NumElements;
        Desc.Format                     = DXGI_FORMAT_UNKNOWN;
        Desc.Buffer.Flags               = D3D12_BUFFER_UAV_FLAG_NONE;
        Desc.Buffer.StructureByteStride = Buffer->GetStride();
    }

    TSharedRef<CD3D12RHIUnorderedAccessView> DxView = dbg_new CD3D12RHIUnorderedAccessView(Device, ResourceOfflineDescriptorHeap);
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

CRHIRenderTargetView* CD3D12RHIInstance::CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo)
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

    TSharedRef<CD3D12RHIRenderTargetView> DxView = dbg_new CD3D12RHIRenderTargetView(Device, RenderTargetOfflineDescriptorHeap);
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

CRHIDepthStencilView* CD3D12RHIInstance::CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo)
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

    TSharedRef<CD3D12RHIDepthStencilView> DxView = dbg_new CD3D12RHIDepthStencilView(Device, DepthStencilOfflineDescriptorHeap);
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

CRHIComputeShader* CD3D12RHIInstance::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RHIComputeShader> Shader = dbg_new CD3D12RHIComputeShader(Device, ShaderCode);
    if (!Shader->Init())
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIVertexShader* CD3D12RHIInstance::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RHIVertexShader> Shader = dbg_new CD3D12RHIVertexShader(Device, ShaderCode);
    if (!CD3D12Shader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIHullShader* CD3D12RHIInstance::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIDomainShader* CD3D12RHIInstance::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIGeometryShader* CD3D12RHIInstance::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIMeshShader* CD3D12RHIInstance::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIAmplificationShader* CD3D12RHIInstance::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

CRHIPixelShader* CD3D12RHIInstance::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RHIPixelShader> Shader = dbg_new CD3D12RHIPixelShader(Device, ShaderCode);
    if (!CD3D12Shader::GetShaderReflection(Shader.Get()))
    {
        return nullptr;
    }

    return Shader.ReleaseOwnership();
}

CRHIRayGenShader* CD3D12RHIInstance::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RHIRayGenShader> Shader = dbg_new CD3D12RHIRayGenShader(Device, ShaderCode);
    if (!CD3D12RHIBaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayAnyHitShader* CD3D12RHIInstance::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RHIRayAnyHitShader> Shader = dbg_new CD3D12RHIRayAnyHitShader(Device, ShaderCode);
    if (!CD3D12RHIBaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayClosestHitShader* CD3D12RHIInstance::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RayClosestHitShader> Shader = dbg_new CD3D12RayClosestHitShader(Device, ShaderCode);
    if (!CD3D12RHIBaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIRayMissShader* CD3D12RHIInstance::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    TSharedRef<CD3D12RHIRayMissShader> Shader = dbg_new CD3D12RHIRayMissShader(Device, ShaderCode);
    if (!CD3D12RHIBaseRayTracingShader::GetRayTracingShaderReflection(Shader.Get()))
    {
        LOG_ERROR("[CD3D12RHIInterface]: Failed to retrieve Shader Identifier");
        return nullptr;
    }
    else
    {
        return Shader.ReleaseOwnership();
    }
}

CRHIDepthStencilState* CD3D12RHIInstance::CreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo)
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

    return dbg_new CD3D12RHIDepthStencilState(Device, Desc);
}

CRHIRasterizerState* CD3D12RHIInstance::CreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo)
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

    return dbg_new CD3D12RHIRasterizerState(Device, Desc);
}

CRHIBlendState* CD3D12RHIInstance::CreateBlendState(const SRHIBlendStateInfo& CreateInfo)
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

    return dbg_new CD3D12RHIBlendState(Device, Desc);
}

CRHIInputLayoutState* CD3D12RHIInstance::CreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo)
{
    return dbg_new CD3D12RHIInputLayoutState(Device, CreateInfo);
}

CRHIGraphicsPipelineState* CD3D12RHIInstance::CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo)
{
    TSharedRef<CD3D12RHIGraphicsPipelineState> NewPipelineState = dbg_new CD3D12RHIGraphicsPipelineState(Device);
    if (!NewPipelineState->Init(CreateInfo))
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIComputePipelineState* CD3D12RHIInstance::CreateComputePipelineState(const SRHIComputePipelineStateInfo& Info)
{
    Assert(Info.Shader != nullptr);

    TSharedRef<CD3D12RHIComputeShader>        Shader           = MakeSharedRef<CD3D12RHIComputeShader>(Info.Shader);
    TSharedRef<CD3D12RHIComputePipelineState> NewPipelineState = dbg_new CD3D12RHIComputePipelineState(Device, Shader);
    if (!NewPipelineState->Init())
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

CRHIRayTracingPipelineState* CD3D12RHIInstance::CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo)
{
    TSharedRef<CD3D12RHIRayTracingPipelineState> NewPipelineState = dbg_new CD3D12RHIRayTracingPipelineState(Device);
    if (NewPipelineState->Init(CreateInfo))
    {
        return NewPipelineState.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

CRHITimestampQuery* CD3D12RHIInstance::CreateTimestampQuery()
{
    return CD3D12RHITimestampQuery::Create(Device);
}

CRHIViewport* CD3D12RHIInstance::CreateViewport(CGenericWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat)
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

    TSharedRef<CD3D12RHIViewport> Viewport = dbg_new CD3D12RHIViewport(Device, DirectCmdContext, WinWindow->GetHandle(), ColorFormat, Width, Height);
    if (Viewport->Init())
    {
        return Viewport.ReleaseOwnership();
    }
    else
    {
        return nullptr;
    }
}

bool CD3D12RHIInstance::UAVSupportsFormat(EFormat Format) const
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

void CD3D12RHIInstance::CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const
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

void CD3D12RHIInstance::CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const
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
