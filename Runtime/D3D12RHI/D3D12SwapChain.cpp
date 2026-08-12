#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12SwapChain.h"
#include "D3D12RHI/D3D12BackBufferProxies.h"
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

EFormat GetD3D12DefaultBackBufferFormat()
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
    , CommandContext(InCommandContext)
    , BackBufferProxy(nullptr)
    , BackBufferProxyRenderTargetView(nullptr)
    , BackBufferProxyUnorderedAccessView(nullptr)
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
    BOOL FullscreenState;
    if (SwapChain)
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

    if (SwapChainWaitableObject)
    {
        CloseHandle(SwapChainWaitableObject);
    }

    if (BackBufferProxy)
    {
        BackBufferProxy->SetSwapChain(nullptr);
    }

    if (BackBufferProxyRenderTargetView)
    {
        BackBufferProxyRenderTargetView->SetSwapChain(nullptr);
    }

    if (BackBufferProxyUnorderedAccessView)
    {
        BackBufferProxyUnorderedAccessView->SetSwapChain(nullptr);
    }

    for (FBackBufferData& Data : BackBuffers)
    {
        if (Data.Texture)
        {
            Data.Texture->SetResource(nullptr);
        }

        if (Data.RenderTargetView)
        {
            Data.RenderTargetView->ReleaseViewResource();
        }

        if (Data.UnorderedAccessView)
        {
            Data.UnorderedAccessView->ReleaseViewResource();
        }
    }

    BackBuffers.Clear();
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
        ResolvedFormat = GetD3D12DefaultBackBufferFormat();
    }
    else
    {
        if (!GetDevice()->SupportsSwapChainFormat(ConvertFormat(Desc.ColorFormat), Desc.Usage))
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

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.Width              = Desc.Width;
    SwapChainDesc.Height             = Desc.Height;
    SwapChainDesc.Format             = ConvertFormat(ResolvedFormat);
    SwapChainDesc.BufferUsage        = ConvertSwapChainUsage(Desc.Usage);
    SwapChainDesc.BufferCount        = NumSwapChainBuffers;
    SwapChainDesc.SampleDesc.Count   = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;
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
    HRESULT Result = Factory->CreateSwapChainForHwnd(CommandQueue, Hwnd, &SwapChainDesc, &FullscreenDesc, nullptr, &DXGISwapChain1);
    if (SUCCEEDED(Result))
    {
        Result = DXGISwapChain1.GetAs<IDXGISwapChain3>(&SwapChain);
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[FD3D12SwapChainRHI]: FAILED to retrieve IDXGISwapChain3");
            return false;
        }

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
        const DXGI_COLOR_SPACE_TYPE RequestedDXGI = ConvertColorSpace(ResolvedColorSpace);
        
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

        for (FBackBufferData& Data : BackBuffers)
        {
            if (Data.Texture)
            {
                Data.Texture->SetResource(nullptr);
            }

            if (Data.RenderTargetView)
            {
                Data.RenderTargetView->ReleaseViewResource();
            }

            if (Data.UnorderedAccessView)
            {
                Data.UnorderedAccessView->ReleaseViewResource();
            }
        }

        const DXGI_FORMAT ResizeDXGIFormat = bFormatChanged ? ConvertFormat(EffectiveFormat) : DXGI_FORMAT_UNKNOWN;
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
            Desc.Width, Desc.Height, ToString(ConvertFormat(Desc.ColorFormat)), ToString(ConvertColorSpace(EffectiveColorSpace)), NumBackBuffers);
    }

    if (bColorSpaceChanged)
    {
        const DXGI_COLOR_SPACE_TYPE NewDXGI = ConvertColorSpace(EffectiveColorSpace);
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

void* FD3D12SwapChainRHI::GetRHINativeBackBufferResourceFromIndex(uint32 Index) const
{
    FD3D12TextureRHI* Texture = GetBackBufferAtIndex(Index);
    return Texture ? Texture->GetRHINativeResource() : nullptr;
}

void* FD3D12SwapChainRHI::GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index) const
{
    FD3D12RenderTargetViewRHI* View = GetBackBufferRenderTargetViewAtIndex(Index);
    return View ? View->GetRHINativeHandle() : nullptr;
}

void* FD3D12SwapChainRHI::GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 Index) const
{
    FD3D12UnorderedAccessViewRHI* View = GetBackBufferUnorderedAccessViewAtIndex(Index);
    return View ? View->GetRHINativeHandle() : nullptr;
}

FRHITexture* FD3D12SwapChainRHI::GetBackBuffer() const
{
    return BackBufferProxy.Get();
}

FRHITexture* FD3D12SwapChainRHI::GetBackBufferResourceFromIndex(uint32 Index) const
{
    return GetBackBufferAtIndex(Index);
}

uint32 FD3D12SwapChainRHI::GetNumBackBufferResources() const
{
    return GetBackBufferCount();
}

FRHIRenderTargetView* FD3D12SwapChainRHI::GetBackBufferRenderTargetView() const
{
    return BackBufferProxyRenderTargetView.Get();
}

FRHIUnorderedAccessView* FD3D12SwapChainRHI::GetBackBufferUnorderedAccessView() const
{
    return Desc.IsUnorderedAccess() ? BackBufferProxyUnorderedAccessView.Get() : nullptr;
}

