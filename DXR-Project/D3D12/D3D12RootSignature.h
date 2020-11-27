#pragma once
#include "D3D12DeviceChild.h"

#include <dxcapi.h>

/*
* D3D12RootSignature
*/

class D3D12RootSignature : public D3D12DeviceChild
{
public:
	D3D12RootSignature(D3D12Device* InDevice);
	~D3D12RootSignature();

	bool Initialize(const D3D12_ROOT_SIGNATURE_DESC& Desc);
	bool Initialize(IDxcBlob* ShaderBlob);

	// DeviceChild Interface
	virtual void SetDebugName(const std::string& Name) override;

	FORCEINLINE ID3D12RootSignature* GetRootSignature() const
	{
		return RootSignature.Get();
	}

	FORCEINLINE ID3D12RootSignature* const * GetRootSignatureAddress() const
	{
		return RootSignature.GetAddressOf();
	}

private:
	bool Initialize(const Void* RootSignatureBlob, UInt32 BlobSizeInBytes);

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
};