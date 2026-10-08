#include "Core/Containers/UniquePtr.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Threading/ScopedLock.h"
#include "RHI/RHIStats.h"
#include "D3D11RHI/D3D11RHI.h"
#include "D3D11RHI/D3D11Buffer.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11DeviceDebug.h"
#include "D3D11RHI/D3D11Fence.h"
#include "D3D11RHI/D3D11Loader.h"
#include "D3D11RHI/D3D11PipelineState.h"
#include "D3D11RHI/D3D11Query.h"
#include "D3D11RHI/D3D11Shader.h"
#include "D3D11RHI/D3D11SwapChain.h"
#include "D3D11RHI/D3D11Texture.h"

IMPLEMENT_ENGINE_MODULE(FD3D11ModuleRHI, D3D11RHI);

DISABLE_UNREFERENCED_VARIABLE_WARNING

static bool IsBackBuffer(FRHIResource* InResource)
{
    return InResource && InResource->GetResourceType() == ERHIResourceType::Texture &&
        static_cast<FRHITexture*>(InResource)->GetDesc().IsPresentable();
}

FD3D11DeviceRHI* FD3D11DeviceRHI::D3D11DeviceRHI = nullptr;

FRHIDevice* FD3D11ModuleRHI::CreateDevice()
{
    TUniquePtr<FD3D11DeviceRHI> NewRHI = MakeUniquePtr<FD3D11DeviceRHI>();
    if (!NewRHI->Initialize())
    {
        return nullptr;
    }

    return NewRHI.Release();
}

FD3D11DeviceRHI::FD3D11DeviceRHI()
    : FRHIDevice()
    , Adapter(nullptr)
    , Device(nullptr)
    , CommandContext(nullptr)
    , FrameNumber(0)
{
    if (!D3D11DeviceRHI)
    {
        D3D11DeviceRHI = this;
    }
}

void FD3D11DeviceRHI::FlushDeferredDeletions()
{
    FD3D11DeviceRHI* DeviceRHI = Get();
    if (!DeviceRHI)
    {
        return;
    }

    // NOTE: Resources could contain other resources, that now need to be flushed
    if (FRHICommandListExecutor::IsInitialized())
    {
        FRHICommandListExecutor::Get().FlushDeletedResources();
    }

    while (!DeviceRHI->DeferredResources.IsEmpty())
    {
        TArray<FRHIResource*> Resources;
        {
            TScopedLock Lock(DeviceRHI->DeferredResourcesCS);
            Resources = Move(DeviceRHI->DeferredResources);
        }

        for (FRHIResource* Resource : Resources)
        {
            delete Resource;
        }

        // NOTE: Resources could contain other resources, that now need to be flushed
        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().FlushDeletedResources();
        }
    }
}

FD3D11DeviceRHI::~FD3D11DeviceRHI()
{
    if (CommandContext)
    {
        CommandContext->ClearState();
        CommandContext->Flush();
    }

    {
        TScopedLock Lock(SamplerStateMapCS);
        SamplerStateMap.Clear();
    }

    {
        TScopedLock Lock(DepthStencilStateMapCS);
        DepthStencilStateMap.Clear();
    }

    {
        TScopedLock Lock(RasterizerStateMapCS);
        RasterizerStateMap.Clear();
    }

    {
        TScopedLock Lock(BlendStateMapCS);
        BlendStateMap.Clear();
    }

    FlushDeferredDeletions();

    SAFE_DELETE(CommandContext);

    const bool bDebugLayerEnabled = Adapter ? Adapter->IsDebugLayerEnabled() : false;
    SAFE_DELETE(Device);
    SAFE_DELETE(Adapter);

    if (bDebugLayerEnabled)
    {
        D3D11Debug::ReportLiveDXGIObjects();
    }

    D3D11::Release();

    if (D3D11DeviceRHI == this)
    {
        D3D11DeviceRHI = nullptr;
    }
}

bool FD3D11DeviceRHI::Initialize()
{
    // Load Library and Function-Pointers etc.
    if (!D3D11::Initialize())
    {
        return false;
    }

    Adapter = new FD3D11Adapter();
    if (!Adapter->Initialize())
    {
        return false;
    }

    Device = new FD3D11Device(Adapter);
    if (!Device->Initialize())
    {
        return false;
    }

    CommandContext = new FD3D11CommandContext(Device);
    if (!CommandContext->Initialize())
    {
        return false;
    }

    return InitializeDeviceFeatureSupport();
}

void FD3D11DeviceRHI::BeginFrame()
{
    FrameNumber++;
}

void FD3D11DeviceRHI::EndFrame()
{
    FlushDeferredDeletions();

    Device->FlushDebugMessages();

#if D3D11_ENABLE_STATS
    {
        FRHIVideoMemoryInfo LocalMemory;
        if (QueryVideoMemoryInfo(EVideoMemoryType::Local, LocalMemory))
        {
            STAT_SET(STAT_RHI_LocalMemoryBudget, LocalMemory.MemoryBudget);
            STAT_SET(STAT_RHI_LocalMemoryUsage,  LocalMemory.MemoryUsage);
        }

        FRHIVideoMemoryInfo NonLocalMemory;
        if (QueryVideoMemoryInfo(EVideoMemoryType::NonLocal, NonLocalMemory))
        {
            STAT_SET(STAT_RHI_NonLocalMemoryBudget, NonLocalMemory.MemoryBudget);
            STAT_SET(STAT_RHI_NonLocalMemoryUsage,  NonLocalMemory.MemoryUsage);
        }
    }
#endif
}

