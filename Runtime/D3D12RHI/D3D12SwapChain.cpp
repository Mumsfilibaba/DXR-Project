#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include "D3D12RHI/D3D12Composition.h"
#include "D3D12RHI/D3D12DeviceDebug.h"

static TAutoConsoleVariable<int32> CVarSwapChainBackBufferCount(
    "D3D12RHI.SwapChain.BackBufferCount",
    "Number of swap chain back buffers",
    D3D12_NUM_BACK_BUFFERS);

static TAutoConsoleVariable<int32> CVarSyncInterval(
    "D3D12RHI.SwapChain.SyncInterval",
    "Override sync interval for Present (-1 = use VSync setting, 0-4 = explicit interval)",
    -1);

static TAutoConsoleVariable<int32> CVarMaxFrameLatency(
    "D3D12RHI.SwapChain.MaxFrameLatency",
    "Maximum frame latency for the swap chain (-1 = use backbuffer count)",
    -1);

static TAutoConsoleVariable<int32> CVarD3D12DefaultBackBufferFormat(
    "D3D12RHI.DefaultBackBufferFormat",
    "Default back-buffer format used when ColorFormat is Unknown. "
    "0=B8G8R8A8_Unorm, "
    "1=R8G8B8A8_Unorm (default), "
    "2=R10G10B10A2_Unorm, "
    "3=R16G16B16A16_Float.",
    1); // D3D12 default: R8G8B8A8_Unorm

static TAutoConsoleVariable<int32> CVarD3D12DefaultBackBufferColorSpace(
    "D3D12RHI.DefaultBackBufferColorSpace",
    "Default back-buffer color space used when ColorSpace is Unknown. "
    "0=RGB_Full_G22_None_P709 / sRGB (default), "
    "1=RGB_Full_G10_None_P709 / scRGB, "
    "2=RGB_Full_G2084_None_P2020 / HDR10, "
    "3=RGB_Full_G22_None_P2020.",
    0);

EFormat FD3D12SwapChainRHI::GetDefaultBackBufferFormat()
{
    static constexpr EFormat FormatTable[] =
    {
        EFormat::B8G8R8A8_Unorm,      // 0
        EFormat::R8G8B8A8_Unorm,      // 1 (D3D12 default)
        EFormat::R10G10B10A2_Unorm,   // 2 (HDR10 candidate)
        EFormat::R16G16B16A16_Float,  // 3 (scRGB candidate)
    };

    int32 Index = CVarD3D12DefaultBackBufferFormat.GetValue();
    if (Index < 0 || Index >= static_cast<int32>(ARRAY_COUNT(FormatTable)))
    {
        D3D12_WARNING("D3D12RHI.DefaultBackBufferFormat=%d is out of range [0..%d]; clamping to 0.", Index, static_cast<int32>(ARRAY_COUNT(FormatTable)) - 1);
        Index = 0;
    }

    return FormatTable[Index];
}

static EColorSpace GetD3D12DefaultBackBufferColorSpace()
{
    static constexpr EColorSpace ColorSpaceTable[] =
    {
        EColorSpace::RGB_Full_G22_None_P709,    // 0 (default)
        EColorSpace::RGB_Full_G10_None_P709,    // 1 (scRGB)
        EColorSpace::RGB_Full_G2084_None_P2020, // 2 (HDR10)
        EColorSpace::RGB_Full_G22_None_P2020,   // 3
    };

    int32 Index = CVarD3D12DefaultBackBufferColorSpace.GetValue();
    if (Index < 0 || Index >= static_cast<int32>(ARRAY_COUNT(ColorSpaceTable)))
    {
        D3D12_WARNING("D3D12RHI.DefaultBackBufferColorSpace=%d is out of range [0..%d]; clamping to 0.", Index, static_cast<int32>(ARRAY_COUNT(ColorSpaceTable)) - 1);
        Index = 0;
    }

    return ColorSpaceTable[Index];
}

FD3D12SwapChainRHI::FD3D12SwapChainRHI(FD3D12Device* InDevice, FD3D12CommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc)
    : FD3D12DeviceChild(InDevice)
    , FRHISwapChain(InSwapChainDesc)
    , SwapChain(nullptr)
#if D3D12_ENABLE_COMPOSITION
    , Composition(nullptr)
#endif
    , CommandContext(InCommandContext)
    , BackBuffer(nullptr)
    , BackBuffers()
    , Hwnd(reinterpret_cast<HWND>(InSwapChainDesc.WindowHandle))
    , SwapChainWaitableObject(0)
    , CurrentColorSpace(EColorSpace::RGB_Full_G22_None_P709)
    , Flags(0)
    , NumBackBuffers(0)
    , ActiveFrameLatency(0)
    , BackBufferIndex(0)
{
}

