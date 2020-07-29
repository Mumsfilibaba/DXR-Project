#pragma once
#include "D3D12DeviceChild.h"

#include <dxcapi.h>

struct GraphicsPipelineStateProperties
{
	std::string					DebugName;
	class D3D12RootSignature*	RootSignature		= nullptr;
	IDxcBlob*					VSBlob				= nullptr;
	IDxcBlob*					PSBlob				= nullptr;
	D3D12_INPUT_ELEMENT_DESC*	InputElements		= nullptr;
	Uint32						NumInputElements	= 0;
	DXGI_FORMAT*				RTFormats			= nullptr;
	Uint32						NumRenderTargets	= 0;
	DXGI_FORMAT					DepthBufferFormat	= DXGI_FORMAT_UNKNOWN;
	bool						EnableDepth			= false;
	bool						EnableBlending		= false;
	D3D12_DEPTH_WRITE_MASK		DepthWriteMask		= D3D12_DEPTH_WRITE_MASK_ALL;
	D3D12_COMPARISON_FUNC		DepthFunc			= D3D12_COMPARISON_FUNC_LESS;
	D3D12_CULL_MODE				CullMode			= D3D12_CULL_MODE_NONE;
};

class D3D12GraphicsPipelineState : public D3D12DeviceChild
{
public:
	D3D12GraphicsPipelineState(D3D12Device* InDevice);
	~D3D12GraphicsPipelineState();

	bool Initialize(const GraphicsPipelineStateProperties& Properties);

	// DeviceChild Interface
	virtual void SetDebugName(const std::string& Name) override;

	FORCEINLINE ID3D12PipelineState* GetPipelineState() const
	{
		return PipelineState.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};