#pragma once
#include "D3D12RefCountedObject.h"

class D3D12RootSignature;

/*
* D3D12DefaultRootSignatures
*/

struct D3D12DefaultRootSignatures
{
	TSharedPtr<D3D12RootSignature> Graphics;
	TSharedPtr<D3D12RootSignature> Compute;
	TSharedPtr<D3D12RootSignature> GlobalRayTracing;
	TSharedPtr<D3D12RootSignature> LocalRayTracing;

	bool Init(class D3D12Device* Device);
};

/*
* D3D12RootSignature
*/

class D3D12RootSignature : public D3D12RefCountedObject
{
public:
	inline D3D12RootSignature(D3D12Device* InDevice, ID3D12RootSignature* InRootSignature)
		: D3D12RefCountedObject(InDevice)
		, RootSignature(InRootSignature)
	{
		VALIDATE(RootSignature != nullptr);
	}

	FORCEINLINE void SetName(const std::string& Name)
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