FD3D12SwapChainRHI::~FD3D12SwapChainRHI()
{
#if D3D12_ENABLE_COMPOSITION
    const bool bHasFullscreenState = SwapChain.IsValid() && !Composition;
#else
    const bool bHasFullscreenState = SwapChain.IsValid();
#endif

    BOOL FullscreenState;
    if (bHasFullscreenState)
    {
        HRESULT Result = SwapChain->GetFullscreenState(&FullscreenState, nullptr);
        if (SUCCEEDED(Result))
        {
            if (FullscreenState)
            {
                SwapChain->SetFullscreenState(FALSE, nullptr);
            }
        }
    }

#if D3D12_ENABLE_COMPOSITION
    Composition.Reset();
#endif

    if (SwapChainWaitableObject)
    {
        CloseHandle(SwapChainWaitableObject);
    }

    ReleaseBackBufferResources();
    BackBuffers.Clear();
}

void FD3D12SwapChainRHI::ReleaseBackBufferResources()
{
    if (BackBuffer)
    {
        BackBuffer->SetResource(nullptr);

        if (FD3D12RenderTargetViewRHI* View = BackBuffer->RenderTargetView.Get())
        {
            View->ReleaseDescriptor();
        }

        if (FD3D12UnorderedAccessViewRHI* View = BackBuffer->UnorderedAccessView.Get())
        {
            View->ReleaseDescriptor();
        }

        if (FD3D12ShaderResourceViewRHI* View = BackBuffer->ShaderResourceView.Get())
        {
            View->ReleaseDescriptor();
        }
    }

    FD3D12OfflineDescriptorHeap& ResourceHeap     = GetDevice()->GetResourceOfflineDescriptorHeap();
    FD3D12OfflineDescriptorHeap& RenderTargetHeap = GetDevice()->GetRenderTargetOfflineDescriptorHeap();

    for (FBackBufferData& BackBufferData : BackBuffers)
    {
        if (BackBufferData.RenderTargetDescriptor)
        {
            RenderTargetHeap.Free(BackBufferData.RenderTargetDescriptor);
            BackBufferData.RenderTargetDescriptor = {};
        }

        if (BackBufferData.UnorderedAccessDescriptor)
        {
            ResourceHeap.Free(BackBufferData.UnorderedAccessDescriptor);
            BackBufferData.UnorderedAccessDescriptor = {};
        }

        if (BackBufferData.ShaderResourceDescriptor)
        {
            ResourceHeap.Free(BackBufferData.ShaderResourceDescriptor);
            BackBufferData.ShaderResourceDescriptor = {};
        }

        BackBufferData.Resource.Reset();
    }
}

