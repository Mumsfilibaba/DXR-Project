#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "RHI/RHI.h"
#include "D3D11RHI/D3D11SwapChain.h"
#include "D3D11RHI/D3D11Capabilities.h"
#include "D3D11RHI/D3D11Composition.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11CommandContext.h"

static TAutoConsoleVariable<int32> CVarSwapChainBackBufferCount(
    "D3D11RHI.SwapChain.BackBufferCount",
    "Number of swap chain back buffers",
    D3D11_NUM_BACK_BUFFERS);

static TAutoConsoleVariable<int32> CVarSyncInterval(
    "D3D11RHI.SwapChain.SyncInterval",
    "Override sync interval for Present (-1 = use VSync setting, 0-4 = explicit interval)",
    -1);

static TAutoConsoleVariable<int32> CVarMaxFrameLatency(
    "D3D11RHI.SwapChain.MaxFrameLatency",
    "Maximum frame latency for the swap chain (-1 = use backbuffer count)",
    -1);

static TAutoConsoleVariable<int32> CVarD3D11DefaultBackBufferFormat(
    "D3D11RHI.DefaultBackBufferFormat",
    "Default back-buffer format used when ColorFormat is Unknown. "
    "0=B8G8R8A8_Unorm, "
    "1=R8G8B8A8_Unorm (default), "
    "2=R10G10B10A2_Unorm, "
    "3=R16G16B16A16_Float.",
    1);

static TAutoConsoleVariable<int32> CVarD3D11DefaultBackBufferColorSpace(
    "D3D11RHI.DefaultBackBufferColorSpace",
    "Default back-buffer color space used when ColorSpace is Unknown. "
    "0=RGB_Full_G22_None_P709 / sRGB (default), "
    "1=RGB_Full_G10_None_P709 / scRGB, "
    "2=RGB_Full_G2084_None_P2020 / HDR10, "
    "3=RGB_Full_G22_None_P2020.",
    0);

EFormat FD3D11SwapChainRHI::GetDefaultBackBufferFormat()
{
    static constexpr EFormat FormatTable[] =
    {
        EFormat::B8G8R8A8_Unorm,      // 0
        EFormat::R8G8B8A8_Unorm,      // 1 (D3D11 default)
        EFormat::R10G10B10A2_Unorm,   // 2 (HDR10 candidate)
        EFormat::R16G16B16A16_Float,  // 3 (scRGB candidate)
    };

    int32 Index = CVarD3D11DefaultBackBufferFormat.GetValue();
    if (Index < 0 || Index >= static_cast<int32>(ARRAY_COUNT(FormatTable)))
    {
        D3D11_WARNING("D3D11RHI.DefaultBackBufferFormat=%d is out of range [0..%d]; clamping to 0.", Index, static_cast<int32>(ARRAY_COUNT(FormatTable)) - 1);
        Index = 0;
    }

    return FormatTable[Index];
}

static EColorSpace GetD3D11DefaultBackBufferColorSpace()
{
    static constexpr EColorSpace ColorSpaceTable[] =
    {
        EColorSpace::RGB_Full_G22_None_P709,    // 0 (default)
        EColorSpace::RGB_Full_G10_None_P709,    // 1 (scRGB)
        EColorSpace::RGB_Full_G2084_None_P2020, // 2 (HDR10)
        EColorSpace::RGB_Full_G22_None_P2020,   // 3
    };

    int32 Index = CVarD3D11DefaultBackBufferColorSpace.GetValue();
    if (Index < 0 || Index >= static_cast<int32>(ARRAY_COUNT(ColorSpaceTable)))
    {
        D3D11_WARNING("D3D11RHI.DefaultBackBufferColorSpace=%d is out of range [0..%d]; clamping to 0.", Index, static_cast<int32>(ARRAY_COUNT(ColorSpaceTable)) - 1);
        Index = 0;
    }

    return ColorSpaceTable[Index];
}

