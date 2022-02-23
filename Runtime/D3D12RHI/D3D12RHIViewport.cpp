#include "D3D12CommandQueue.h"
#include "D3D12RHIInstance.h"
#include "D3D12RHIViewport.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIViewport

CD3D12RHIViewport::CD3D12RHIViewport(CD3D12Device* InDevice, CD3D12RHICommandContext* InCmdContext, HWND InHwnd, EFormat InFormat, uint32 InWidth, uint32 InHeight)
    : CD3D12DeviceChild(InDevice)
    , CRHIViewport(InFormat, InWidth, InHeight)
    , Hwnd(InHwnd)
    , SwapChain(nullptr)
    , CmdContext(InCmdContext)
    , BackBuffers()
    , BackBufferViews()
{
}

CD3D12RHIViewport::~CD3D12RHIViewport()
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

bool CD3D12RHIViewport::Init()
{
    // Save the flags
    Flags = GetDevice()->CanAllowTearing() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    Flags = Flags | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    const uint32 NumSwapChainBuffers = D3D12_NUM_BACK_BUFFERS;
    const DXGI_FORMAT NativeFormat = ConvertFormat(Format);

    Assert(Width > 0 && Height > 0);

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
    Memory::Memzero(&SwapChainDesc);

    SwapChainDesc.Width = Width;
    SwapChainDesc.Height = Height;
    SwapChainDesc.Format = NativeFormat;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = NumSwapChainBuffers;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    SwapChainDesc.Flags = Flags;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullscreenDesc;
    Memory::Memzero(&FullscreenDesc);

    FullscreenDesc.RefreshRate.Numerator = 0;
    FullscreenDesc.RefreshRate.Denominator = 1;
    FullscreenDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
    FullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    FullscreenDesc.Windowed = true;

    TComPtr<IDXGISwapChain1> TempSwapChain;
    HRESULT Result = GetDevice()->GetFactory()->CreateSwapChainForHwnd(CmdContext->GetQueue().GetQueue(), Hwnd, &SwapChainDesc, &FullscreenDesc, nullptr, &TempSwapChain);
    if (SUCCEEDED(Result))
    {
        Result = TempSwapChain.GetAs<IDXGISwapChain3>(&SwapChain);
        if (FAILED(Result))
        {
            LOG_ERROR("[CD3D12Viewport]: FAILED to retrive IDXGISwapChain3");
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
        LOG_ERROR("[CD3D12Viewport]: FAILED to create SwapChain");
        return false;
    }

    GetDevice()->GetFactory()->MakeWindowAssociation(Hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (!RetriveBackBuffers())
    {
        return false;
    }

    LOG_INFO("[CD3D12Viewport]: Created SwapChain");
    return true;
}

bool CD3D12RHIViewport::Resize(uint32 InWidth, uint32 InHeight)
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
            Width = InWidth;
            Height = InHeight;
        }
        else
        {
            LOG_WARNING("[CD3D12Viewport]: Resize FAILED");
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

bool CD3D12RHIViewport::Present(bool VerticalSync)
{
    TRACE_FUNCTION_SCOPE();

    const uint32 SyncInterval = !!VerticalSync;

    uint32 PresentFlags = 0;
    if (SyncInterval == 0 && Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
    {
        PresentFlags = DXGI_PRESENT_ALLOW_TEARING;
    }

    HRESULT Result = SwapChain->Present(SyncInterval, PresentFlags);
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        RHID3D12DeviceRemovedHandler(GetDevice());
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

void CD3D12RHIViewport::SetName(const String& InName)
{
    SwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(InName.Size()), InName.Data());

    uint32 Index = 0;
    for (TSharedRef<CD3D12RHITexture2D>& Buffer : BackBuffers)
    {
        Buffer->SetName(InName + "Buffer [" + ToString(Index) + "]");
        Index++;
    }
}

bool CD3D12RHIViewport::RetriveBackBuffers()
{
    if (BackBuffers.Size() < (int32)NumBackBuffers)
    {
        BackBuffers.Resize(NumBackBuffers);
    }

    if (BackBufferViews.Size() < (int32)NumBackBuffers)
    {
        CD3D12OfflineDescriptorHeap* RenderTargetOfflineHeap = GD3D12RHIInstance->GetRenderTargetOfflineDescriptorHeap();
        BackBufferViews.Resize(NumBackBuffers);

        for (TSharedRef<CD3D12RHIRenderTargetView>& View : BackBufferViews)
        {
            if (!View)
            {
                View = dbg_new CD3D12RHIRenderTargetView(GetDevice(), RenderTargetOfflineHeap);
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
            LOG_INFO("[CD3D12Viewport]: GetBuffer(" + ToString(i) + ") Failed");
            return false;
        }

        BackBuffers[i] = dbg_new CD3D12RHITexture2D(GetDevice(), GetColorFormat(), Width, Height, 1, 1, 1, TextureFlag_RTV, SClearValue());
        BackBuffers[i]->SetResource(dbg_new CD3D12Resource(GetDevice(), BackBufferResource));

        D3D12_RENDER_TARGET_VIEW_DESC Desc;
        Memory::Memzero(&Desc);

        Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        Desc.Format = BackBuffers[i]->GetNativeFormat();
        Desc.Texture2D.MipSlice = 0;
        Desc.Texture2D.PlaneSlice = 0;

        if (!BackBufferViews[i]->CreateView(BackBuffers[i]->GetResource(), Desc))
        {
            return false;
        }

        BackBufferViews[i].AddRef();
        BackBuffers[i]->SetRenderTargetView(BackBufferViews[i].Get());
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    return true;
}