FRHITexture* FD3D11DeviceRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData)
{
    FD3D11TextureRHIRef NewTexture = new FD3D11TextureRHI(GetDevice(), InTextureDesc);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }

#if D3D11_ENABLE_STATS
    {
        const int64 AllocatedSize = static_cast<int64>(NewTexture->GetAllocationSize());
        if (InTextureDesc.IsRenderTarget() || InTextureDesc.IsDepthStencil())
        {
            STAT_ADD(STAT_RHI_RenderTargetMemory, AllocatedSize);
        }
        else
        {
            STAT_ADD(STAT_RHI_TextureMemory, AllocatedSize);
        }
    }
#endif

    return NewTexture.ReleaseOwnership();
}

FRHIBuffer* FD3D11DeviceRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData)
{
    FD3D11BufferRHIRef NewBuffer = new FD3D11BufferRHI(GetDevice(), InBufferDesc);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }

#if D3D11_ENABLE_STATS
    {
        const int64 AllocatedSize = static_cast<int64>(NewBuffer->GetAllocationSize());
        if (InBufferDesc.IsVertexBuffer())
        {
            STAT_ADD(STAT_RHI_VertexBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsIndexBuffer())
        {
            STAT_ADD(STAT_RHI_IndexBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsConstantBuffer())
        {
            STAT_ADD(STAT_RHI_ConstantBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsShaderResourceBuffer() || InBufferDesc.IsUnorderedAccessBuffer())
        {
            STAT_ADD(STAT_RHI_StructuredBufferMemory, AllocatedSize);
        }
        else
        {
            STAT_ADD(STAT_RHI_MiscBufferMemory, AllocatedSize);
        }

        if (InBufferDesc.IsReadBack())
        {
            STAT_ADD(STAT_RHI_ReadbackMemory, AllocatedSize);
        }
        if (InBufferDesc.IsDynamic() || InBufferDesc.IsTransient())
        {
            STAT_ADD(STAT_RHI_UploadMemory, AllocatedSize);
        }
    }
#endif

    return NewBuffer.ReleaseOwnership();
}

FRHISamplerState* FD3D11DeviceRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
{
    TScopedLock Lock(SamplerStateMapCS);

    if (FD3D11SamplerStateRHIRef* ExistingSamplerState = SamplerStateMap.Find(InSamplerDesc))
    {
        FD3D11SamplerStateRHIRef Result = *ExistingSamplerState;
        return Result.ReleaseOwnership();
    }

    FD3D11SamplerStateRHIRef NewSamplerState = new FD3D11SamplerStateRHI(GetDevice(), InSamplerDesc);
    if (!NewSamplerState->Initialize())
    {
        return nullptr;
    }

    SamplerStateMap.Add(InSamplerDesc, NewSamplerState);
    return NewSamplerState.ReleaseOwnership();
}

FRHISwapChain* FD3D11DeviceRHI::CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
{
    CHECK(InSwapChainDesc.WindowHandle != nullptr);

    if (!Tasks::IsInRHIThread())
    {
        FRHISwapChain* NewSwapChain = nullptr;
        Tasks::LaunchOnRHIThread("D3D11CreateSwapChain", [this, &NewSwapChain, &InSwapChainDesc]()
        {
            NewSwapChain = CreateSwapChain(InSwapChainDesc);
        }).Wait();

        return NewSwapChain;
    }

    FD3D11SwapChainRHIRef NewSwapChain = new FD3D11SwapChainRHI(Device, CommandContext, InSwapChainDesc);
    if (!NewSwapChain->Initialize())
    {
        return nullptr;
    }

    return NewSwapChain.ReleaseOwnership();
}

FRHIQuery* FD3D11DeviceRHI::CreateQuery(EQueryType InQueryType)
{
    FD3D11QueryRHIRef NewQuery = new FD3D11QueryRHI(GetDevice(), InQueryType);
    if (!NewQuery->Initialize())
    {
        return nullptr;
    }

    return NewQuery.ReleaseOwnership();
}

FRHIFence* FD3D11DeviceRHI::CreateFence()
{
    FD3D11FenceRHIRef NewFence = new FD3D11FenceRHI(GetDevice());
    if (!NewFence->Initialize())
    {
        return nullptr;
    }

    return NewFence.ReleaseOwnership();
}

FRHIShaderResourceView* FD3D11DeviceRHI::CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
{
    if (!InResource)
    {
        D3D11_ERROR("CreateShaderResourceView requires a non-null resource");
        return nullptr;
    }

    if (IsBackBuffer(InResource))
    {
        D3D11_ERROR("CreateShaderResourceView: cannot create a view from the back-buffer.");
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC Desc = {};

    ID3D11Resource* D3D11Resource = nullptr;
    if (InDesc.IsBufferSRV())
    {
        D3D11_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Buffer,
            "CreateShaderResourceView: buffer view requires an FRHIBuffer resource");

        FD3D11BufferRHI* D3D11Buffer = FD3D11DeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(InResource));
        CHECK(D3D11Buffer != nullptr);

        D3D11Resource = D3D11Buffer->GetD3D11Resource();

        const auto& BufferDesc = InDesc.Buffer;
        switch (BufferDesc.Type)
        {
            case EBufferViewType::Typed:
            {
                // Buffer<T>: A real format-typed buffer view
                Desc.Format         = D3D11CastShaderResourceFormat(ConvertFormat(BufferDesc.Format));
                Desc.BufferEx.Flags = 0;
                break;
            }

            case EBufferViewType::ByteAddress:
            {
                // ByteAddressBuffer: A raw R32-typeless view, addressed in 4-byte units. The buffer needs ALLOW_RAW_VIEWS.
                Desc.Format         = DXGI_FORMAT_R32_TYPELESS;
                Desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
                break;
            }

            case EBufferViewType::Structured:
            {
                // StructuredBuffer<T>: UNKNOWN format, the stride comes from the buffer, which needs to be created STRUCTURED
                Desc.Format         = DXGI_FORMAT_UNKNOWN;
                Desc.BufferEx.Flags = 0;
                break;
            }

            default:
            {
                D3D11_ERROR("Unsupported EBufferViewType for buffer SRV");
                return nullptr;
            }
        }

        Desc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFEREX;
        Desc.BufferEx.FirstElement = BufferDesc.FirstElement;
        Desc.BufferEx.NumElements  = BufferDesc.NumElements;
    }
    else if (InDesc.IsTextureSRV())
    {
        D3D11_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
            "CreateShaderResourceView: texture view requires an FRHITexture resource");

        FD3D11TextureRHI* D3D11Texture = FD3D11DeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
        CHECK(D3D11Texture != nullptr);
        CHECK(IsViewDimensionCompatible(D3D11Texture->GetDesc().Dimension, InDesc.ViewDimension));

        D3D11Resource = D3D11Texture->GetD3D11Resource();

        const bool bIsMultisampled = D3D11Texture->GetDesc().IsMultisampled();
        switch (InDesc.ViewDimension)
        {
            case EViewDimension::Texture1D:
            {
                const auto& TextureDesc        = InDesc.Texture1D;
                Desc.Format                    = D3D11CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE1D;
                Desc.Texture1D.MostDetailedMip = TextureDesc.FirstMipLevel;
                Desc.Texture1D.MipLevels       = TextureDesc.NumMips;
                break;
            }

            case EViewDimension::Texture1DArray:
            {
                const auto& TextureDesc             = InDesc.Texture1DArray;
                Desc.Format                         = D3D11CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                Desc.Texture1DArray.MostDetailedMip = TextureDesc.FirstMipLevel;
                Desc.Texture1DArray.MipLevels       = TextureDesc.NumMips;
                Desc.Texture1DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                Desc.Texture1DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::Texture2D:
            {
                const auto& TextureDesc = InDesc.Texture2D;
                Desc.Format = D3D11CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));

                if (!bIsMultisampled)
                {
                    Desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                    Desc.Texture2D.MostDetailedMip = TextureDesc.FirstMipLevel;
                    Desc.Texture2D.MipLevels       = TextureDesc.NumMips;
                }
                else
                {
                    Desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                }

                break;
            }

            case EViewDimension::Texture2DArray:
            {
                const auto& TextureDesc = InDesc.Texture2DArray;
                Desc.Format = D3D11CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));

                if (!bIsMultisampled)
                {
                    Desc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    Desc.Texture2DArray.MostDetailedMip = TextureDesc.FirstMipLevel;
                    Desc.Texture2DArray.MipLevels       = TextureDesc.NumMips;
                    Desc.Texture2DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                    Desc.Texture2DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                }
                else
                {
                    Desc.ViewDimension                    = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    Desc.Texture2DMSArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                    Desc.Texture2DMSArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                }

                break;
            }

            case EViewDimension::TextureCube:
            {
                const auto& TextureDesc          = InDesc.TextureCube;
                Desc.Format                      = D3D11CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
                Desc.TextureCube.MostDetailedMip = TextureDesc.FirstMipLevel;
                Desc.TextureCube.MipLevels       = TextureDesc.NumMips;
                break;
            }

            case EViewDimension::TextureCubeArray:
            {
                const auto& TextureDesc                = InDesc.TextureCubeArray;
                Desc.Format                            = D3D11CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                     = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                Desc.TextureCubeArray.MostDetailedMip  = TextureDesc.FirstMipLevel;
                Desc.TextureCubeArray.MipLevels        = TextureDesc.NumMips;
                Desc.TextureCubeArray.First2DArrayFace = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
                Desc.TextureCubeArray.NumCubes         = Math::Max<uint16>(TextureDesc.NumCubes, 1u);
                break;
            }

            case EViewDimension::Texture3D:
            {
                const auto& TextureDesc        = InDesc.Texture3D;
                Desc.Format                    = D3D11CastShaderResourceFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
                Desc.Texture3D.MostDetailedMip = TextureDesc.FirstMipLevel;
                Desc.Texture3D.MipLevels       = TextureDesc.NumMips;
                break;
            }

            default:
            {
                D3D11_ERROR("CreateShaderResourceView: unsupported texture ViewDimension");
                return nullptr;
            }
        }
    }
    else
    {
        D3D11_ERROR("CreateShaderResourceView: D3D11 only has buffer and texture ShaderResourceViews");
        return nullptr;
    }

    CHECK(D3D11Resource != nullptr);

    FD3D11ShaderResourceViewRHIRef D3D11View = new FD3D11ShaderResourceViewRHI(GetDevice(), InResource, InDesc);
    if (!D3D11View->Initialize(D3D11Resource, Desc))
    {
        return nullptr;
    }

    return D3D11View.ReleaseOwnership();
}

