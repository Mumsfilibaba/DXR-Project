#pragma once
#include "D3D12DeviceChild.h"

#include <dxcapi.h>

struct ComputePipelineStateProperties
{
	std::string DebugName;
	class D3D12RootSignature* RootSignature = nullptr;
	IDxcBlob* CSBlob = nullptr;
};

class D3D12ComputePipelineState : public D3D12DeviceChild
{
public:
	D3D12ComputePipelineState(D3D12Device* InDevice);
	~D3D12ComputePipelineState();

	bool Initialize(const ComputePipelineStateProperties& Properties);

	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;

	FORCEINLINE ID3D12PipelineState* GetPipeline() const
	{
		return PipelineState.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};