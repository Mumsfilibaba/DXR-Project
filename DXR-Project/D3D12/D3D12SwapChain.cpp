#include "D3D12SwapChain.h"
#include "D3D12Device.h"
#include "D3D12CommandQueue.h"

#include "../Windows/WindowsWindow.h"

D3D12SwapChain::D3D12SwapChain(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12SwapChain::~D3D12SwapChain()
{
}

bool D3D12SwapChain::Init(WindowsWindow* Window, D3D12CommandQueue* Queue)
{
	using namespace Microsoft::WRL;

	// Create a descriptor for the swap chain.
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width					= 0;
	SwapChainDesc.Height				= 0;
	SwapChainDesc.Format				= GetSurfaceFormat();
	SwapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount			= GetSurfaceCount();
	SwapChainDesc.SampleDesc.Count		= 1;
	SwapChainDesc.SampleDesc.Quality	= 0;
	SwapChainDesc.Scaling				= DXGI_SCALING_STRETCH;
	SwapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.AlphaMode				= DXGI_ALPHA_MODE_IGNORE;
	SwapChainDesc.Flags					= Device->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> TempSwapChain;
	HRESULT hResult = Device->GetFactory()->CreateSwapChainForHwnd(Queue->GetQueue(), Window->GetHandle(), &SwapChainDesc, nullptr, nullptr, &TempSwapChain);
	if (SUCCEEDED(hResult))
	{
		hResult = TempSwapChain.As<IDXGISwapChain3>(&SwapChain);
		if (SUCCEEDED(hResult))
		{
			// Disable Alt+Enter for this SwapChain/Window
			Device->GetFactory()->MakeWindowAssociation(Window->GetHandle(), DXGI_MWA_NO_ALT_ENTER);

			RetriveSwapChainSurfaces();

			::OutputDebugString("[D3D12SwapChain]: Created SwapChain\n");
			return true;
		}
		else
		{
			::OutputDebugString("[D3D12SwapChain]: Failed to retrive IDXGISwapChain3\n");
			return false;
		}
	}
	else
	{
		::OutputDebugString("[D3D12SwapChain]: Failed to create SwapChain\n");
		return false;
	}
}

Uint32 D3D12SwapChain::GetCurrentBackBufferIndex() const
{
	return SwapChain->GetCurrentBackBufferIndex();
}

bool D3D12SwapChain::Present(Uint32 SyncInterval)
{
	return SUCCEEDED(SwapChain->Present(SyncInterval, 0));
}

void D3D12SwapChain::RetriveSwapChainSurfaces()
{
	using namespace Microsoft::WRL;

	BackBuffers.resize(GetSurfaceCount());

	Uint32 BufferID = 0;
	for (ComPtr<ID3D12Resource>& Buffer : BackBuffers)
	{
		HRESULT hResult = SwapChain->GetBuffer(BufferID, IID_PPV_ARGS(&Buffer));
		if (SUCCEEDED(hResult))
		{
			BufferID++;
		}
		else
		{
			::OutputDebugString("[D3D12SwapChain]: Failed to retrive SwapChain Buffer\n");
			break;
		}
	}
}
