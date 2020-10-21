#pragma once
#include "D3D12Buffer.h"
#include "Defines.h"

#include <wrl/client.h>

#include <dxcapi.h>

class D3D12DescriptorTable;
class D3D12RootSignature;

/*
* RayTracingPipelineStateProperties
*/

struct RayTracingPipelineStateProperties
{
	std::string DebugName;

	D3D12RootSignature* RayGenRootSignature		= nullptr;
	D3D12RootSignature* HitGroupRootSignature	= nullptr;
	D3D12RootSignature* MissRootSignature		= nullptr;
	D3D12RootSignature* GlobalRootSignature		= nullptr;

	Uint32 MaxRecursions = 0;
};

/*
* D3D12RayTracingPipelineState
*/

class D3D12RayTracingPipelineState : public D3D12DeviceChild
{
public:
	D3D12RayTracingPipelineState(D3D12Device* InDevice);
	~D3D12RayTracingPipelineState();

	bool Initialize(const RayTracingPipelineStateProperties& Properties);

	FORCEINLINE void SetName(const std::string& Name)
	{
		std::wstring WideName = ConvertToWide(Name);
		StateObject->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12StateObject* GetStateObject() const
	{
		return StateObject.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject> StateObject;
};