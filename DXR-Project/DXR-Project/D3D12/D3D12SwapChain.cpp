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
	// Create a descriptor for the swap chain.
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width					= 0;
	SwapChainDesc.Height				= 0;
	SwapChainDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount			= BackBufferCount;
	SwapChainDesc.SampleDesc.Count		= 1;
	SwapChainDesc.SampleDesc.Quality	= 0;
	SwapChainDesc.Scaling				= DXGI_SCALING_STRETCH;
	SwapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.AlphaMode				= DXGI_ALPHA_MODE_IGNORE;
	SwapChainDesc.Flags					= Device->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	HRESULT hResult = Device->GetFactory()->CreateSwapChainForHwnd(Queue->GetQueue(), Window->GetHandle(), &SwapChainDesc, nullptr, nullptr, &SwapChain);
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
		::OutputDebugString("[D3D12SwapChain]: Failed to create SwapChain\n");
		return false;
	}
}

void D3D12SwapChain::RetriveSwapChainSurfaces()
{
	using namespace Microsoft::WRL;

	BackBuffers.resize(BackBufferCount);

	Uint32 BufferID = 0;
	for (ComPtr<ID3D12Resource> Buffer : BackBuffers)
	{
		HRESULT hResult = SwapChain->GetBuffer(BufferID, IID_PPV_ARGS(&Buffer));
		if (SUCCEEDED(hResult))
		{
			BufferID++;
		}
		else
		{
			break;
		}
	}
}
