#pragma once
#include "D3D12RefCountedObject.h"

#include "Utilities/StringUtilities.h"

class D3D12RootSignature;

/*
* Default Root Signature defines
*/

#define D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER		0
#define D3D12_DEFAULT_CONSTANT_BUFFER_ROOT_PARAMETER			1
#define D3D12_DEFAULT_SHADER_RESOURCE_VIEW_ROOT_PARAMETER		2
#define D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_ROOT_PARAMETER		3
#define D3D12_DEFAULT_SAMPLER_STATE_ROOT_PARAMETER				4
#define D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_COUNT				32
#define D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT				16
#define D3D12_DEFAULT_ONLINE_RESOURCE_DESCRIPTOR_HEAP_COUNT		2048
#define D3D12_DEFAULT_ONLINE_SAMPLER_DESCRIPTOR_HEAP_COUNT		2048

/*
* D3D12DefaultRootSignatures
*/

struct D3D12DefaultRootSignatures
{
	TSharedPtr<D3D12RootSignature> Graphics;
	TSharedPtr<D3D12RootSignature> Compute;
	TSharedPtr<D3D12RootSignature> GlobalRayTracing;
	TSharedPtr<D3D12RootSignature> LocalRayTracing;

	bool CreateRootSignatures(class D3D12Device* Device);
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