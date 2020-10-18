#include "D3D12Factory.h"



D3D12Factory* D3D12Factory::Create()
{


	// Create factory
	IDXGIFactory2* Factory = nullptr;

	if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory))))
	{
		LOG_ERROR("[D3D12GraphicsDevice]: FAILED to create factory");
		return nullptr;
	}
	else
	{
		// Retrive newer factory interface
		ComPtr<IDXGIFactory5> Factory5;
		if (FAILED(Factory.As(&Factory5)))
		{
			LOG_ERROR("[D3D12GraphicsDevice]: FAILED to retrive IDXGIFactory5");
			return false;
		}
		else
		{
			HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing));
			if (SUCCEEDED(hResult))
			{
				if (AllowTearing)
				{
					LOG_INFO("[D3D12GraphicsDevice]: Tearing is supported");
				}
				else
				{
					LOG_INFO("[D3D12GraphicsDevice]: Tearing is NOT supported");
				}
			}
		}
	}

    return nullptr;
}