bool FD3D12SwapChainRHI::Initialize(FD3D12CommandContext* InCommandContext)
{
    // Ensure that the CommandContext used is the same that we created the viewport with.
    // The limitation is really just that we use the same ID3D12CommandQueue that we used for 
    // creation since the presentation is queued up on the command-queue.
    
    CHECK(CommandContext == InCommandContext);

    // Save the flags
    Flags = GetDevice()->GetAdapter()->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    if (Desc.bFramePacing)
    {
        Flags = Flags | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    }

    RECT ClientRect;
    GetClientRect(Hwnd, &ClientRect);
    
    if (Desc.Width == 0)
    {
        Desc.Width = uint16(ClientRect.right - ClientRect.left);
    }
    
    if (Desc.Height == 0)
    {
        Desc.Height = uint16(ClientRect.bottom - ClientRect.top);
    }
    
    if (!Desc.Width)
    {
        D3D12_ERROR_CRITICAL("SwapChain width of zero is not supported");
        return false;
    }
    
    if (!Desc.Height)
    {
        D3D12_ERROR_CRITICAL("SwapChain height of zero is not supported");
        return false;
    }

    if (!Desc.IsRenderTarget() && !Desc.IsUnorderedAccess())
    {
        D3D12_ERROR("[FD3D12SwapChainRHI]: Desc.Usage must include at least one of ESwapChainUsageFlags::RenderTarget or ESwapChainUsageFlags::UnorderedAccess");
        return false;
    }

    EFormat ResolvedFormat = EFormat::Unknown;
    if (Desc.ColorFormat == EFormat::Unknown)
    {
        ResolvedFormat = GetDefaultBackBufferFormat();
    }
    else
    {
        if (!GetDevice()->SupportsSwapChainFormat(D3D12RHI::ConvertFormat(Desc.ColorFormat), Desc.Usage))
        {
            D3D12_ERROR("[FD3D12SwapChainRHI]: Requested back-buffer format %s is not supported on this device.", ToString(Desc.ColorFormat));
            return false;
        }

        ResolvedFormat = Desc.ColorFormat;
    }

    Desc.ColorFormat = ResolvedFormat;

    const EColorSpace ResolvedColorSpace = (Desc.ColorSpace == EColorSpace::Unknown)
        ? GetD3D12DefaultBackBufferColorSpace()
        : Desc.ColorSpace;

    const uint32 NumSwapChainBuffers = Math::Clamp<int32>(CVarSwapChainBackBufferCount.GetValue(), 2, 8);

#if D3D12_ENABLE_COMPOSITION
    const bool bUseComposition = Desc.IsTransparent() && GD3D12SupportsComposition;
#else
    const bool bUseComposition = false;
#endif

    if (Desc.IsTransparent() && !bUseComposition)
    {
        D3D12_WARNING("[FD3D12SwapChainRHI]: DirectComposition unavailable, falling back to an opaque swap chain");
        SetEnumFlag(Desc.Flags, ESwapChainFlags::Transparent, false);
    }

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.Width              = Desc.Width;
    SwapChainDesc.Height             = Desc.Height;
    SwapChainDesc.Format             = D3D12RHI::ConvertFormat(ResolvedFormat);
    SwapChainDesc.BufferUsage        = D3D12RHI::ConvertSwapChainUsage(Desc.Usage);
    SwapChainDesc.BufferCount        = NumSwapChainBuffers;
    SwapChainDesc.SampleDesc.Count   = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.AlphaMode          = bUseComposition ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_IGNORE;
    SwapChainDesc.Flags              = Flags;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullscreenDesc = {};
    FullscreenDesc.RefreshRate.Numerator   = 0;
    FullscreenDesc.RefreshRate.Denominator = 1;
    FullscreenDesc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
    FullscreenDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    FullscreenDesc.Windowed                = true;

    IDXGIFactory2* Factory = GetDevice()->GetAdapter()->GetDXGIFactory();
    CHECK(Factory != nullptr);

    ID3D12CommandQueue* CommandQueue = GetDevice()->GetD3D12CommandQueue(InCommandContext->GetQueueType());
    CHECK(CommandQueue != nullptr);

    TComPtr<IDXGISwapChain1> DXGISwapChain1;
    HRESULT Result = bUseComposition
        ? Factory->CreateSwapChainForComposition(CommandQueue, &SwapChainDesc, nullptr, &DXGISwapChain1)
        : Factory->CreateSwapChainForHwnd(CommandQueue, Hwnd, &SwapChainDesc, &FullscreenDesc, nullptr, &DXGISwapChain1);

    if (SUCCEEDED(Result))
    {
        Result = DXGISwapChain1.GetAs<IDXGISwapChain3>(&SwapChain);
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[FD3D12SwapChainRHI]: FAILED to retrieve IDXGISwapChain3");
            return false;
        }

    #if D3D12_ENABLE_COMPOSITION
        if (bUseComposition)
        {
            FD3D12CompositionRef NewComposition = new FD3D12Composition(GetDevice());
            if (!NewComposition->Initialize(Hwnd, DXGISwapChain1.Get()))
            {
                D3D12_ERROR_CRITICAL("[FD3D12SwapChainRHI]: FAILED to bind the SwapChain to a composition visual");
                return false;
            }

            Composition = NewComposition;
        }
    #endif

        // Optional: only needed for SetHDRMetaData. Absence downgrades HDR output, not correctness.
        if (FAILED(DXGISwapChain1.GetAs<IDXGISwapChain4>(&SwapChain4)))
        {
            D3D12_WARNING("[FD3D12SwapChainRHI]: IDXGISwapChain4 unavailable; HDR metadata will not be submitted");
        }

        NumBackBuffers = NumSwapChainBuffers;

        if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
        {
            SwapChainWaitableObject = SwapChain->GetFrameLatencyWaitableObject();

            const int32 FrameLatencyCVar = CVarMaxFrameLatency.GetValue();
            ActiveFrameLatency = (FrameLatencyCVar >= 0) ? static_cast<uint32>(FrameLatencyCVar) : NumSwapChainBuffers;
            SwapChain->SetMaximumFrameLatency(ActiveFrameLatency);
        }
    }
    else
    {
        D3D12_ERROR_CRITICAL("[FD3D12SwapChainRHI]: FAILED to create SwapChain");
        return false;
    }

    Factory->MakeWindowAssociation(Hwnd, DXGI_MWA_NO_ALT_ENTER);

    // Apply color space
    {
        const DXGI_COLOR_SPACE_TYPE RequestedDXGI = D3D12RHI::ConvertColorSpace(ResolvedColorSpace);
        
        UINT SupportFlags = 0;
        SwapChain->CheckColorSpaceSupport(RequestedDXGI, &SupportFlags);
        
        if ((SupportFlags & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == 0)
        {
            D3D12_ERROR("[FD3D12SwapChainRHI]: Requested (%s, %s) combination not supported on this output; aborting.", 
                ToString(ResolvedFormat), ToString(ResolvedColorSpace));
            return false;
        }

        const HRESULT SetColorSpaceResult = SwapChain->SetColorSpace1(RequestedDXGI);
        if (FAILED(SetColorSpaceResult))
        {
            D3D12_ERROR("[FD3D12SwapChainRHI]: SetColorSpace1 failed (hr=0x%x).", static_cast<uint32>(SetColorSpaceResult));
            return false;
        }

        CurrentColorSpace = ResolvedColorSpace;
        Desc.ColorSpace   = ResolvedColorSpace;
    }

    if (CurrentColorSpace == EColorSpace::RGB_Full_G2084_None_P2020 && !Desc.HDRMetadata.bIsValid)
    {
        FRHIHDRMetadata DefaultMetadata = RHI::GetDefaultHDRMetadata();

        FRHIDisplayHDRInfo DisplayInfo;
        if (RHI::ShouldUseDisplayLuminance() && QueryDisplayHDRInfo(DisplayInfo))
        {
            DefaultMetadata.MinMasteringLuminance     = DisplayInfo.MinLuminance;
            DefaultMetadata.MaxMasteringLuminance     = DisplayInfo.MaxLuminance;
            DefaultMetadata.MaxFrameAverageLightLevel = DisplayInfo.MaxFullFrameLuminance;
        }

        SetHDRMetadata(DefaultMetadata);
    }

    if (!RetrieveBackBuffers())
    {
        return false;
    }

    D3D12_INFO("[FD3D12SwapChainRHI]: Created SwapChain (%s, %s)", ToString(ResolvedFormat), ToString(ResolvedColorSpace));
    return true;
}

