#include "D3D12Viewport.h"
#include "D3D12CommandQueue.h"

/*
* D3D12Viewport
*/

D3D12Viewport::D3D12Viewport(D3D12Device* InDevice, D3D12CommandContext* InCmdContext, HWND InHwnd, UInt32 InWidth, UInt32 InHeight, EFormat InPixelFormat)
	: D3D12DeviceChild(InDevice)
	, Viewport(InWidth, InHeight, InPixelFormat)
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
	IDXGIFactory2* Factory		= Device->GetFactory();
	D3D12CommandQueue& Queue	= CmdContext->GetQueue();

	// Save the flags
	Flags = Device->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	const UInt32 NumSwapChainBuffers	= 4;
	const DXGI_FORMAT NativeFormat		= ConvertFormat(PixelFormat);

	VALIDATE(Width > 0 && Height > 0);

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	Memory::Memzero(&SwapChainDesc);

	SwapChainDesc.Width					= Width;
	SwapChainDesc.Height				= Height;
	SwapChainDesc.Format				= NativeFormat;
	SwapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount			= NumSwapChainBuffers;
	SwapChainDesc.SampleDesc.Count		= 1;
	SwapChainDesc.SampleDesc.Quality	= 0;
	SwapChainDesc.Scaling				= DXGI_SCALING_STRETCH;
	SwapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	SwapChainDesc.AlphaMode				= DXGI_ALPHA_MODE_IGNORE;
	SwapChainDesc.Flags					= Flags;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullscreenDesc;
	Memory::Memzero(&FullscreenDesc);

	FullscreenDesc.RefreshRate.Numerator	= 0;
	FullscreenDesc.RefreshRate.Denominator	= 1;
	FullscreenDesc.Scaling					= DXGI_MODE_SCALING_UNSPECIFIED;
	FullscreenDesc.ScanlineOrdering			= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	FullscreenDesc.Windowed					= true;

	TComPtr<IDXGISwapChain1> TempSwapChain;
	HRESULT Result = Factory->CreateSwapChainForHwnd(
		Queue.GetQueue(),
		Hwnd, 
		&SwapChainDesc, 
		&FullscreenDesc,
		nullptr, 
		&TempSwapChain);
	if (SUCCEEDED(Result))
	{
		Result = TempSwapChain.As<IDXGISwapChain3>(&SwapChain);
		if (FAILED(Result))
		{
			LOG_ERROR("[D3D12Viewport]: FAILED to retrive IDXGISwapChain3");
			return false;
		}

		NumBackBuffers = NumSwapChainBuffers;
	}
	else
	{
		LOG_ERROR("[D3D12Viewport]: FAILED to create SwapChain");
		return false;
	}

	Factory->MakeWindowAssociation(Hwnd, DXGI_MWA_NO_ALT_ENTER);

	if (!RetriveBackBuffers())
	{
		return false;
	}

	LOG_INFO("[D3D12Viewport]: Created SwapChain");
	return true;
}

Bool D3D12Viewport::Resize(UInt32 InWidth, UInt32 InHeight)
{
	// TODO: Make sure that we can release the old surfaces

	if ((InWidth != Width || InHeight != Height) && InWidth > 0 && InHeight > 0)
	{
		BackBuffers.Clear();
		BackBufferViews.Clear();

		HRESULT Result = SwapChain->ResizeBuffers(0, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, Flags);
		if (SUCCEEDED(Result))
		{
			Width	= InWidth;
			Height	= InHeight;
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
		
		return true;
	}
	else
	{
		return false;
	}
}

Bool D3D12Viewport::Present(Bool VerticalSync)
{
	UInt32 SyncInterval = UInt32(VerticalSync);
	HRESULT Result = SwapChain->Present(SyncInterval, 0);
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

void D3D12Viewport::SetName(const std::string& Name)
{
	SwapChain->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(Name.size()), Name.data());
	
	UInt32 Index = 0;
	for (TSharedRef<D3D12Texture2D>& Buffer : BackBuffers)
	{
		Buffer->SetName(Name + "Buffer [" + std::to_string(Index) + "]");
		Index++;
	}
}

Bool D3D12Viewport::RetriveBackBuffers()
{
	BackBuffers.Clear();
	BackBufferViews.Clear();

	for (UInt32 i = 0; i < NumBackBuffers; i++)
	{
		TComPtr<ID3D12Resource> BackBufferResource;
		HRESULT Result = SwapChain->GetBuffer(i, IID_PPV_ARGS(&BackBufferResource));
		if (FAILED(Result))
		{
			LOG_INFO("[D3D12Viewport]: GetBuffer(" + std::to_string(i) + ") Failed");
			return false;
		}

		D3D12Texture2D* BackBuffer = BackBuffers.EmplaceBack(DBG_NEW D3D12BackBufferTexture2D(Device, BackBufferResource)).Get();

		D3D12_RENDER_TARGET_VIEW_DESC Desc;
		Memory::Memzero(&Desc);

		Desc.ViewDimension	= D3D12_RTV_DIMENSION_TEXTURE2D;
		Desc.Format			= BackBuffer->GetNativeFormat();
		Desc.Texture2D.MipSlice		= 0;
		Desc.Texture2D.PlaneSlice	= 0;

		BackBufferViews.EmplaceBack(DBG_NEW D3D12RenderTargetView(Device, BackBuffer, Desc));
	}

	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	return true;
}