FRHIUnorderedAccessView* FD3D11DeviceRHI::CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (!InResource)
    {
        D3D11_ERROR("CreateUnorderedAccessView requires a non-null resource");
        return nullptr;
    }

    if (IsBackBuffer(InResource))
    {
        D3D11_ERROR("CreateUnorderedAccessView: cannot create a view from the back-buffer.");
        return nullptr;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC Desc = {};

    ID3D11Resource* D3D11Resource = nullptr;
    if (InDesc.IsBufferUAV())
    {
        D3D11_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Buffer,
            "CreateUnorderedAccessView: buffer view requires an FRHIBuffer resource");

        FD3D11BufferRHI* D3D11Buffer = FD3D11DeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(InResource));
        CHECK(D3D11Buffer != nullptr);

        D3D11Resource = D3D11Buffer->GetD3D11Resource();

        const auto& BufferDesc = InDesc.Buffer;
        switch (BufferDesc.Type)
        {
            case EBufferViewType::Typed:
            {
                // RWBuffer<T>: A real format-typed buffer view
                Desc.Format       = D3D11CastUnorderedAccessFormat(ConvertFormat(BufferDesc.Format));
                Desc.Buffer.Flags = 0;
                break;
            }

            case EBufferViewType::ByteAddress:
            {
                // RWByteAddressBuffer: A raw R32-typeless view, addressed in 4-byte units. The buffer needs ALLOW_RAW_VIEWS.
                Desc.Format       = DXGI_FORMAT_R32_TYPELESS;
                Desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
                break;
            }

            case EBufferViewType::Structured:
            {
                // RWStructuredBuffer<T>: UNKNOWN format, the stride comes from the buffer, which needs to be created STRUCTURED
                Desc.Format       = DXGI_FORMAT_UNKNOWN;
                Desc.Buffer.Flags = 0;
                break;
            }

            default:
            {
                D3D11_ERROR("Unsupported EBufferViewType for buffer UAV");
                return nullptr;
            }
        }

        Desc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
        Desc.Buffer.FirstElement = BufferDesc.FirstElement;
        Desc.Buffer.NumElements  = BufferDesc.NumElements;
    }
    else if (InDesc.IsTextureUAV())
    {
        D3D11_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
            "CreateUnorderedAccessView: texture view requires an FRHITexture resource");

        FD3D11TextureRHI* D3D11Texture = FD3D11DeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
        CHECK(D3D11Texture != nullptr);
        CHECK(IsViewDimensionCompatible(D3D11Texture->GetDesc().Dimension, InDesc.ViewDimension));

        D3D11Resource = D3D11Texture->GetD3D11Resource();

        switch (InDesc.ViewDimension)
        {
            case EViewDimension::Texture1D:
            {
                const auto& TextureDesc = InDesc.Texture1D;
                Desc.Format             = D3D11CastUnorderedAccessFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE1D;
                Desc.Texture1D.MipSlice = TextureDesc.MipLevel;
                break;
            }

            case EViewDimension::Texture1DArray:
            {
                const auto& TextureDesc             = InDesc.Texture1DArray;
                Desc.Format                         = D3D11CastUnorderedAccessFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                  = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                Desc.Texture1DArray.MipSlice        = TextureDesc.MipLevel;
                Desc.Texture1DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                Desc.Texture1DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::Texture2D:
            {
                const auto& TextureDesc = InDesc.Texture2D;
                Desc.Format             = D3D11CastUnorderedAccessFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
                Desc.Texture2D.MipSlice = TextureDesc.MipLevel;
                break;
            }

            case EViewDimension::Texture2DArray:
            {
                const auto& TextureDesc             = InDesc.Texture2DArray;
                Desc.Format                         = D3D11CastUnorderedAccessFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                Desc.Texture2DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                Desc.Texture2DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
                break;
            }

            case EViewDimension::TextureCube:
            {
                const auto& TextureDesc             = InDesc.TextureCube;
                Desc.Format                         = D3D11CastUnorderedAccessFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                Desc.Texture2DArray.FirstArraySlice = 0;
                Desc.Texture2DArray.ArraySize       = RHI_NUM_CUBE_FACES;
                break;
            }

            case EViewDimension::TextureCubeArray:
            {
                const auto& TextureDesc             = InDesc.TextureCubeArray;
                Desc.Format                         = D3D11CastUnorderedAccessFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension                  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                Desc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                Desc.Texture2DArray.FirstArraySlice = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
                Desc.Texture2DArray.ArraySize       = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));
                break;
            }

            case EViewDimension::Texture3D:
            {
                const auto& TextureDesc    = InDesc.Texture3D;
                Desc.Format                = D3D11CastUnorderedAccessFormat(ConvertFormat(TextureDesc.Format));
                Desc.ViewDimension         = D3D11_UAV_DIMENSION_TEXTURE3D;
                Desc.Texture3D.MipSlice    = TextureDesc.MipLevel;
                Desc.Texture3D.FirstWSlice = TextureDesc.FirstWSlice;
                Desc.Texture3D.WSize       = Math::Max<uint16>(TextureDesc.WSize, 1u);
                break;
            }

            default:
            {
                D3D11_ERROR("CreateUnorderedAccessView: unsupported texture ViewDimension");
                return nullptr;
            }
        }
    }
    else
    {
        D3D11_ERROR("CreateUnorderedAccessView: D3D11 only has buffer and texture UnorderedAccessViews");
        return nullptr;
    }

    CHECK(D3D11Resource != nullptr);

    FD3D11UnorderedAccessViewRHIRef D3D11View = new FD3D11UnorderedAccessViewRHI(GetDevice(), InResource, InDesc);
    if (!D3D11View->Initialize(D3D11Resource, Desc))
    {
        return nullptr;
    }

    return D3D11View.ReleaseOwnership();
}

