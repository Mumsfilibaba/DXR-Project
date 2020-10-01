#pragma once
#include "D3D12DeviceChild.h"

#include <dxcapi.h>

class D3D12RootSignature;

/*
* ComputePipelineStateProperties
*/

struct ComputePipelineStateProperties
{
	ComputePipelineStateProperties() = default;

	inline ComputePipelineStateProperties(const std::string& InDebugName, D3D12RootSignature* InRootSignature, IDxcBlob* InCSBlob)
		: DebugName(InDebugName)
		, RootSignature(InRootSignature)
		, CSBlob(InCSBlob)
	{
	}

	std::string			DebugName;
	D3D12RootSignature* RootSignature = nullptr;
	IDxcBlob* CSBlob = nullptr;
};

/*
* D3D12ComputePipelineState 
*/

class D3D12ComputePipelineState : public D3D12DeviceChild
{
public:
	D3D12ComputePipelineState(D3D12Device* InDevice);
	~D3D12ComputePipelineState();

	bool Initialize(const ComputePipelineStateProperties& Properties);

	// DeviceChild Interface
	virtual void SetDebugName(const std::string& Name) override;

	FORCEINLINE ID3D12PipelineState* GetPipeline() const
	{
		return PipelineState.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};