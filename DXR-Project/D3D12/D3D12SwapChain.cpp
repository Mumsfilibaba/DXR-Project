#include "D3D12SwapChain.h"
#include "D3D12Device.h"
#include "D3D12CommandQueue.h"

#include "Windows/WindowsWindow.h"

/*
* D3D12SwapChain
*/

D3D12SwapChain::D3D12SwapChain(D3D12Device* InDevice)
	: D3D12RefCountedObject(InDevice)
	, Window(nullptr)
	, SwapChain()
	, Width(0)
	, Height(0)
	, Flags(0)
{
}

D3D12SwapChain::~D3D12SwapChain()
{
}

bool D3D12SwapChain::CreateSwapChain(IDXGIFactory2* Factory, const TSharedRef<WindowsWindow>& InWindow, D3D12CommandQueue* Queue)
{
	using namespace Microsoft::WRL;

	// Save the flags
	Flags = Device->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	WindowShape Shape;
	InWindow->GetWindowShape(Shape);

	Width	= Shape.Width;
	Height	= Shape.Height;

	// Create a descriptor for the swap chain.
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width					= Width;
	SwapChainDesc.Height				= Height;
	SwapChainDesc.Format				= GetSurfaceFormat();
	SwapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount			= GetSurfaceCount();
	SwapChainDesc.SampleDesc.Count		= 1;
	SwapChainDesc.SampleDesc.Quality	= 0;
	SwapChainDesc.Scaling				= DXGI_SCALING_STRETCH;
	SwapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.AlphaMode				= DXGI_ALPHA_MODE_IGNORE;
	SwapChainDesc.Flags					= Flags;

	ComPtr<IDXGISwapChain1> TempSwapChain;
	HRESULT hResult = Factory->CreateSwapChainForHwnd(Queue->GetQueue(), InWindow->GetHandle(), &SwapChainDesc, nullptr, nullptr, &TempSwapChain);
	if (SUCCEEDED(hResult))
	{
		hResult = TempSwapChain.As<IDXGISwapChain3>(&SwapChain);
		if (SUCCEEDED(hResult))
		{
			// Disable Alt+Enter for this SwapChain/Window
			Factory->MakeWindowAssociation(InWindow->GetHandle(), DXGI_MWA_NO_ALT_ENTER);

			LOG_INFO("[D3D12SwapChain]: Created SwapChain");
			return true;
		}
		else
		{
			LOG_ERROR("[D3D12SwapChain]: FAILED to retrive IDXGISwapChain3");
			return false;
		}
	}
	else
	{
		LOG_ERROR("[D3D12SwapChain]: FAILED to create SwapChain");
		return false;
	}
}

bool D3D12SwapChain::Resize(UInt32 InWidth, UInt32 InHeight)
{
	if (InWidth == 0 || InHeight == 0)
	{
		LOG_ERROR("[D3D12SwapChain]: Resize FAILED. Width or Height cannot be zero");
		return false;
	}

	if (InWidth == Width && InHeight == Height)
	{
		LOG_ERROR("[D3D12SwapChain]: Resize FAILED. Width or Height is the same");
		return false;
	}

	HRESULT Result = SwapChain->ResizeBuffers(0, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, Flags);
	if (SUCCEEDED(Result))
	{
		Width	= InWidth;
		Height	= InHeight;
		return true;
	}
	else
	{
		LOG_WARNING("[D3D12SwapChain]: Resize FAILED");
		return false;
	}
}

UInt32 D3D12SwapChain::GetCurrentBackBufferIndex() const
{
	return SwapChain->GetCurrentBackBufferIndex();
}

bool D3D12SwapChain::Present(UInt32 SyncInterval)
{
	return SUCCEEDED(SwapChain->Present(SyncInterval, 0));
}

//void D3D12SwapChain::RetriveSwapChainSurfaces()
//{
//	using namespace Microsoft::WRL;
//
//	if (BackBuffers.Size() < GetSurfaceCount())
//	{
//		BackBuffers.Resize(GetSurfaceCount());
//	}
//
//	if (BackBuffersViews.Size() < GetSurfaceCount())
//	{
//		BackBuffersViews.Resize(GetSurfaceCount());
//	}
//
//	Uint32 BufferID = 0;
//	ComPtr<ID3D12Resource> Resource;
//	for (TSharedPtr<D3D12Texture2D>& Buffer : BackBuffers)
//	{
//		HRESULT hResult = SwapChain->GetBuffer(BufferID, IID_PPV_ARGS(&Resource));
//		if (SUCCEEDED(hResult))
//		{
//			if (!Buffer)
//			{
//				Buffer = TSharedPtr<D3D12Texture>(new D3D12Texture(Device));
//			}
//
//			if (Buffer->D3D12Resource::Initialize(Resource.Get()))
//			{
//				D3D12_RENDER_TARGET_VIEW_DESC RtvDesc = { };
//				RtvDesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
//				RtvDesc.Format					= GetSurfaceFormat();
//				RtvDesc.Texture2D.MipSlice		= 0;
//				RtvDesc.Texture2D.PlaneSlice	= 0;
//
//				if (!BackBuffersViews[BufferID])
//				{
//					BackBuffersViews[BufferID] = TSharedPtr<D3D12RenderTargetView>(new D3D12RenderTargetView(Device, Resource.Get(), &RtvDesc));
//				}
//				else
//				{
//					BackBuffersViews[BufferID]->CreateView(Resource.Get(), &RtvDesc);
//				}
//
//				Buffer->SetName("BackBuffer[" + std::to_string(BufferID) + "]");
//				//Buffer->SetRenderTargetView(BackBuffersViews[BufferID], 0);
//			}
//
//			BufferID++;
//		}
//		else
//		{
//			LOG_ERROR("[D3D12SwapChain]: FAILED to retrive SwapChain Buffer");
//			break;
//		}
//	}
//}
