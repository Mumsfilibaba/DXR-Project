#include "D3D12Viewport.h"
#include "D3D12CommandQueue.h"
#include "D3D12RenderLayer.h"

D3D12Viewport::D3D12Viewport(D3D12Device* InDevice, D3D12CommandContext* InCmdContext, HWND InHwnd, EFormat InFormat, UInt32 InWidth, UInt32 InHeight)
    : D3D12DeviceChild(InDevice)
    , Viewport(InFormat, InWidth, InHeight)
    , Hwnd(InHwnd)
    , SwapChain(nullptr)
    , CmdContext(InCmdContext)
    , BackBuffers()
    , BackBufferViews()
{
}

D3D12Viewport::~D3D12Viewport()
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
}

Bool D3D12Viewport::Init()
{
    // Save the flags
    Flags = GetDevice()->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    Flags = Flags | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    const UInt32 NumSwapChainBuffers = 3;
    const DXGI_FORMAT NativeFormat   = ConvertFormat(Format);

    Assert(Width > 0 && Height > 0);

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
    Memory::Memzero(&SwapChainDesc);

    SwapChainDesc.Width              = Width;
    SwapChainDesc.Height             = Height;
    SwapChainDesc.Format             = NativeFormat;
    SwapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount        = NumSwapChainBuffers;
    SwapChainDesc.SampleDesc.Count   = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;
    SwapChainDesc.Flags              = Flags;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullscreenDesc;
    Memory::Memzero(&FullscreenDesc);

    FullscreenDesc.RefreshRate.Numerator   = 0;
    FullscreenDesc.RefreshRate.Denominator = 1;
    FullscreenDesc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
    FullscreenDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    FullscreenDesc.Windowed                = true;

    TComPtr<IDXGISwapChain1> TempSwapChain;
    HRESULT Result = GetDevice()->GetFactory()->CreateSwapChainForHwnd(CmdContext->GetQueue().GetQueue(), Hwnd, &SwapChainDesc, &FullscreenDesc, nullptr, &TempSwapChain);
    if (SUCCEEDED(Result))
    {
        Result = TempSwapChain.As<IDXGISwapChain3>(&SwapChain);
        if (FAILED(Result))
        {
            LOG_ERROR("[D3D12Viewport]: FAILED to retrive IDXGISwapChain3");
            return false;
        }

        NumBackBuffers = NumSwapChainBuffers;

        SwapChain->SetMaximumFrameLatency(1);
    }
    else
    {
        LOG_ERROR("[D3D12Viewport]: FAILED to create SwapChain");
        return false;
    }

    GetDevice()->GetFactory()->MakeWindowAssociation(Hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (!RetriveBackBuffers())
    {
        return false;
    }

    LOG_INFO("[D3D12Viewport]: Created SwapChain");
    return true;
}

Bool D3D12Viewport::Resize(UInt32 InWidth, UInt32 InHeight)
{
    // TODO: Make sure that we release the old surfaces

    if ((InWidth != Width || InHeight != Height) && InWidth > 0 && InHeight > 0)
    {
        CmdContext->ClearState();

        BackBuffers.Clear();
        BackBufferViews.Clear();

        HRESULT Result = SwapChain->ResizeBuffers(0, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, Flags);
        if (SUCCEEDED(Result))
        {
            Width  = InWidth;
            Height = InHeight;
        }
        else
        {
            LOG_WARNING("[D3D12Viewport]: Resize FAILED");
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

Bool D3D12Viewport::Present(Bool VerticalSync)
{
    const UInt32 SyncInterval = !!VerticalSync;
    
    UInt32 PresentFlags = 0;
    if (SyncInterval == 0 && Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
    {
        PresentFlags = DXGI_PRESENT_ALLOW_TEARING;
    }

    HRESULT Result = SwapChain->Present(SyncInterval, PresentFlags);
    if (SUCCEEDED(Result))
    {
        BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
        return true;
    }
    else
    {
        return false;
    }
}

void D3D12Viewport::SetName(const std::string& InName)
{
    SwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(InName.size()), InName.data());
    
    UInt32 Index = 0;
    for (TRef<D3D12Texture2D>& Buffer : BackBuffers)
    {
        Buffer->SetName(InName + "Buffer [" + std::to_string(Index) + "]");
        Index++;
    }
}

Bool D3D12Viewport::RetriveBackBuffers()
{
    if (BackBuffers.Size() < NumBackBuffers)
    {
        BackBuffers.Resize(NumBackBuffers);
    }

    if (BackBufferViews.Size() < NumBackBuffers)
    {
        D3D12OfflineDescriptorHeap* RenderTargetOfflineHeap = gD3D12RenderLayer->GetRenderTargetOfflineDescriptorHeap();
        BackBufferViews.Resize(NumBackBuffers);
        for (TRef<D3D12RenderTargetView>& View : BackBufferViews)
        {
            if (!View)
            {
                View = DBG_NEW D3D12RenderTargetView(GetDevice(), RenderTargetOfflineHeap);
                if (!View->Init())
                {
                    return false;
                }
            }
        }
    }

    for (UInt32 i = 0; i < NumBackBuffers; i++)
    {
        TComPtr<ID3D12Resource> BackBufferResource;
        HRESULT Result = SwapChain->GetBuffer(i, IID_PPV_ARGS(&BackBufferResource));
        if (FAILED(Result))
        {
            LOG_INFO("[D3D12Viewport]: GetBuffer(" + std::to_string(i) + ") Failed");
            return false;
        }

        BackBuffers[i] = DBG_NEW D3D12Texture2D(GetDevice(), GetColorFormat(), Width, Height, 1, 1, 1, TextureFlag_RTV, ClearValue());
        BackBuffers[i]->SetResource(DBG_NEW D3D12Resource(GetDevice(), BackBufferResource));

        D3D12_RENDER_TARGET_VIEW_DESC Desc;
        Memory::Memzero(&Desc);

        Desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        Desc.Format               = BackBuffers[i]->GetNativeFormat();
        Desc.Texture2D.MipSlice   = 0;
        Desc.Texture2D.PlaneSlice = 0;

        if (!BackBufferViews[i]->CreateView(BackBuffers[i]->GetResource(), Desc))
        {
            return false;
        }

        BackBuffers[i]->SetRenderTargetView(BackBufferViews[i].Get());
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    return true;
}