FD3D11SwapChainRHI::FD3D11SwapChainRHI(FD3D11Device* InDevice, FD3D11CommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc)
    : FRHISwapChain(InSwapChainDesc)
    , FD3D11DeviceChild(InDevice)
    , SwapChain(nullptr)
    , SwapChain4(nullptr)
#if D3D11_ENABLE_COMPOSITION
    , Composition(nullptr)
#endif
    , CommandContext(InCommandContext)
    , BackBuffer(nullptr)
    , Hwnd(reinterpret_cast<HWND>(InSwapChainDesc.WindowHandle))
    , SwapChainWaitableObject(0)
    , AppliedHDRMetadataType(DXGI_HDR_METADATA_TYPE_NONE)
    , AppliedHDR10Metadata()
    , CurrentColorSpace(EColorSpace::RGB_Full_G22_None_P709)
    , Flags(0)
    , NumBackBuffers(0)
    , ActiveFrameLatency(0)
{
}

FD3D11SwapChainRHI::~FD3D11SwapChainRHI()
{
#if D3D11_ENABLE_COMPOSITION
    const bool bHasFullscreenState = SwapChain.IsValid() && !Composition;
#else
    const bool bHasFullscreenState = SwapChain.IsValid();
#endif

    if (bHasFullscreenState)
    {
        BOOL FullscreenState = FALSE;
        if (SUCCEEDED(SwapChain->GetFullscreenState(&FullscreenState, nullptr)) && FullscreenState)
        {
            SwapChain->SetFullscreenState(FALSE, nullptr);
        }
    }

#if D3D11_ENABLE_COMPOSITION
    Composition.Reset();
#endif

    if (SwapChainWaitableObject)
    {
        CloseHandle(SwapChainWaitableObject);
    }

    ReleaseBackBufferResources();
}

bool FD3D11SwapChainRHI::Initialize()
{
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
        D3D11_ERROR_CRITICAL("SwapChain width of zero is not supported");
        return false;
    }

    if (!Desc.Height)
    {
        D3D11_ERROR_CRITICAL("SwapChain height of zero is not supported");
        return false;
    }

    if (!Desc.IsRenderTarget() && !Desc.IsUnorderedAccess())
    {
        D3D11_ERROR("[FD3D11SwapChainRHI]: Desc.Usage must include at least one of ESwapChainUsageFlags::RenderTarget or ESwapChainUsageFlags::UnorderedAccess");
        return false;
    }

    EFormat ResolvedFormat = EFormat::Unknown;
    if (Desc.ColorFormat == EFormat::Unknown)
    {
        ResolvedFormat = GetDefaultBackBufferFormat();
    }
    else
    {
        if (!GetDevice()->SupportsSwapChainFormat(ConvertFormat(Desc.ColorFormat), Desc.Usage))
        {
            D3D11_ERROR("[FD3D11SwapChainRHI]: Requested back-buffer format %s is not supported on this device.", ToString(Desc.ColorFormat));
            return false;
        }

        ResolvedFormat = Desc.ColorFormat;
    }

    Desc.ColorFormat = ResolvedFormat;

    const EColorSpace ResolvedColorSpace = (Desc.ColorSpace == EColorSpace::Unknown)
        ? GetD3D11DefaultBackBufferColorSpace()
        : Desc.ColorSpace;

    const uint32 NumSwapChainBuffers = Math::Clamp<int32>(CVarSwapChainBackBufferCount.GetValue(), 2, 8);

#if D3D11_ENABLE_COMPOSITION
    const bool bUseComposition = Desc.IsTransparent() && GD3D11SupportsComposition;
#else
    const bool bUseComposition = false;
