#include "Core/Misc/FrameProfiler.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12SwapChain.h"

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

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
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
        }

        SwapChain->SetMaximumFrameLatency(NumSwapChainBuffers);
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
    if ((InWidth != Desc.Width || InHeight != Desc.Height) && InWidth > 0u && InHeight > 0u)
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

        HRESULT Result = SwapChain->ResizeBuffers(0, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, Flags);
        if (SUCCEEDED(Result))
        {
            Desc.Width  = uint16(InWidth);
            Desc.Height = uint16(InHeight);
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

        D3D12_INFO("[FD3D12SwapChainRHI]: Resized %u x %u", Desc.Width, Desc.Height);
    }

    // NOTE: Not considered an error to try to resize when the size is the same, maybe it should?
    return true;
}

bool FD3D12SwapChainRHI::Present(bool bVerticalSync)
{
    TRACE_FUNCTION_SCOPE();

    const uint32 SyncInterval = bVerticalSync ? 1 : 0;

    uint32 PresentFlags = 0;
    if (SyncInterval == 0 && Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
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
            const DWORD WaitResult = WaitForSingleObjectEx(SwapChainWaitableObject, INFINITE, TRUE);
            if (WaitResult != WAIT_OBJECT_0)
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

bool FD3D12SwapChainRHI::RetrieveBackBuffers()
{
    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(GetColorFormat(), GetWidth(), GetHeight(), 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable);
    if (BackBuffers.Size() < static_cast<int32>(NumBackBuffers))
    {
        BackBuffers.Resize(NumBackBuffers);
        for (FD3D12TextureRHIRef& Texture : BackBuffers)
        {
            Texture = new FD3D12TextureRHI(GetDevice(), BackBufferDesc);
            if (BackBufferDesc.bEnableResourceStateTracking)
            {
                Texture->EnableStateTracking(EResourceAccess::Present);
            }
        }
    }

    if (BackBufferProxy)
    {
        BackBufferProxy->Resize(GetWidth(), GetHeight());
    }
    else
    {
        BackBufferProxy = new FD3D12BackBufferTexture(GetDevice(), this, BackBufferDesc);
        if (BackBufferDesc.bEnableResourceStateTracking)
        {
            BackBufferProxy->EnableStateTracking(EResourceAccess::Present);
        }
    }

    for (uint32 Index = 0; Index < NumBackBuffers; ++Index)
    {
        TComPtr<ID3D12Resource> BackBufferResource;
        HRESULT Result = SwapChain->GetBuffer(Index, IID_PPV_ARGS(&BackBufferResource));
        if (FAILED(Result))
        {
            D3D12_INFO("[FD3D12SwapChainRHI]: GetBuffer(%u) Failed", Index);
            return false;
        }

        BackBuffers[Index]->SetResource(new FD3D12Resource(GetDevice(), BackBufferResource));
        BackBuffers[Index]->GetResource()->SetDebugName(FString::CreateFormatted("BackBuffer[%u]", Index));

        if (FD3D12ResourceState* ResourceState = BackBuffers[Index]->GetResourceState())
        {
            ResourceState->SetState(D3D12_RESOURCE_STATE_PRESENT);
        }
    }

    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

    if (FD3D12TextureRHI* CurrentBackbuffer = BackBufferProxy->GetCurrentBackBufferTexture())
    {
        if (FD3D12ResourceState* ResourceState = BackBufferProxy->GetResourceState())
        {
            ResourceState->SetState(D3D12_RESOURCE_STATE_PRESENT);
        }

        return true;
    }
    else
    {
        return false;
    }
}