bool FD3D12SwapChainRHI::Resize(FD3D12CommandContext* InCommandContext, uint32 InWidth, uint32 InHeight, EFormat NewFormat, EColorSpace NewColorSpace)
{
    const uint32      ResolvedWidth          = (InWidth  > 0u) ? InWidth  : Desc.Width;
    const uint32      ResolvedHeight         = (InHeight > 0u) ? InHeight : Desc.Height;
    const EFormat     EffectiveFormat        = (NewFormat     == EFormat::Unknown)     ? Desc.ColorFormat    : NewFormat;
    const EColorSpace EffectiveColorSpace    = (NewColorSpace == EColorSpace::Unknown) ? CurrentColorSpace   : NewColorSpace;
    const uint32      DesiredBackBufferCount = Math::Clamp<int32>(CVarSwapChainBackBufferCount.GetValue(), 2, 8);

    const bool bSizeChanged        = (ResolvedWidth != Desc.Width || ResolvedHeight != Desc.Height) && ResolvedWidth > 0u && ResolvedHeight > 0u;
    const bool bBufferCountChanged = DesiredBackBufferCount != NumBackBuffers;
    const bool bFormatChanged      = (NewFormat     != EFormat::Unknown)     && (EffectiveFormat     != Desc.ColorFormat);
    const bool bColorSpaceChanged  = (NewColorSpace != EColorSpace::Unknown) && (EffectiveColorSpace != CurrentColorSpace);

    if (bFormatChanged || bColorSpaceChanged)
    {
        if (!IsFormatSupported(EffectiveFormat, EffectiveColorSpace))
        {
            D3D12_WARNING("[FD3D12SwapChainRHI]: Resize: (%s, %s) not supported on this swap-chain; leaving unchanged.", 
                ToString(EffectiveFormat), ToString(EffectiveColorSpace));
            return false;
        }
    }

    const bool bNeedsResizeBuffers = bSizeChanged || bBufferCountChanged || bFormatChanged;
    if (bNeedsResizeBuffers)
    {
        if (InCommandContext->IsRecording())
        {
            InCommandContext->SplitCommandListAndResetState(false, true);
        }
        else
        {
            InCommandContext->ClearState();
        }

        ReleaseBackBufferResources();

        const DXGI_FORMAT ResizeDXGIFormat = bFormatChanged ? D3D12RHI::ConvertFormat(EffectiveFormat) : DXGI_FORMAT_UNKNOWN;
        HRESULT Result = SwapChain->ResizeBuffers(DesiredBackBufferCount, ResolvedWidth, ResolvedHeight, ResizeDXGIFormat, Flags);
        if (SUCCEEDED(Result))
        {
            NumBackBuffers = DesiredBackBufferCount;

            if (bSizeChanged)
            {
                Desc.Width  = uint16(ResolvedWidth);
                Desc.Height = uint16(ResolvedHeight);
            }

            if (bFormatChanged)
            {
                Desc.ColorFormat = EffectiveFormat;
            }
        }
        else
        {
            D3D12_WARNING("[FD3D12SwapChainRHI]: Resize FAILED");
            return false;
        }

        if (!RetrieveBackBuffers())
        {
            return false;
        }

        D3D12_INFO("[FD3D12SwapChainRHI]: Resized Width=%u Height=%u Format=%s Colorspace=%s BackBuffers=%u",
            Desc.Width, Desc.Height, ToString(D3D12RHI::ConvertFormat(Desc.ColorFormat)), ToString(D3D12RHI::ConvertColorSpace(EffectiveColorSpace)), NumBackBuffers);
    }

    if (bColorSpaceChanged)
    {
        const DXGI_COLOR_SPACE_TYPE NewDXGI = D3D12RHI::ConvertColorSpace(EffectiveColorSpace);
        if (FAILED(SwapChain->SetColorSpace1(NewDXGI)))
        {
            D3D12_WARNING("[FD3D12SwapChainRHI]: SetColorSpace1(%s) failed", ToString(EffectiveColorSpace));
            return false;
        }

        Desc.ColorSpace   = EffectiveColorSpace;
        CurrentColorSpace = EffectiveColorSpace;

        D3D12_INFO("[FD3D12SwapChainRHI]: Color space changed to %s", ToString(EffectiveColorSpace));
    }

    // ResizeBuffers can drop the metadata, and a color-space change can invalidate it.
    if (bNeedsResizeBuffers || bColorSpaceChanged)
    {
        ApplyHDRMetadata();
    }

    // Apply frame latency changes if needed.
    if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    {
        const int32  FrameLatencyCVar    = CVarMaxFrameLatency.GetValue();
        const uint32 DesiredFrameLatency = (FrameLatencyCVar >= 0) ? static_cast<uint32>(FrameLatencyCVar) : NumBackBuffers;

        if (DesiredFrameLatency != ActiveFrameLatency)
        {
            SwapChain->SetMaximumFrameLatency(DesiredFrameLatency);
            ActiveFrameLatency = DesiredFrameLatency;

            D3D12_INFO("[FD3D12SwapChainRHI]: Changed max frame latency to %u", ActiveFrameLatency);
        }
    }

    return true;
}

