#include "D3D12CommandQueue.h"
#include "D3D12CoreInterface.h"
#include "D3D12Viewport.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Viewport

CD3D12Viewport::CD3D12Viewport(CD3D12Device* InDevice, CD3D12CommandContext* InCmdContext, const CRHIViewportInitializer& Initializer)
    : CD3D12DeviceChild(InDevice)
    , CRHIViewport(Initializer)
    , Hwnd(reinterpret_cast<HWND>(Initializer.WindowHandle))
    , SwapChain(nullptr)
    , CmdContext(InCmdContext)
    , BackBuffers()
    , BackBufferViews()
{ }

CD3D12Viewport::~CD3D12Viewport()
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
}

bool CD3D12Viewport::Init()
{
    // Save the flags
    Flags = GetDevice()->CanAllowTearing() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    Flags = Flags | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    const uint32      NumSwapChainBuffers = D3D12_NUM_BACK_BUFFERS;
    const DXGI_FORMAT NativeFormat        = ConvertFormat(Format);

    RECT ClientRect;
    GetClientRect(Hwnd, &ClientRect);

    if (Width == 0)
    {
        Width = uint16(ClientRect.right - ClientRect.left);
    }

    if (Height == 0)
    {
        Height = uint16(ClientRect.bottom - ClientRect.top);
    }

    D3D12_ERROR_COND(Width  != 0, "Viewport-width of zero is not supported");
    D3D12_ERROR_COND(Height != 0, "Viewport-height of zero is not supported");

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
    CMemory::Memzero(&SwapChainDesc);

    SwapChainDesc.Width              = Width;
    SwapChainDesc.Height             = Height;
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
    CMemory::Memzero(&FullscreenDesc);

    FullscreenDesc.RefreshRate.Numerator   = 0;
    FullscreenDesc.RefreshRate.Denominator = 1;
    FullscreenDesc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
    FullscreenDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    FullscreenDesc.Windowed                = true;

    TComPtr<IDXGISwapChain1> TempSwapChain;
    HRESULT Result = GetDevice()->GetFactory()->CreateSwapChainForHwnd(CmdContext->GetQueue().GetQueue(), Hwnd, &SwapChainDesc, &FullscreenDesc, nullptr, &TempSwapChain);
    if (SUCCEEDED(Result))
    {
        Result = TempSwapChain.GetAs<IDXGISwapChain3>(&SwapChain);
        if (FAILED(Result))
        {
            D3D12_ERROR("[CD3D12Viewport]: FAILED to retrieve IDXGISwapChain3");
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
        D3D12_ERROR("[CD3D12Viewport]: FAILED to create SwapChain");
        return false;
    }

    GetDevice()->GetFactory()->MakeWindowAssociation(Hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (!RetriveBackBuffers())
    {
        return false;
    }

    D3D12_INFO("[CD3D12Viewport]: Created SwapChain");
    return true;
}

bool CD3D12Viewport::Resize(uint32 InWidth, uint32 InHeight)
{
    // TODO: Make sure that we release the old surfaces

    if ((InWidth != Width || InHeight != Height) && (InWidth > 0) && (InHeight > 0))
    {
        CmdContext->ClearState();

        BackBuffers.Clear();
        BackBufferViews.Clear();

        HRESULT Result = SwapChain->ResizeBuffers(0, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, Flags);
        if (SUCCEEDED(Result))
        {
            Width  = uint16(InWidth);
            Height = uint16(InHeight);
        }
        else
        {
            D3D12_WARNING("[CD3D12Viewport]: Resize FAILED");
            return false;
        }

        if (!RetriveBackBuffers())
        {
            return false;
        }

    }

    // NOTE: Not considered an error to try to resize when the size is the same, maybe it should?
    return true;
}

bool CD3D12Viewport::Present(bool VerticalSync)
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
            WaitForSingleObjectEx(SwapChainWaitableObject, INFINITE, true);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CD3D12Viewport::RetriveBackBuffers()
{
    if (BackBuffers.Size() < (int32)NumBackBuffers)
    {
        BackBuffers.Resize(NumBackBuffers);
    }

    if (BackBufferViews.Size() < (int32)NumBackBuffers)
    {
        CD3D12OfflineDescriptorHeap* RenderTargetOfflineHeap = GD3D12Instance->GetRenderTargetOfflineDescriptorHeap();
        BackBufferViews.Resize(NumBackBuffers);

        for (TSharedRef<CD3D12RenderTargetView>& View : BackBufferViews)
        {
            if (!View)
            {
                View = dbg_new CD3D12RenderTargetView(GetDevice(), RenderTargetOfflineHeap);
                if (!View->AllocateHandle())
                {
                    return false;
                }
            }
        }
    }

    for (uint32 i = 0; i < NumBackBuffers; i++)
    {
        TComPtr<ID3D12Resource> BackBufferResource;
        HRESULT Result = SwapChain->GetBuffer(i, IID_PPV_ARGS(&BackBufferResource));
        if (FAILED(Result))
        {
            D3D12_INFO("[CD3D12Viewport]: GetBuffer(" + ToString(i) + ") Failed");
            return false;
        }

        CRHITexture2DInitializer BackBufferInitializer(GetColorFormat(), Width, Height, 1, 1, ETextureUsageFlags::AllowRTV, EResourceAccess::Common);
        BackBuffers[i] = dbg_new CD3D12Texture2D(GetDevice(), BackBufferInitializer);
        BackBuffers[i]->SetResource(dbg_new CD3D12Resource(GetDevice(), BackBufferResource));

        D3D12_RENDER_TARGET_VIEW_DESC Desc;
        CMemory::Memzero(&Desc);

        Desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        Desc.Format               = BackBuffers[i]->GetDXGIFormat();
        Desc.Texture2D.MipSlice   = 0;
        Desc.Texture2D.PlaneSlice = 0;

        if (!BackBufferViews[i]->CreateView(BackBuffers[i]->GetD3D12Resource(), Desc))
        {
            return false;
        }

        BackBufferViews[i].AddRef();
        BackBuffers[i]->SetRenderTargetView(BackBufferViews[i].Get());
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    return true;
}
