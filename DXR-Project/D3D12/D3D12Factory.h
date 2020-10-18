#pragma once
#include <dxgi1_6.h>

#include <wrl/client.h>

/*
* D3D12Factory
*/

class D3D12Factory
{
public:
	inline D3D12Factory(IDXGIFactory2* InFactory)
		: Factory(InFactory)
		, AllowTearing(false)
	{
		VALIDATE(Factory != nullptr);
	}

	~D3D12Factory() = default;

	FORCEINLINE IDXGIFactory2* GetFactory() const
	{
		return Factory.Get();
	}

	static D3D12Factory* Create();

private:
	Microsoft::WRL::ComPtr<IDXGIFactory2> Factory;
	bool AllowTearing;
};