#include "D3D12Interface.h"
#include "D3D12Viewport.h"
#include "Core/Misc/FrameProfiler.h"

FD3D12Viewport::FD3D12Viewport(FD3D12Device* InDevice, FD3D12CommandContext* InCmdContext, const FRHIViewportDesc& InDesc)
    : FD3D12DeviceChild(InDevice)
    , FRHIViewport(InDesc)
    , SwapChain(nullptr)
    , CommandContext(InCmdContext)
    , BackBuffer(nullptr)
    , BackBuffers()
    , Hwnd(reinterpret_cast<HWND>(InDesc.WindowHandle))
    , SwapChainWaitableObject(0)
    , Flags(0)
    , NumBackBuffers(0)
    , BackBufferIndex(0)
{ }

FD3D12Viewport::~FD3D12Viewport()
{
    BOOL FullscreenState;

    HRESULT Result = SwapChain->GetFullscreenState(&FullscreenState, nullptr);
    if (SUCCEEDED(Result))
    {
        if (FullscreenState)
        {
            SwapChain->SetFullscreenState(FALSE, nullptr);
        }
    }

    if (SwapChainWaitableObject)
    {
        CloseHandle(SwapChainWaitableObject);
    }

    BackBuffer->SetViewport(nullptr);
}

bool FD3D12Viewport::Initialize()
{
    // Save the flags
    Flags = GetDevice()->GetAdapter()->SupportsTearing() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    Flags = Flags | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    const uint32      NumSwapChainBuffers = D3D12_NUM_BACK_BUFFERS;
    const DXGI_FORMAT NativeFormat        = ConvertFormat(Desc.ColorFormat);

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

    D3D12_ERROR_COND(Desc.Width  != 0, "Viewport-width of zero is not supported");
    D3D12_ERROR_COND(Desc.Height != 0, "Viewport-height of zero is not supported");

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
    FMemory::Memzero(&SwapChainDesc);

    SwapChainDesc.Width              = Desc.Width;
    SwapChainDesc.Height             = Desc.Height;
    SwapChainDesc.Format             = NativeFormat;
    SwapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount        = NumSwapChainBuffers;
    SwapChainDesc.SampleDesc.Count   = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    SwapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;
    SwapChainDesc.Flags              = Flags;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullscreenDesc;
    FMemory::Memzero(&FullscreenDesc);

    FullscreenDesc.RefreshRate.Numerator   = 0;
    FullscreenDesc.RefreshRate.Denominator = 1;
    FullscreenDesc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
    FullscreenDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    FullscreenDesc.Windowed                = true;

    IDXGIFactory2* Factory = GetDevice()->GetAdapter()->GetDXGIFactory();
    CHECK(Factory != nullptr);

    TComPtr<IDXGISwapChain1> TempSwapChain;

    HRESULT Result = Factory->CreateSwapChainForHwnd(
        GetDevice()->GetD3D12CommandQueue(ED3D12CommandQueueType::Direct),
        Hwnd,
        &SwapChainDesc,
        &FullscreenDesc,
        nullptr,
        &TempSwapChain);
    if (SUCCEEDED(Result))
    {
        Result = TempSwapChain.GetAs<IDXGISwapChain3>(&SwapChain);
        if (FAILED(Result))
        {
            D3D12_ERROR("[FD3D12Viewport]: FAILED to retrieve IDXGISwapChain3");
            return false;
        }

        NumBackBuffers = NumSwapChainBuffers;

        if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
        {
            SwapChainWaitableObject = SwapChain->GetFrameLatencyWaitableObject();
        }

        SwapChain->SetMaximumFrameLatency(5);
    }
    else
    {
        D3D12_ERROR("[FD3D12Viewport]: FAILED to create SwapChain");
        return false;
    }

    Factory->MakeWindowAssociation(Hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (!RetriveBackBuffers())
    {
        return false;
    }

    D3D12_INFO("[FD3D12Viewport]: Created SwapChain");
    return true;
}

bool FD3D12Viewport::Resize(uint32 InWidth, uint32 InHeight)
{
    if ((InWidth != Desc.Width || InHeight != Desc.Height) && (InWidth > 0) && (InHeight > 0))
    {
        GRHICommandExecutor.WaitForOutstandingTasks();

        CommandContext->ClearState();

        for (FD3D12TextureRef& Texture : BackBuffers)
        {
            Texture->SetResource(nullptr);
        }

        HRESULT Result = SwapChain->ResizeBuffers(0, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, Flags);
        if (SUCCEEDED(Result))
        {
            Desc.Width  = uint16(InWidth);
            Desc.Height = uint16(InHeight);
        }
        else
        {
            D3D12_WARNING("[FD3D12Viewport]: Resize FAILED");
            return false;
        }

        if (!RetriveBackBuffers())
        {
            return false;
        }

        D3D12_INFO("[FD3D12Viewport]: Resized %u x %u", Desc.Width, Desc.Height);
    }

    // NOTE: Not considered an error to try to resize when the size is the same, maybe it should?
    return true;
}

bool FD3D12Viewport::Present(bool VerticalSync)
{
    TRACE_FUNCTION_SCOPE();

    const uint32 SyncInterval = !!VerticalSync;

    uint32 PresentFlags = 0;
    if (SyncInterval == 0 && (Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING))
    {
        PresentFlags = DXGI_PRESENT_ALLOW_TEARING;
    }

    HRESULT Result = SwapChain->Present(SyncInterval, PresentFlags);
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }

    if (SUCCEEDED(Result))
    {
        BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

        if (Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
        {
            Result = WaitForSingleObjectEx(SwapChainWaitableObject, INFINITE, true);
            if (FAILED(Result))
            {
                return false;
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool FD3D12Viewport::RetriveBackBuffers()
{
    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(
        GetColorFormat(),
        Desc.Width,
        Desc.Height,
        1, 
        1,
        ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable);

    if (BackBuffers.Size() < static_cast<int32>(NumBackBuffers))
    {
        BackBuffers.Resize(NumBackBuffers);
        for (FD3D12TextureRef& Texture : BackBuffers)
        {
            Texture = new FD3D12Texture(GetDevice(), BackBufferDesc);
        }
    }

    BackBuffer = new FD3D12BackBufferTexture(GetDevice(), this, BackBufferDesc);

    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        TComPtr<ID3D12Resource> BackBufferResource;

        HRESULT Result = SwapChain->GetBuffer(Index, IID_PPV_ARGS(&BackBufferResource));
        if (FAILED(Result))
        {
            D3D12_INFO("[FD3D12Viewport]: GetBuffer(%u) Failed", Index);
            return false;
        }

        BackBuffers[Index]->SetResource(new FD3D12Resource(GetDevice(), BackBufferResource));
        BackBuffers[Index]->GetD3D12Resource()->SetName(FString::CreateFormatted("BackBuffer[%u]", Index));
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

    FD3D12Texture* CurrentBackbuffer = BackBuffer->GetCurrentBackBufferTexture();
    if (!CurrentBackbuffer)
    {
        return false;
    }

    return true;
}