bool FD3D12SwapChainRHI::IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const
{
    if (!SwapChain)
    {
        return false;
    }

    if (!GetDevice()->SupportsSwapChainFormat(ConvertFormat(Format), Desc.Usage))
    {
        return false;
    }

    UINT SupportFlags = 0;
    if (FAILED(SwapChain->CheckColorSpaceSupport(ConvertColorSpace(ColorSpace), &SupportFlags)))
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
    OutInfo.ColorSpace            = ConvertColorSpace(OutputDesc.ColorSpace);
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
    D3D12RHICheckDeviceRemoved(GetDevice(), Result, "Present");

    if (SUCCEEDED(Result))
    {
        BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

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

        for (FBackBufferData& Data : BackBuffers)
        {
            if (Data.Texture)
            {
                Data.Texture->SetResource(nullptr);
            }

            if (Data.RenderTargetView)
            {
                Data.RenderTargetView->ReleaseViewResource();
            }

            if (Data.UnorderedAccessView)
            {
                Data.UnorderedAccessView->ReleaseViewResource();
            }
        }

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

bool FD3D12SwapChainRHI::RetrieveBackBuffers()
{
    ETextureUsageFlags UsageFlags = ETextureUsageFlags::Presentable;
    if (Desc.IsRenderTarget())
    {
        UsageFlags |= ETextureUsageFlags::RenderTarget;
    }
    if (Desc.IsUnorderedAccess())
    {
        UsageFlags |= ETextureUsageFlags::UnorderedAccessTexture;
    }

    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat, Desc.Width, Desc.Height, 1, 1, UsageFlags);
    
    BackBuffers.Resize(NumBackBuffers);
    for (int32 Index = 0; Index < BackBuffers.Size(); ++Index)
    {
        BackBuffers[Index].Texture = new FD3D12TextureRHI(GetDevice(), BackBufferDesc);
    }

    if (BackBufferProxy)
    {
        BackBufferProxy->Resize(Desc.Width, Desc.Height);
    }
    else
    {
        BackBufferProxy = new FD3D12BackBufferProxyTextureRHI(this, BackBufferDesc);
    }

    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        TComPtr<ID3D12Resource> D3DBackBufferResource;

        HRESULT Result = SwapChain->GetBuffer(Index, IID_PPV_ARGS(&D3DBackBufferResource));
        if (FAILED(Result))
        {
            D3D12_INFO("[FD3D12SwapChainRHI]: GetBuffer(%u) Failed", Index);
            return false;
        }

        FD3D12ResourceRef BackBufferResource = new FD3D12Resource(
            GetDevice(),
            D3DBackBufferResource.ReleaseOwnership(),
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_STATE_PRESENT);

        BackBufferResource->DisableDeferredRelease();

        BackBuffers[Index].Texture->SetResource(BackBufferResource.Get());
        BackBuffers[Index].Texture->GetResource()->SetDebugName(String::Printf("BackBuffer[%u]", Index));
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

    for (FBackBufferData& Data : BackBuffers)
    {
        Data.RenderTargetView    = nullptr;
        Data.UnorderedAccessView = nullptr;
    }

    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        FD3D12TextureRHI* BackBufferTexture = BackBuffers[Index].Texture.Get();

        if (Desc.IsRenderTarget())
        {
            D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
            RTVDesc.Format               = ConvertFormat(Desc.ColorFormat);
            RTVDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
            RTVDesc.Texture2D.MipSlice   = 0;
            RTVDesc.Texture2D.PlaneSlice = 0;

            FD3D12RenderTargetViewRHIRef NewRTV = new FD3D12RenderTargetViewRHI(GetDevice(), GetDevice()->GetRenderTargetOfflineDescriptorHeap(), BackBufferTexture, FRHIRenderTargetViewDesc::CreateTexture2D(Desc.ColorFormat, 0));
            if (!NewRTV->Initialize(BackBufferTexture->GetResource(), RTVDesc))
            {
                D3D12_ERROR("[FD3D12SwapChainRHI]: Failed to create back-buffer RTV for index %u", Index);
                return false;
            }

            BackBuffers[Index].RenderTargetView = NewRTV;
        }

        if (Desc.IsUnorderedAccess())
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
            UAVDesc.Format                = ConvertFormat(Desc.ColorFormat);
            UAVDesc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE2D;
            UAVDesc.Texture2D.MipSlice    = 0;
            UAVDesc.Texture2D.PlaneSlice  = 0;

            FD3D12UnorderedAccessViewRHIRef NewUAV = new FD3D12UnorderedAccessViewRHI(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap(), BackBufferTexture, FRHIUnorderedAccessViewDesc::CreateTexture2D(Desc.ColorFormat, 0));
            if (!NewUAV->Initialize(nullptr, BackBufferTexture->GetResource(), UAVDesc))
            {
                D3D12_ERROR("[FD3D12SwapChainRHI]: Failed to create back-buffer UAV for index %u", Index);
                return false;
            }

            BackBuffers[Index].UnorderedAccessView = NewUAV;
        }
    }

    if (Desc.IsRenderTarget() && !BackBufferProxyRenderTargetView)
    {
        BackBufferProxyRenderTargetView = new FD3D12BackBufferProxyRenderTargetViewRHI(this, BackBufferProxy.Get());
        BackBufferProxy->SetProxyRenderTargetView(BackBufferProxyRenderTargetView.Get());
    }

    if (Desc.IsUnorderedAccess() && !BackBufferProxyUnorderedAccessView)
    {
        BackBufferProxyUnorderedAccessView = new FD3D12BackBufferProxyUnorderedAccessViewRHI(this, BackBufferProxy.Get());
        BackBufferProxy->SetProxyUnorderedAccessView(BackBufferProxyUnorderedAccessView.Get());
    }

    if (FD3D12TextureRHI* CurrentBackbuffer = BackBufferProxy->GetTextureInterface())
    {
        return true;
    }
    else
    {
        return false;
    }
}