FRHIRenderTargetView* FD3D11DeviceRHI::CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
{
    if (!InResource)
    {
        D3D11_WARNING("Cannot create RenderTargetView without a valid resource");
        return nullptr;
    }

    if (IsBackBuffer(InResource))
    {
        D3D11_ERROR("CreateRenderTargetView: cannot create a view from the back-buffer.");
        return nullptr;
    }

    D3D11_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
        "CreateRenderTargetView: requires an FRHITexture resource");

    FD3D11TextureRHI* D3D11Texture = FD3D11DeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
    if (!D3D11Texture || !D3D11Texture->GetD3D11Resource())
    {
        D3D11_WARNING("Cannot create RenderTargetView without a valid texture");
        return nullptr;
    }

    CHECK(IsViewDimensionCompatible(D3D11Texture->GetDesc().Dimension, InDesc.ViewDimension));

    if (!D3D11Texture->GetDesc().IsRenderTarget())
    {
        String DebugName;
        D3D11Texture->GetDebugName(DebugName);
        D3D11_ERROR("Texture '%s' does not allow RenderTargetViews", *DebugName);
        return nullptr;
    }

    const bool bIsMultisampled = D3D11Texture->GetDesc().IsMultisampled();

    D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
        {
            const auto& TextureDesc    = InDesc.Texture1D;
            RTVDesc.Format             = D3D11CastRenderTargetFormat(ConvertFormat(TextureDesc.Format));
            RTVDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE1D;
            RTVDesc.Texture1D.MipSlice = TextureDesc.MipLevel;
            break;
        }

        case EViewDimension::Texture1DArray:
        {
            const auto& TextureDesc                = InDesc.Texture1DArray;
            RTVDesc.Format                         = D3D11CastRenderTargetFormat(ConvertFormat(TextureDesc.Format));
            RTVDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
            RTVDesc.Texture1DArray.MipSlice        = TextureDesc.MipLevel;
            RTVDesc.Texture1DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
            RTVDesc.Texture1DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            break;
        }

        case EViewDimension::Texture2D:
        {
            const auto& TextureDesc = InDesc.Texture2D;
            RTVDesc.Format = D3D11CastRenderTargetFormat(ConvertFormat(TextureDesc.Format));

            if (!bIsMultisampled)
            {
                RTVDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
                RTVDesc.Texture2D.MipSlice = TextureDesc.MipLevel;
            }
            else
            {
                RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            }

            break;
        }

        case EViewDimension::Texture2DArray:
        {
            const auto& TextureDesc = InDesc.Texture2DArray;
            RTVDesc.Format = D3D11CastRenderTargetFormat(ConvertFormat(TextureDesc.Format));

            if (!bIsMultisampled)
            {
                RTVDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                RTVDesc.Texture2DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                RTVDesc.Texture2DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            }
            else
            {
                RTVDesc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                RTVDesc.Texture2DMSArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            }

            break;
        }

        case EViewDimension::TextureCube:
        {
            const auto& TextureDesc = InDesc.TextureCube;
            RTVDesc.Format = D3D11CastRenderTargetFormat(ConvertFormat(TextureDesc.Format));

            if (!bIsMultisampled)
            {
                RTVDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                RTVDesc.Texture2DArray.FirstArraySlice = 0;
                RTVDesc.Texture2DArray.ArraySize       = RHI_NUM_CUBE_FACES;
            }
            else
            {
                RTVDesc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.FirstArraySlice = 0;
                RTVDesc.Texture2DMSArray.ArraySize       = RHI_NUM_CUBE_FACES;
            }

            break;
        }

        case EViewDimension::TextureCubeArray:
        {
            const auto& TextureDesc = InDesc.TextureCubeArray;
            RTVDesc.Format = D3D11CastRenderTargetFormat(ConvertFormat(TextureDesc.Format));

            const uint32 FirstLayer = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
            const uint32 NumLayers  = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));

            if (!bIsMultisampled)
            {
                RTVDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                RTVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                RTVDesc.Texture2DArray.FirstArraySlice = FirstLayer;
                RTVDesc.Texture2DArray.ArraySize       = NumLayers;
            }
            else
            {
                RTVDesc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                RTVDesc.Texture2DMSArray.FirstArraySlice = FirstLayer;
                RTVDesc.Texture2DMSArray.ArraySize       = NumLayers;
            }

            break;
        }

        case EViewDimension::Texture3D:
        {
            const auto& TextureDesc       = InDesc.Texture3D;
            RTVDesc.Format                = D3D11CastRenderTargetFormat(ConvertFormat(TextureDesc.Format));
            RTVDesc.ViewDimension         = D3D11_RTV_DIMENSION_TEXTURE3D;
            RTVDesc.Texture3D.MipSlice    = TextureDesc.MipLevel;
            RTVDesc.Texture3D.FirstWSlice = TextureDesc.FirstWSlice;
            RTVDesc.Texture3D.WSize       = Math::Max<uint16>(TextureDesc.WSize, 1u);
            break;
        }

        default:
        {
            D3D11_ERROR("CreateRenderTargetView: unsupported ViewDimension");
            return nullptr;
        }
    }

    D3D11_ERROR_COND(RTVDesc.Format != DXGI_FORMAT_UNKNOWN, "Unallowed format for RenderTargetViews");

    FD3D11RenderTargetViewRHIRef D3D11View = new FD3D11RenderTargetViewRHI(GetDevice(), D3D11Texture, InDesc);
    if (!D3D11View->Initialize(D3D11Texture->GetD3D11Resource(), RTVDesc))
    {
        return nullptr;
    }

    return D3D11View.ReleaseOwnership();
}