void* FD3D12SwapChainRHI::GetRHINativeHandle() const
{
    return SwapChain.Get();
}

void* FD3D12SwapChainRHI::GetRHINativeResourceFromIndex(uint32 Index) const
{
    FD3D12Resource* BackBufferResource = GetResourceAtIndex(Index);
    return BackBufferResource 
        ? reinterpret_cast<void*>(BackBufferResource->GetD3D12Resource()) 
        : nullptr;
}

void* FD3D12SwapChainRHI::GetRHINativeRenderTargetViewFromIndex(uint32 Index) const
{
    const int32 ResourceIndex = static_cast<int32>(Index);
    return BackBuffers.IsValidIndex(ResourceIndex)
        ? reinterpret_cast<void*>(static_cast<UPTR_INT>(BackBuffers[ResourceIndex].RenderTargetDescriptor.Handle.ptr))
        : nullptr;
}

void* FD3D12SwapChainRHI::GetRHINativeUnorderedAccessViewFromIndex(uint32 Index) const
{
    const int32 ResourceIndex = static_cast<int32>(Index);
    return BackBuffers.IsValidIndex(ResourceIndex)
        ? reinterpret_cast<void*>(static_cast<UPTR_INT>(BackBuffers[ResourceIndex].UnorderedAccessDescriptor.Handle.ptr))
        : nullptr;
}

void* FD3D12SwapChainRHI::GetRHINativeShaderResourceViewFromIndex(uint32 Index) const
{
    const int32 ResourceIndex = static_cast<int32>(Index);
    return BackBuffers.IsValidIndex(ResourceIndex)
        ? reinterpret_cast<void*>(static_cast<UPTR_INT>(BackBuffers[ResourceIndex].ShaderResourceDescriptor.Handle.ptr))
        : nullptr;
}

FRHITexture* FD3D12SwapChainRHI::GetBackBuffer() const
{
    return BackBuffer.Get();
}

FRHIRenderTargetView* FD3D12SwapChainRHI::GetRenderTargetView() const
{
    return BackBuffer ? BackBuffer->GetRenderTargetView() : nullptr;
}

FRHIUnorderedAccessView* FD3D12SwapChainRHI::GetUnorderedAccessView() const
{
    return BackBuffer ? BackBuffer->GetUnorderedAccessView() : nullptr;
}

FRHIShaderResourceView* FD3D12SwapChainRHI::GetShaderResourceView() const
{
    return BackBuffer ? BackBuffer->GetShaderResourceView() : nullptr;
}

uint32 FD3D12SwapChainRHI::GetNumResources() const
{
    return GetBackBufferCount();
}

