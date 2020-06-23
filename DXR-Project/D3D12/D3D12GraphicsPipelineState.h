#pragma once
#include "D3D12DeviceChild.h"

#include <dxcapi.h>

struct GraphicsPipelineStateProperties
{
	std::string					Name;
	class D3D12RootSignature*	RootSignature	= nullptr;
	IDxcBlob*					VSBlob			= nullptr;
	IDxcBlob*					PSBlob			= nullptr;
};

class D3D12GraphicsPipelineState : public D3D12DeviceChild
{
public:
	D3D12GraphicsPipelineState(D3D12Device* InDevice);
	~D3D12GraphicsPipelineState();

	bool Initialize(const GraphicsPipelineStateProperties& InProperties);

	FORCEINLINE ID3D12PipelineState* GetPipelineState() const
	{
		return PipelineState.Get();
	}

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& InName) override;

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};