FRHIDepthStencilView* FD3D11DeviceRHI::CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
{
    if (!InResource)
    {
        D3D11_WARNING("Cannot create DepthStencilView without a valid resource");
        return nullptr;
    }

    if (IsBackBuffer(InResource))
    {
        D3D11_ERROR("CreateDepthStencilView: cannot create a view from the back-buffer.");
        return nullptr;
    }

    D3D11_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
        "CreateDepthStencilView: requires an FRHITexture resource");

    FD3D11TextureRHI* D3D11Texture = FD3D11DeviceRHI::ResourceCast(static_cast<FRHITexture*>(InResource));
    if (!D3D11Texture || !D3D11Texture->GetD3D11Resource())
    {
        D3D11_WARNING("Cannot create DepthStencilView without a valid texture");
        return nullptr;
    }

    CHECK(IsViewDimensionCompatible(D3D11Texture->GetDesc().Dimension, InDesc.ViewDimension));

    if (!D3D11Texture->GetDesc().IsDepthStencil())
    {
        String DebugName;
        D3D11Texture->GetDebugName(DebugName);
        D3D11_ERROR("Texture '%s' does not allow DepthStencilViews", *DebugName);
        return nullptr;
    }

    const bool bIsMultisampled = D3D11Texture->GetDesc().IsMultisampled();

    D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
        {
            const auto& TextureDesc    = InDesc.Texture1D;
            DSVDesc.Format             = D3D11CastDepthStencilFormat(ConvertFormat(TextureDesc.Format));
            DSVDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE1D;
            DSVDesc.Texture1D.MipSlice = TextureDesc.MipLevel;
            break;
        }

        case EViewDimension::Texture1DArray:
        {
            const auto& TextureDesc                = InDesc.Texture1DArray;
            DSVDesc.Format                         = D3D11CastDepthStencilFormat(ConvertFormat(TextureDesc.Format));
            DSVDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
            DSVDesc.Texture1DArray.MipSlice        = TextureDesc.MipLevel;
            DSVDesc.Texture1DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
            DSVDesc.Texture1DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            break;
        }

        case EViewDimension::Texture2D:
        {
            const auto& TextureDesc = InDesc.Texture2D;
            DSVDesc.Format = D3D11CastDepthStencilFormat(ConvertFormat(TextureDesc.Format));

            if (!bIsMultisampled)
            {
                DSVDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
                DSVDesc.Texture2D.MipSlice = TextureDesc.MipLevel;
            }
            else
            {
                DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
            }

            break;
        }

        case EViewDimension::Texture2DArray:
        {
            const auto& TextureDesc = InDesc.Texture2DArray;
            DSVDesc.Format = D3D11CastDepthStencilFormat(ConvertFormat(TextureDesc.Format));

            if (!bIsMultisampled)
            {
                DSVDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                DSVDesc.Texture2DArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                DSVDesc.Texture2DArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            }
            else
            {
                DSVDesc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.FirstArraySlice = TextureDesc.FirstArraySlice;
                DSVDesc.Texture2DMSArray.ArraySize       = Math::Max<uint16>(TextureDesc.NumSlices, 1u);
            }

            break;
        }

        case EViewDimension::TextureCube:
        {
            const auto& TextureDesc = InDesc.TextureCube;
            DSVDesc.Format = D3D11CastDepthStencilFormat(ConvertFormat(TextureDesc.Format));

            if (!bIsMultisampled)
            {
                DSVDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                DSVDesc.Texture2DArray.FirstArraySlice = 0;
                DSVDesc.Texture2DArray.ArraySize       = RHI_NUM_CUBE_FACES;
            }
            else
            {
                DSVDesc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.FirstArraySlice = 0;
                DSVDesc.Texture2DMSArray.ArraySize       = RHI_NUM_CUBE_FACES;
            }

            break;
        }

        case EViewDimension::TextureCubeArray:
        {
            const auto& TextureDesc = InDesc.TextureCubeArray;
            DSVDesc.Format = D3D11CastDepthStencilFormat(ConvertFormat(TextureDesc.Format));

            const uint32 FirstLayer = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, TextureDesc.FirstCube);
            const uint32 NumLayers  = RHICubesToArrayLayers(ETextureDimension::TextureCubeArray, Math::Max<uint16>(TextureDesc.NumCubes, 1u));

            if (!bIsMultisampled)
            {
                DSVDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                DSVDesc.Texture2DArray.MipSlice        = TextureDesc.MipLevel;
                DSVDesc.Texture2DArray.FirstArraySlice = FirstLayer;
                DSVDesc.Texture2DArray.ArraySize       = NumLayers;
            }
            else
            {
                DSVDesc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                DSVDesc.Texture2DMSArray.FirstArraySlice = FirstLayer;
                DSVDesc.Texture2DMSArray.ArraySize       = NumLayers;
            }

            break;
        }

        default:
        {
            D3D11_ERROR("CreateDepthStencilView: unsupported ViewDimension");
            return nullptr;
        }
    }

    if (DSVDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
        D3D11_ERROR("Unallowed format for DepthStencilViews");
        return nullptr;
    }

    DSVDesc.Flags = 0;
    if (IsEnumFlagSet(InDesc.Flags, EDepthStencilViewFlags::ReadOnlyDepth))
    {
        DSVDesc.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
    }

    if (IsEnumFlagSet(InDesc.Flags, EDepthStencilViewFlags::ReadOnlyStencil))
    {
        DSVDesc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;
    }

    FD3D11DepthStencilViewRHIRef D3D11View = new FD3D11DepthStencilViewRHI(GetDevice(), D3D11Texture, InDesc);
    if (!D3D11View->Initialize(D3D11Texture->GetD3D11Resource(), DSVDesc))
    {
        return nullptr;
    }

    return D3D11View.ReleaseOwnership();
}

