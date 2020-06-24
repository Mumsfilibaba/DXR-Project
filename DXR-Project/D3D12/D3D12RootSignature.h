#pragma once
#include "D3D12DeviceChild.h"

class D3D12RootSignature : public D3D12DeviceChild
{
public:
	D3D12RootSignature(D3D12Device* InDevice);
	~D3D12RootSignature();

	bool Initialize();

	FORCEINLINE ID3D12RootSignature* GetRootSignature() const
	{
		return RootSignature.Get();
	}

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& InName) override;

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
};