bool FD3D12SwapChainRHI::IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const
{
    if (!SwapChain)
    {
        return false;
    }

    if (!GetDevice()->SupportsSwapChainFormat(D3D12RHI::ConvertFormat(Format), Desc.Usage))
    {
        return false;
    }

    UINT SupportFlags = 0;
    if (FAILED(SwapChain->CheckColorSpaceSupport(D3D12RHI::ConvertColorSpace(ColorSpace), &SupportFlags)))
    {
        return false;
    }

    return (SupportFlags & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) != 0;
}

bool FD3D12SwapChainRHI::SetHDRMetadata(const FRHIHDRMetadata& Metadata)
{
    Desc.HDRMetadata = Metadata;
    return ApplyHDRMetadata();
}

bool FD3D12SwapChainRHI::ApplyHDRMetadata()
{
    if (!SwapChain4)
    {
        return false;
    }

    const bool bIsHDRColorSpace = (CurrentColorSpace == EColorSpace::RGB_Full_G2084_None_P2020);
    if (!Desc.HDRMetadata.bIsValid || !bIsHDRColorSpace)
    {
        return SUCCEEDED(SwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr));
    }

    const FRHIHDRMetadata& Source = Desc.HDRMetadata;

    DXGI_HDR_METADATA_HDR10 HDR10 = {};
    HDR10.RedPrimary[0]             = FRHIHDRMetadata::EncodeChromaticity(Source.RedPrimary.X);
    HDR10.RedPrimary[1]             = FRHIHDRMetadata::EncodeChromaticity(Source.RedPrimary.Y);
    HDR10.GreenPrimary[0]           = FRHIHDRMetadata::EncodeChromaticity(Source.GreenPrimary.X);
    HDR10.GreenPrimary[1]           = FRHIHDRMetadata::EncodeChromaticity(Source.GreenPrimary.Y);
    HDR10.BluePrimary[0]            = FRHIHDRMetadata::EncodeChromaticity(Source.BluePrimary.X);
    HDR10.BluePrimary[1]            = FRHIHDRMetadata::EncodeChromaticity(Source.BluePrimary.Y);
    HDR10.WhitePoint[0]             = FRHIHDRMetadata::EncodeChromaticity(Source.WhitePoint.X);
    HDR10.WhitePoint[1]             = FRHIHDRMetadata::EncodeChromaticity(Source.WhitePoint.Y);
    HDR10.MaxMasteringLuminance     = static_cast<UINT>(Math::Max(Math::RoundToInt(Source.MaxMasteringLuminance), 0));
    HDR10.MinMasteringLuminance     = FRHIHDRMetadata::EncodeMinLuminance(Source.MinMasteringLuminance);
    HDR10.MaxContentLightLevel      = FRHIHDRMetadata::EncodeNits16(Source.MaxContentLightLevel);
    HDR10.MaxFrameAverageLightLevel = FRHIHDRMetadata::EncodeNits16(Source.MaxFrameAverageLightLevel);

    const HRESULT Result = SwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(HDR10), &HDR10);
    if (FAILED(Result))
    {
        D3D12_WARNING("[FD3D12SwapChainRHI]: SetHDRMetaData failed (hr=0x%x)", static_cast<uint32>(Result));
        return false;
    }

    D3D12_INFO("[FD3D12SwapChainRHI]: HDR10 metadata set (max=%.1f nits, min=%.4f nits, MaxCLL=%.1f, MaxFALL=%.1f)",
        Source.MaxMasteringLuminance, Source.MinMasteringLuminance, Source.MaxContentLightLevel, Source.MaxFrameAverageLightLevel);
    return true;
}

bool FD3D12SwapChainRHI::QueryDisplayHDRInfo(FRHIDisplayHDRInfo& OutInfo) const
{
#if DXGI_1_6
    if (!SwapChain)
    {
        return false;
    }

    TComPtr<IDXGIOutput> Output;
    if (FAILED(SwapChain->GetContainingOutput(&Output)) || !Output)
    {
        return false;
    }

    TComPtr<IDXGIOutput6> Output6;
    if (FAILED(Output.GetAs<IDXGIOutput6>(&Output6)))
    {
        return false;
    }

    DXGI_OUTPUT_DESC1 OutputDesc = {};
    if (FAILED(Output6->GetDesc1(&OutputDesc)))
    {
        return false;
    }

    OutInfo.RedPrimary            = { OutputDesc.RedPrimary[0],   OutputDesc.RedPrimary[1]   };
    OutInfo.GreenPrimary          = { OutputDesc.GreenPrimary[0], OutputDesc.GreenPrimary[1] };
    OutInfo.BluePrimary           = { OutputDesc.BluePrimary[0],  OutputDesc.BluePrimary[1]  };
    OutInfo.WhitePoint            = { OutputDesc.WhitePoint[0],   OutputDesc.WhitePoint[1]   };
    OutInfo.ColorSpace            = D3D12RHI::ConvertColorSpace(OutputDesc.ColorSpace);
    OutInfo.MinLuminance          = OutputDesc.MinLuminance;
    OutInfo.MaxLuminance          = OutputDesc.MaxLuminance;
    OutInfo.MaxFullFrameLuminance = OutputDesc.MaxFullFrameLuminance;
    OutInfo.BitsPerColor          = OutputDesc.BitsPerColor;
    return true;
#else
    (void)OutInfo;
    return false;
#endif
}