template<typename ShaderType>
ShaderType* FD3D11DeviceRHI::CreateD3D11Shader(EShaderStage Stage, const TArray<uint8>& ShaderCode)
{
    FShaderCodeView CodeView;
    if (!FShaderCodeReader::Read(ShaderCode, CodeView) || CodeView.GetStage() != Stage || CodeView.GetOutputLanguage() != EShaderOutputLanguage::DXBC)
    {
        D3D11_ERROR("[FD3D11DeviceRHI]: The shader code is not a valid DXBC %s shader", ToString(Stage));
        return nullptr;
    }

    TSharedRef<ShaderType> NewShader = new ShaderType(GetDevice());
    if (!NewShader->Initialize(CodeView))
    {
        return nullptr;
    }

    if constexpr (TIsBaseOf<FRHIVertexShader, ShaderType>::Value)
    {
        NewShader->SetVertexInputs(CodeView.GetVertexInputs());
    }

    return NewShader.ReleaseOwnership();
}

FRHIComputeShader* FD3D11DeviceRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return CreateD3D11Shader<FD3D11ComputeShaderRHI>(EShaderStage::Compute, ShaderCode);
}

FRHIVertexShader* FD3D11DeviceRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return CreateD3D11Shader<FD3D11VertexShaderRHI>(EShaderStage::Vertex, ShaderCode);
}

