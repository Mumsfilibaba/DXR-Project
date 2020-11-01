#pragma once
#include "RenderingCore/PipelineState.h"

#include "D3D12DeviceChild.h"

#include <dxcapi.h>

class D3D12RootSignature;

/*
* D3D12ComputePipelineState 
*/

class D3D12ComputePipelineState : public ComputePipelineState, public D3D12DeviceChild
{
public:
	D3D12ComputePipelineState(D3D12Device* InDevice);
	~D3D12ComputePipelineState();

	FORCEINLINE void SetName(const std::string& Name)
	{
		std::wstring WideName = ConvertToWide(Name);
		PipelineState->SetName(WideName.c_str());
	}

	FORCEINLINE ID3D12PipelineState* GetPipeline() const
	{
		return PipelineState.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};