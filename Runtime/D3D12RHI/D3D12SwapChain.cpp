#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12SwapChain.h"

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

FD3D12SwapChainRHI::FD3D12SwapChainRHI(FD3D12Device* InDevice, FD3D12CommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc)
    : FD3D12DeviceChild(InDevice)
    , FRHISwapChain(InSwapChainDesc)
    , SwapChain(nullptr)
    , CommandContext(InCommandContext)
    , BackBufferProxy(nullptr)
    , BackBuffers()
    , Hwnd(reinterpret_cast<HWND>(InSwapChainDesc.WindowHandle))
    , SwapChainWaitableObject(0)
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

    const uint32 NumSwapChainBuffers = Math::Clamp<int32>(CVarSwapChainBackBufferCount.GetValue(), 2, 8);

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.Width              = Desc.Width;
    SwapChainDesc.Height             = Desc.Height;
    SwapChainDesc.Format             = ConvertFormat(Desc.ColorFormat);
    SwapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
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

    if (!RetrieveBackBuffers())
    {
        return false;
    }

    D3D12_INFO("[FD3D12SwapChainRHI]: Created SwapChain");
    return true;
}

bool FD3D12SwapChainRHI::Resize(FD3D12CommandContext* InCommandContext, uint32 InWidth, uint32 InHeight)
{
    const uint32 DesiredBackBufferCount = Math::Clamp<int32>(CVarSwapChainBackBufferCount.GetValue(), 2, 8);
    
    const bool bSizeChanged        = (InWidth != Desc.Width || InHeight != Desc.Height) && InWidth > 0u && InHeight > 0u;
    const bool bBufferCountChanged = DesiredBackBufferCount != NumBackBuffers;

    if (bSizeChanged || bBufferCountChanged)
    {
        if (InCommandContext->IsRecording())
        {
            InCommandContext->SplitCommandListAndResetState(false, true);
        }
        else
        {
            InCommandContext->ClearState();
        }

        for (FD3D12TextureRHIRef& Texture : BackBuffers)
        {
            Texture->SetResource(nullptr);
        }

        const uint32 ResizeWidth  = bSizeChanged ? InWidth  : Desc.Width;
        const uint32 ResizeHeight = bSizeChanged ? InHeight : Desc.Height;

        HRESULT Result = SwapChain->ResizeBuffers(DesiredBackBufferCount, ResizeWidth, ResizeHeight, DXGI_FORMAT_UNKNOWN, Flags);
        if (SUCCEEDED(Result))
        {
            NumBackBuffers = DesiredBackBufferCount;

            if (bSizeChanged)
            {
                Desc.Width  = uint16(InWidth);
                Desc.Height = uint16(InHeight);
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

        D3D12_INFO("[FD3D12SwapChainRHI]: Resized %u x %u (backbuffers=%u)", Desc.Width, Desc.Height, NumBackBuffers);
    }

    // Apply frame latency changes if needed
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

void* FD3D12SwapChainRHI::GetBackBufferRenderTargetView()
{
    FD3D12TextureRHI* CurrentBackBuffer = BackBuffers[BackBufferIndex].Get();
    return CurrentBackBuffer->GetOrCreateRenderTargetView(FRHIRenderTargetView(CurrentBackBuffer));
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
#if D3D12_ENABLE_DEVICE_LOST_CHECK
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }
#endif

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
        CommandContext->Flush();

        for (FD3D12TextureRHIRef& Texture : BackBuffers)
        {
            Texture->SetResource(nullptr);
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
    const ETextureUsageFlags UsageFlags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable;
    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(GetColorFormat(), GetWidth(), GetHeight(), 1, 1, UsageFlags);

    if (BackBuffers.Size() < static_cast<int32>(NumBackBuffers))
    {
        BackBuffers.Resize(NumBackBuffers);
        for (FD3D12TextureRHIRef& Texture : BackBuffers)
        {
            Texture = new FD3D12TextureRHI(GetDevice(), BackBufferDesc);
        }
    }

    if (BackBufferProxy)
    {
        BackBufferProxy->Resize(GetWidth(), GetHeight());
    }
    else
    {
        BackBufferProxy = new FD3D12BackBufferTexture(GetDevice(), this, BackBufferDesc);
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
        
        BackBuffers[Index]->SetResource(BackBufferResource.Get());
        BackBuffers[Index]->GetResource()->SetDebugName(FString::CreateFormatted("BackBuffer[%u]", Index));
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

    if (FD3D12TextureRHI* CurrentBackbuffer = BackBufferProxy->GetCurrentBackBufferTexture())
    {
        return true;
    }
    else
    {
        return false;
    }
}