FRHIHullShader* FD3D11DeviceRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    return CreateD3D11Shader<FD3D11HullShaderRHI>(EShaderStage::Hull, ShaderCode);
}

FRHIDomainShader* FD3D11DeviceRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return CreateD3D11Shader<FD3D11DomainShaderRHI>(EShaderStage::Domain, ShaderCode);
}

FRHIGeometryShader* FD3D11DeviceRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return CreateD3D11Shader<FD3D11GeometryShaderRHI>(EShaderStage::Geometry, ShaderCode);
}

FRHIPixelShader* FD3D11DeviceRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return CreateD3D11Shader<FD3D11PixelShaderRHI>(EShaderStage::Pixel, ShaderCode);
}

FRHIDepthStencilState* FD3D11DeviceRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    TScopedLock Lock(DepthStencilStateMapCS);

    if (FD3D11DepthStencilStateRHIRef* ExistingState = DepthStencilStateMap.Find(InDesc))
    {
        FD3D11DepthStencilStateRHIRef Result = *ExistingState;
        return Result.ReleaseOwnership();
    }

    FD3D11DepthStencilStateRHIRef NewState = new FD3D11DepthStencilStateRHI(GetDevice(), InDesc);
    if (!NewState->Initialize())
    {
        return nullptr;
    }

    DepthStencilStateMap.Add(InDesc, NewState);
    return NewState.ReleaseOwnership();
}

FRHIRasterizerState* FD3D11DeviceRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    TScopedLock Lock(RasterizerStateMapCS);

    if (FD3D11RasterizerStateRHIRef* ExistingState = RasterizerStateMap.Find(InDesc))
    {
        FD3D11RasterizerStateRHIRef Result = *ExistingState;
        return Result.ReleaseOwnership();
    }

    FD3D11RasterizerStateRHIRef NewState = new FD3D11RasterizerStateRHI(GetDevice(), InDesc);
    if (!NewState->Initialize())
    {
        return nullptr;
    }

    RasterizerStateMap.Add(InDesc, NewState);
    return NewState.ReleaseOwnership();
}

