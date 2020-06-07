#include "D3D12SwapChain.h"

D3D12SwapChain::D3D12SwapChain()
{
}

D3D12SwapChain::~D3D12SwapChain()
{
}

bool D3D12SwapChain::Init(WindowsWindow* Window)
{
	// Create a descriptor for the swap chain.
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width					= 0;
	SwapChainDesc.Height				= 0;
	SwapChainDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount			= 3;
	SwapChainDesc.SampleDesc.Count		= 1;
	SwapChainDesc.SampleDesc.Quality	= 0;
	SwapChainDesc.Scaling				= DXGI_SCALING_STRETCH;
	SwapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.AlphaMode				= DXGI_ALPHA_MODE_IGNORE;
	SwapChainDesc.Flags					= 0;// (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	return false;
}