#endif

    if (Desc.IsTransparent() && !bUseComposition)
    {
        D3D11_WARNING("[FD3D11SwapChainRHI]: DirectComposition unavailable, falling back to an opaque swap chain");
        SetEnumFlag(Desc.Flags, ESwapChainFlags::Transparent, false);
    }

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.Width              = Desc.Width;
    SwapChainDesc.Height             = Desc.Height;
    SwapChainDesc.Format             = ConvertFormat(ResolvedFormat);
    SwapChainDesc.BufferUsage        = D3D11ConvertSwapChainUsage(Desc.Usage);
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

    // D3D11 presents through the device, where D3D12 presents through the direct queue
    ID3D11Device* D3D11Device = GetDevice()->GetD3D11Device();

    TComPtr<IDXGISwapChain1> DXGISwapChain1;
    HRESULT Result = bUseComposition
        ? Factory->CreateSwapChainForComposition(D3D11Device, &SwapChainDesc, nullptr, &DXGISwapChain1)
        : Factory->CreateSwapChainForHwnd(D3D11Device, Hwnd, &SwapChainDesc, &FullscreenDesc, nullptr, &DXGISwapChain1);

    if (SUCCEEDED(Result))
    {
        Result = DXGISwapChain1.GetAs<IDXGISwapChain3>(&SwapChain);
        if (FAILED(Result))
        {
            D3D11_ERROR_CRITICAL("[FD3D11SwapChainRHI]: FAILED to retrieve IDXGISwapChain3");
            return false;
        }

    #if D3D11_ENABLE_COMPOSITION
        if (bUseComposition)
        {
            FD3D11CompositionRef NewComposition = new FD3D11Composition(GetDevice());
            if (!NewComposition->Initialize(Hwnd, DXGISwapChain1.Get()))
            {
                D3D11_ERROR_CRITICAL("[FD3D11SwapChainRHI]: FAILED to bind the SwapChain to a composition visual");
                return false;
            }

            Composition = NewComposition;
        }
    #endif

        if (FAILED(DXGISwapChain1.GetAs<IDXGISwapChain4>(&SwapChain4)))
        {
            D3D11_WARNING("[FD3D11SwapChainRHI]: IDXGISwapChain4 unavailable; HDR metadata will not be submitted");
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
        D3D11_ERROR_CRITICAL("[FD3D11SwapChainRHI]: FAILED to create SwapChain (Result=0x%08X, Composition=%s, %ux%u, Format=%s, BufferCount=%u, BufferUsage=0x%08X, Flags=0x%08X, Hwnd=%p)",
            static_cast<uint32>(Result), bUseComposition ? "Yes" : "No", SwapChainDesc.Width, SwapChainDesc.Height,
            ToString(ResolvedFormat), SwapChainDesc.BufferCount, static_cast<uint32>(SwapChainDesc.BufferUsage),
            static_cast<uint32>(SwapChainDesc.Flags), Hwnd);
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
            D3D11_ERROR("[FD3D11SwapChainRHI]: Requested (%s, %s) combination not supported on this output; aborting.",
                ToString(ResolvedFormat), ToString(ResolvedColorSpace));
            return false;
        }

        const HRESULT SetColorSpaceResult = SwapChain->SetColorSpace1(RequestedDXGI);
        if (FAILED(SetColorSpaceResult))
        {
            D3D11_ERROR("[FD3D11SwapChainRHI]: SetColorSpace1 failed (hr=0x%x).", static_cast<uint32>(SetColorSpaceResult));
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

    if (!RetrieveBackBuffer())
    {
        return false;
    }

    D3D11_INFO("[FD3D11SwapChainRHI]: Created SwapChain (%s, %s, BufferCount=%u, AlphaMode=%s, Composition=%s, AllowTearing=%s)",
        ToString(ResolvedFormat), ToString(ResolvedColorSpace), NumSwapChainBuffers, bUseComposition ? "Premultiplied" : "Ignore",
        bUseComposition ? "Yes" : "No", (Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) ? "Yes" : "No");

    return true;
}

bool FD3D11SwapChainRHI::Resize(uint32 InWidth, uint32 InHeight, EFormat NewFormat, EColorSpace NewColorSpace)
{
    const uint32      ResolvedWidth          = (InWidth  > 0u) ? InWidth  : Desc.Width;
    const uint32      ResolvedHeight         = (InHeight > 0u) ? InHeight : Desc.Height;
    const EFormat     EffectiveFormat        = (NewFormat     == EFormat::Unknown)     ? Desc.ColorFormat  : NewFormat;
    const EColorSpace EffectiveColorSpace    = (NewColorSpace == EColorSpace::Unknown) ? CurrentColorSpace : NewColorSpace;
    const uint32      DesiredBackBufferCount = Math::Clamp<int32>(CVarSwapChainBackBufferCount.GetValue(), 2, 8);

    const bool bSizeChanged        = (ResolvedWidth != Desc.Width || ResolvedHeight != Desc.Height) && ResolvedWidth > 0u && ResolvedHeight > 0u;
    const bool bBufferCountChanged = DesiredBackBufferCount != NumBackBuffers;
    const bool bFormatChanged      = (NewFormat     != EFormat::Unknown)     && (EffectiveFormat     != Desc.ColorFormat);
    const bool bColorSpaceChanged  = (NewColorSpace != EColorSpace::Unknown) && (EffectiveColorSpace != CurrentColorSpace);

    if (bFormatChanged || bColorSpaceChanged)
    {
        if (!IsFormatSupported(EffectiveFormat, EffectiveColorSpace))
        {
            D3D11_WARNING("[FD3D11SwapChainRHI]: Resize: (%s, %s) not supported on this swap-chain; leaving unchanged.",
                ToString(EffectiveFormat), ToString(EffectiveColorSpace));
            return false;
        }
    }

    const bool bNeedsResizeBuffers = bSizeChanged || bBufferCountChanged || bFormatChanged;
    if (bNeedsResizeBuffers)
    {
        CommandContext->ClearState();
        CommandContext->Flush();

        ReleaseBackBufferResources();

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
            GetDevice()->CheckDeviceRemoved(Result, "ResizeBuffers");
            D3D11_WARNING("[FD3D11SwapChainRHI]: Resize FAILED (0x%08X)", static_cast<uint32>(Result));
            return false;
        }

        if (!RetrieveBackBuffer())
        {
            return false;
        }

        D3D11_INFO("[FD3D11SwapChainRHI]: Resized Width=%u Height=%u Format=%s Colorspace=%s BackBuffers=%u",
            Desc.Width, Desc.Height, ToString(Desc.ColorFormat), ToString(EffectiveColorSpace), NumBackBuffers);
    }

    if (bColorSpaceChanged)
    {
        const DXGI_COLOR_SPACE_TYPE NewDXGI = ConvertColorSpace(EffectiveColorSpace);
        if (FAILED(SwapChain->SetColorSpace1(NewDXGI)))
        {
            D3D11_WARNING("[FD3D11SwapChainRHI]: SetColorSpace1(%s) failed", ToString(EffectiveColorSpace));
            return false;
        }

        Desc.ColorSpace   = EffectiveColorSpace;
        CurrentColorSpace = EffectiveColorSpace;

        D3D11_INFO("[FD3D11SwapChainRHI]: Color space changed to %s", ToString(EffectiveColorSpace));
    }

    // ResizeBuffers can drop the metadata, and a color-space change can invalidate it.
    if (bNeedsResizeBuffers || bColorSpaceChanged)
    {
        ApplyHDRMetadata();
    }

    if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    {
        const int32  FrameLatencyCVar    = CVarMaxFrameLatency.GetValue();
        const uint32 DesiredFrameLatency = (FrameLatencyCVar >= 0) ? static_cast<uint32>(FrameLatencyCVar) : NumBackBuffers;

        if (DesiredFrameLatency != ActiveFrameLatency)
        {
            SwapChain->SetMaximumFrameLatency(DesiredFrameLatency);
            ActiveFrameLatency = DesiredFrameLatency;

            D3D11_INFO("[FD3D11SwapChainRHI]: Changed max frame latency to %u", ActiveFrameLatency);
        }
    }

    return true;
}