FRHIBlendState* FD3D11DeviceRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    TScopedLock Lock(BlendStateMapCS);

    if (FD3D11BlendStateRHIRef* ExistingState = BlendStateMap.Find(InDesc))
    {
        FD3D11BlendStateRHIRef Result = *ExistingState;
        return Result.ReleaseOwnership();
    }

    FD3D11BlendStateRHIRef NewState = new FD3D11BlendStateRHI(GetDevice(), InDesc);
    if (!NewState->Initialize())
    {
        return nullptr;
    }

    BlendStateMap.Add(InDesc, NewState);
    return NewState.ReleaseOwnership();
}

FRHIInputLayout* FD3D11DeviceRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FD3D11InputLayoutRHI(InInputElements);
}

FRHIGraphicsPipelineState* FD3D11DeviceRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    FD3D11GraphicsPipelineStateRHIRef NewPipelineState = new FD3D11GraphicsPipelineStateRHI(GetDevice());
    if (!NewPipelineState->Initialize(InDesc))
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

FRHIComputePipelineState* FD3D11DeviceRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    FD3D11ComputePipelineStateRHIRef NewPipelineState = new FD3D11ComputePipelineStateRHI(GetDevice());
    if (!NewPipelineState->Initialize(InDesc))
    {
        return nullptr;
    }

    return NewPipelineState.ReleaseOwnership();
}

IRHICommandContext* FD3D11DeviceRHI::ObtainCommandContext()
{
    return CommandContext;
}

bool FD3D11DeviceRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const
{
    if (!Adapter)
    {
        return false;
    }

    const DXGI_MEMORY_SEGMENT_GROUP MemoryGroup = MemoryType == EVideoMemoryType::Local ?
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL :
        DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL;

    DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemoryInfo;
    HRESULT hr = Adapter->GetDXGIAdapter3()->QueryVideoMemoryInfo(0, MemoryGroup, &VideoMemoryInfo);
    if (FAILED(hr))
    {
        D3D11_ERROR("[FD3D11DeviceRHI] QueryVideoMemoryInfo failed");
        return false;
    }

    OutMemoryInfo.MemoryType   = MemoryType;
    OutMemoryInfo.MemoryUsage  = VideoMemoryInfo.CurrentUsage;
    OutMemoryInfo.MemoryBudget = VideoMemoryInfo.Budget;
    return true;
}

bool FD3D11DeviceRHI::QueryUAVFormatSupport(EFormat Format) const
{
    ID3D11Device* D3D11Device = Device->GetD3D11Device();

    D3D11_FEATURE_DATA_D3D11_OPTIONS2 FeatureData = {};
    if (SUCCEEDED(D3D11Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &FeatureData, sizeof(FeatureData))))
    {
        if (FeatureData.TypedUAVLoadAdditionalFormats)
        {
            D3D11_FEATURE_DATA_FORMAT_SUPPORT2 FormatSupport = {};
            FormatSupport.InFormat = ConvertFormat(Format);

            const HRESULT Result = D3D11Device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &FormatSupport, sizeof(FormatSupport));
            if (FAILED(Result) || (FormatSupport.OutFormatSupport2 & D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0)
            {
                return false;
            }
        }
    }

    return true;
}

bool FD3D11DeviceRHI::QuerySupportedSampleCounts(EFormat Format, uint32& OutSampleCounts) const
{
    OutSampleCounts = 0;

    const DXGI_FORMAT DxgiFormat = ConvertFormat(Format);
    if (DxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        return false;
    }

    for (uint32 SampleCount = 1; SampleCount <= D3D11_MAX_RESOURCE_SAMPLE_COUNT; SampleCount <<= 1)
    {
        uint32 Quality = 0;
        if (Device->QueryMultisampleQuality(DxgiFormat, SampleCount, Quality))
        {
            OutSampleCounts |= SampleCount;
        }
    }

    return OutSampleCounts != 0;
}

bool FD3D11DeviceRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    FD3D11QueryRHI* D3D11Query = FD3D11DeviceRHI::ResourceCast(Query);
    if (!D3D11Query)
    {
        return false;
    }

    if (!D3D11Query->ResolveResult(Mode))
    {
        return false;
    }

    OutResult = *D3D11Query->QueryResult;
    return true;
}

bool FD3D11DeviceRHI::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    FD3D11QueryRHI* D3D11Query = FD3D11DeviceRHI::ResourceCast(Query);
    if (!D3D11Query)
    {
        return false;
    }

    if (!D3D11Query->ResolveResult(Mode))
    {
        return false;
    }

    OutResult = *reinterpret_cast<const FRHIPipelineStatistics*>(D3D11Query->QueryResult);
    return true;
}

void FD3D11DeviceRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    if (Resource)
    {
        TScopedLock Lock(DeferredResourcesCS);
        DeferredResources.Add(Resource);
    }
}

void* FD3D11DeviceRHI::GetRHINativeAdapter()
{
    CHECK(Adapter != nullptr);
    return reinterpret_cast<void*>(Adapter->GetDXGIAdapter());
}

void* FD3D11DeviceRHI::GetRHINativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetD3D11Device());
}

String FD3D11DeviceRHI::GetAdapterName() const
{
    CHECK(Adapter != nullptr);
    return Adapter->GetDescription();
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
