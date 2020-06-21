#pragma once
#include "D3D12DeviceChild.h"

class D3D12GraphicsPipelineState
{
public:
	D3D12GraphicsPipelineState(D3D12Device* InDevice);
	~D3D12GraphicsPipelineState();

	bool Initialize();

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> Pipeline;
};