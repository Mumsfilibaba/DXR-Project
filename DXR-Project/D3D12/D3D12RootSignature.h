#pragma once
#include "D3D12DeviceChild.h"

#include <dxcapi.h>

class D3D12RootSignature : public D3D12DeviceChild
{
public:
	D3D12RootSignature(D3D12Device* InDevice);
	~D3D12RootSignature();

	bool Initialize();
	bool Initialize(IDxcBlob* ShaderBlob);

	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;

	FORCEINLINE ID3D12RootSignature* GetRootSignature() const
	{
		return RootSignature.Get();
	}

private:
	bool Initialize(const void* RootSignatureBlob, Uint32 BlobSizeInBytes);

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
};