void FD3D12SwapChainRHI::AcquireNextBackBuffer()
{
    if (!SwapChain)
    {
        return;
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    SwapResources(BackBufferIndex);
}

void FD3D12SwapChainRHI::SwapResources(uint32 Index)
{
    const int32 ResourceIndex = static_cast<int32>(Index);
    if (!BackBuffers.IsValidIndex(ResourceIndex) || !BackBuffer)
    {
        return;
    }

    const FBackBufferData& BackBufferData = BackBuffers[ResourceIndex];
    BackBuffer->SetResource(BackBufferData.Resource.Get());

    if (FD3D12RenderTargetViewRHI* View = BackBuffer->RenderTargetView.Get())
    {
        View->UpdateDescriptor(BackBufferData.Resource.Get(), BackBufferData.RenderTargetDescriptor);
    }

    if (FD3D12UnorderedAccessViewRHI* View = BackBuffer->UnorderedAccessView.Get())
    {
        View->UpdateDescriptor(BackBufferData.Resource.Get(), BackBufferData.UnorderedAccessDescriptor);
    }

    if (FD3D12ShaderResourceViewRHI* View = BackBuffer->ShaderResourceView.Get())
    {
        View->UpdateDescriptor(BackBufferData.Resource.Get(), BackBufferData.ShaderResourceDescriptor);
    }
}

bool FD3D12SwapChainRHI::Present(bool bVerticalSync)
{
    TRACE_FUNCTION_SCOPE();

    const int32  SyncIntervalOverride = CVarSyncInterval.GetValue();
    const uint32 SyncInterval         = (SyncIntervalOverride >= 0) ? Math::Clamp<uint32>(SyncIntervalOverride, 0, 4) : (bVerticalSync ? 1 : 0);

    uint32 PresentFlags = 0;
    if (SyncInterval == 0 && Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
    {
        PresentFlags = DXGI_PRESENT_ALLOW_TEARING;
    }

    HRESULT Result = SwapChain->Present(SyncInterval, PresentFlags);
    D3D12Debug::CheckDeviceRemoved(GetDevice(), Result, "Present");

    if (SUCCEEDED(Result))
    {
        if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
        {
            const DWORD WaitResult = WaitForSingleObjectEx(SwapChainWaitableObject, INFINITE, TRUE);
            if (WaitResult != WAIT_OBJECT_0)
            {
                return false;
            }
        }

        ApplySettingsChanges();
        return true;
    }
    else
    {
        return false;
    }
}

void FD3D12SwapChainRHI::ApplySettingsChanges()
{
    const uint32 DesiredBackBufferCount = Math::Clamp<int32>(CVarSwapChainBackBufferCount.GetValue(), 2, 8);
    if (DesiredBackBufferCount != NumBackBuffers)
    {
        // Wait for all GPU work to complete before releasing backbuffer resources
        CommandContext->SplitCommandListAndResetState(false, true);

        ReleaseBackBufferResources();

        HRESULT Result = SwapChain->ResizeBuffers(DesiredBackBufferCount, Desc.Width, Desc.Height, DXGI_FORMAT_UNKNOWN, Flags);
        if (SUCCEEDED(Result))
        {
            NumBackBuffers = DesiredBackBufferCount;
            RetrieveBackBuffers();
            D3D12_INFO("[FD3D12SwapChainRHI]: Changed backbuffer count to %u", NumBackBuffers);
        }
        else
        {
            D3D12_WARNING("[FD3D12SwapChainRHI]: ResizeBuffers for backbuffer count change FAILED");
            RetrieveBackBuffers();
        }
    }

    if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    {
        const int32  FrameLatencyCVar    = CVarMaxFrameLatency.GetValue();
        const uint32 DesiredFrameLatency = (FrameLatencyCVar >= 0) ? static_cast<uint32>(FrameLatencyCVar) : NumBackBuffers;

        if (DesiredFrameLatency != ActiveFrameLatency)
        {
            SwapChain->SetMaximumFrameLatency(DesiredFrameLatency);
            ActiveFrameLatency = DesiredFrameLatency;
            D3D12_INFO("[FD3D12SwapChainRHI]: Changed max frame latency to %u", ActiveFrameLatency);
        }
    }
}

bool FD3D12SwapChainRHI::CreateBackBuffer()
{
    ETextureUsageFlags BackBufferUsageFlags = ETextureUsageFlags::Presentable;
    if (Desc.IsRenderTarget())
    {
        BackBufferUsageFlags |= ETextureUsageFlags::RenderTarget;
    }

    if (Desc.IsUnorderedAccess())
    {
        BackBufferUsageFlags |= ETextureUsageFlags::UnorderedAccessTexture;
    }

    if (Desc.IsShaderResource())
    {
        BackBufferUsageFlags |= ETextureUsageFlags::ShaderResourceTexture;
    }

    const FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat, Desc.Width, Desc.Height,
        1, 1, BackBufferUsageFlags, FClearValue(), ERHIResourceStateTrackingMode::Manual);

    BackBuffer = new FD3D12TextureRHI(GetDevice(), BackBufferDesc);
    if (!BackBuffer)
    {
        D3D12_ERROR("[FD3D12SwapChainRHI]: Failed to create the BackBuffer texture");
        return false;
    }

    return true;
}

bool FD3D12SwapChainRHI::CreateBackBufferDescriptors()
{
    FD3D12OfflineDescriptorHeap& ResourceHeap     = GetDevice()->GetResourceOfflineDescriptorHeap();
    FD3D12OfflineDescriptorHeap& RenderTargetHeap = GetDevice()->GetRenderTargetOfflineDescriptorHeap();

    for (FBackBufferData& BackBufferData : BackBuffers)
    {
        ID3D12Resource* D3DResource = BackBufferData.Resource->GetD3D12Resource();

        if (FD3D12RenderTargetViewRHI* View = BackBuffer->RenderTargetView.Get())
        {
            BackBufferData.RenderTargetDescriptor = RenderTargetHeap.Allocate();
            if (!BackBufferData.RenderTargetDescriptor)
            {
                D3D12_ERROR("[FD3D12SwapChainRHI]: Failed to allocate a back-buffer RTV descriptor");
                return false;
            }

            GetDevice()->GetD3D12Device()->CreateRenderTargetView(
                D3DResource,
                &View->GetD3D12Desc(),
                BackBufferData.RenderTargetDescriptor.Handle);
        }

        if (FD3D12UnorderedAccessViewRHI* View = BackBuffer->UnorderedAccessView.Get())
        {
            BackBufferData.UnorderedAccessDescriptor = ResourceHeap.Allocate();
            if (!BackBufferData.UnorderedAccessDescriptor)
            {
                D3D12_ERROR("[FD3D12SwapChainRHI]: Failed to allocate a back-buffer UAV descriptor");
                return false;
            }

            GetDevice()->GetD3D12Device()->CreateUnorderedAccessView(
                D3DResource,
                nullptr,
                &View->GetD3D12Desc(),
                BackBufferData.UnorderedAccessDescriptor.Handle);
        }

        if (FD3D12ShaderResourceViewRHI* View = BackBuffer->ShaderResourceView.Get())
        {
            BackBufferData.ShaderResourceDescriptor = ResourceHeap.Allocate();
            if (!BackBufferData.ShaderResourceDescriptor)
            {
                D3D12_ERROR("[FD3D12SwapChainRHI]: Failed to allocate a back-buffer SRV descriptor");
                return false;
            }

            GetDevice()->GetD3D12Device()->CreateShaderResourceView(
                D3DResource,
                &View->GetD3D12Desc(),
                BackBufferData.ShaderResourceDescriptor.Handle);
        }
    }

    return true;
}

bool FD3D12SwapChainRHI::RetrieveBackBuffers()
{
    BackBuffers.Resize(NumBackBuffers);

    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        TComPtr<ID3D12Resource> D3DBackBufferResource;

        HRESULT Result = SwapChain->GetBuffer(Index, IID_PPV_ARGS(&D3DBackBufferResource));
        if (FAILED(Result))
        {
            D3D12_INFO("[FD3D12SwapChainRHI]: GetBuffer(%u) Failed", Index);
            return false;
        }

        FD3D12ResourceRef BackBufferResource = new FD3D12Resource(GetDevice(), D3DBackBufferResource.ReleaseOwnership(),
            D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_PRESENT);

        BackBufferResource->SetResourceStateMode(ED3D12ResourceStateMode::ManualState);
        BackBufferResource->DisableDeferredRelease();
        BackBufferResource->SetDebugName(String::Printf("BackBuffer[%u]", Index));

        BackBuffers[Index].Resource = BackBufferResource;
    }

    if (BackBuffer)
    {
        BackBuffer->SetSwapChainResource(GetResourceAtIndex(0), Desc.ColorFormat, Desc.Width, Desc.Height);
    }
    else if (!CreateBackBuffer())
    {
        return false;
    }

    if (!BackBuffer->InitializeSwapChainTexture())
    {
        return false;
    }

    if (!CreateBackBufferDescriptors())
    {
        return false;
    }

    AcquireNextBackBuffer();
    return BackBuffer->GetResource() != nullptr;
}