bool FD3D11SwapChainRHI::Present(bool bVerticalSync)
{
    TRACE_SCOPE("D3D11 SwapChain Present");

    const int32  SyncIntervalOverride = CVarSyncInterval.GetValue();
    const uint32 SyncInterval         = (SyncIntervalOverride >= 0) ? Math::Clamp<uint32>(SyncIntervalOverride, 0, 4) : (bVerticalSync ? 1 : 0);

    uint32 PresentFlags = 0;
    if (SyncInterval == 0 && Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
    {
        PresentFlags = DXGI_PRESENT_ALLOW_TEARING;
    }

    HRESULT Result;
    {
        TRACE_SCOPE("D3D11 DXGI Present");

        Result = SwapChain->Present(SyncInterval, PresentFlags);
        GetDevice()->CheckDeviceRemoved(Result, "Present");
    }

    if (FAILED(Result))
    {
        return false;
    }

    if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    {
        TRACE_SCOPE("D3D11 SwapChain Latency Wait");

        const DWORD WaitResult = WaitForSingleObjectEx(SwapChainWaitableObject, INFINITE, TRUE);
        if (WaitResult != WAIT_OBJECT_0)
        {
            return false;
        }
    }

    {
        TRACE_SCOPE("D3D11 SwapChain Apply Settings");
        ApplySettingsChanges();
    }

    return true;
}

void FD3D11SwapChainRHI::ApplySettingsChanges()
{
    const uint32 DesiredBackBufferCount = Math::Clamp<int32>(CVarSwapChainBackBufferCount.GetValue(), 2, 8);
    if (DesiredBackBufferCount != NumBackBuffers)
    {
        CommandContext->ClearState();
        CommandContext->Flush();

        ReleaseBackBufferResources();

        HRESULT Result = SwapChain->ResizeBuffers(DesiredBackBufferCount, Desc.Width, Desc.Height, DXGI_FORMAT_UNKNOWN, Flags);
        if (SUCCEEDED(Result))
        {
            NumBackBuffers = DesiredBackBufferCount;
            D3D11_INFO("[FD3D11SwapChainRHI]: Changed backbuffer count to %u", NumBackBuffers);
        }
        else
        {
            D3D11_WARNING("[FD3D11SwapChainRHI]: ResizeBuffers for backbuffer count change FAILED");
        }

        RetrieveBackBuffer();
    }

    if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    {
        const int32  FrameLatencyCVar    = CVarMaxFrameLatency.GetValue();
        const uint32 DesiredFrameLatency = (FrameLatencyCVar >= 0) ? static_cast<uint32>(FrameLatencyCVar) : NumBackBuffers;

        if (DesiredFrameLatency != ActiveFrameLatency)
        {
            SwapChain->SetMaximumFrameLatency(DesiredFrameLatency);
            ActiveFrameLatency = DesiredFrameLatency;
            D3D11_INFO("[FD3D11SwapChainRHI]: Changed max frame latency to %u", ActiveFrameLatency);
        }
    }
}

void* FD3D11SwapChainRHI::GetRHINativeHandle() const
{
    return SwapChain.Get();
}

void* FD3D11SwapChainRHI::GetRHINativeResourceFromIndex(uint32 Index) const
{
    return (Index == 0 && BackBuffer) ? BackBuffer->GetRHINativeResource() : nullptr;
}

void* FD3D11SwapChainRHI::GetRHINativeRenderTargetViewFromIndex(uint32 Index) const
{
    FRHIRenderTargetView* View = GetRenderTargetView();
    return (Index == 0 && View) ? View->GetRHINativeHandle() : nullptr;
}

void* FD3D11SwapChainRHI::GetRHINativeUnorderedAccessViewFromIndex(uint32 Index) const
{
    FRHIUnorderedAccessView* View = GetUnorderedAccessView();
    return (Index == 0 && View) ? View->GetRHINativeHandle() : nullptr;
}

void* FD3D11SwapChainRHI::GetRHINativeShaderResourceViewFromIndex(uint32 Index) const
{
    FRHIShaderResourceView* View = GetShaderResourceView();
    return (Index == 0 && View) ? View->GetRHINativeHandle() : nullptr;
}

FRHITexture* FD3D11SwapChainRHI::GetBackBuffer() const
{
    return BackBuffer.Get();
}

FRHIRenderTargetView* FD3D11SwapChainRHI::GetRenderTargetView() const
{
    return BackBuffer ? BackBuffer->GetRenderTargetView() : nullptr;
}

FRHIUnorderedAccessView* FD3D11SwapChainRHI::GetUnorderedAccessView() const
{
    return BackBuffer ? BackBuffer->GetUnorderedAccessView() : nullptr;
}

FRHIShaderResourceView* FD3D11SwapChainRHI::GetShaderResourceView() const
{
    return BackBuffer ? BackBuffer->GetShaderResourceView() : nullptr;
}

bool FD3D11SwapChainRHI::IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const
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

bool FD3D11SwapChainRHI::SetHDRMetadata(const FRHIHDRMetadata& Metadata)
{
    Desc.HDRMetadata = Metadata;
    return ApplyHDRMetadata();
}

bool FD3D11SwapChainRHI::ApplyHDRMetadata()
{
    if (!SwapChain4)
    {
        return false;
    }

    const bool bIsHDRColorSpace = (CurrentColorSpace == EColorSpace::RGB_Full_G2084_None_P2020);
    if (!Desc.HDRMetadata.bIsValid || !bIsHDRColorSpace)
    {
        if (AppliedHDRMetadataType == DXGI_HDR_METADATA_TYPE_NONE)
        {
            return true;
        }

        if (FAILED(SwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr)))
        {
            return false;
        }

        AppliedHDRMetadataType = DXGI_HDR_METADATA_TYPE_NONE;
        return true;
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

    if (AppliedHDRMetadataType == DXGI_HDR_METADATA_TYPE_HDR10 && Memory::Memcmp(&AppliedHDR10Metadata, &HDR10, sizeof(HDR10)) == 0)
    {
        return true;
    }

    const HRESULT Result = SwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(HDR10), &HDR10);
    if (FAILED(Result))
    {
        D3D11_WARNING("[FD3D11SwapChainRHI]: SetHDRMetaData failed (hr=0x%x)", static_cast<uint32>(Result));
        return false;
    }

    AppliedHDRMetadataType = DXGI_HDR_METADATA_TYPE_HDR10;
    AppliedHDR10Metadata   = HDR10;

    D3D11_INFO("[FD3D11SwapChainRHI]: HDR10 metadata set (max=%.1f nits, min=%.4f nits, MaxCLL=%.1f, MaxFALL=%.1f)",
        Source.MaxMasteringLuminance, Source.MinMasteringLuminance, Source.MaxContentLightLevel, Source.MaxFrameAverageLightLevel);
    return true;
}

bool FD3D11SwapChainRHI::QueryDisplayHDRInfo(FRHIDisplayHDRInfo& OutInfo) const
{
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
}

bool FD3D11SwapChainRHI::CreateBackBuffer()
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

    BackBuffer = new FD3D11TextureRHI(GetDevice(), BackBufferDesc);
    return true;
}

bool FD3D11SwapChainRHI::RetrieveBackBuffer()
{
    TComPtr<ID3D11Texture2D> D3D11BackBuffer;

    const HRESULT Result = SwapChain->GetBuffer(0, IID_PPV_ARGS(&D3D11BackBuffer));
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11SwapChainRHI]: GetBuffer(0) Failed (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    if (!BackBuffer && !CreateBackBuffer())
    {
        return false;
    }

    D3D11SetDebugName(D3D11BackBuffer.Get(), "BackBuffer");
    return BackBuffer->InitializeSwapChainTexture(D3D11BackBuffer, Desc.ColorFormat, Desc.Width, Desc.Height);
}

void FD3D11SwapChainRHI::ReleaseBackBufferResources()
{
    if (BackBuffer)
    {
        BackBuffer->ReleaseSwapChainTexture();
    }
}
