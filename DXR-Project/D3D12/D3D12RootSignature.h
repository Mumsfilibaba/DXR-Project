#pragma once
#include "D3D12DeviceChild.h"

#include <dxcapi.h>

/*
* D3D12RootSignature
*/

class D3D12RootSignature : public D3D12DeviceChild
{
public:
	inline D3D12RootSignature(D3D12Device* InDevice, ID3D12RootSignature* InRootSignature)
		: D3D12DeviceChild(InDevice)
		, RootSignature(InRootSignature)
	{
		VALIDATE(RootSignature != nullptr);
	}
	
	~D3D12RootSignature() = default;

	// DeviceChild Interface
	FORCEINLINE void SetDebugName(const std::string& Name)
	{
		std::wstring WideName = ConvertToWide(Name);
		RootSignature->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12RootSignature* GetRootSignature() const
	{
		return RootSignature.Get();
	}

	FORCEINLINE ID3D12RootSignature* const * GetAddressOfRootSignature() const
	{
		return RootSignature.GetAddressOf();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
};