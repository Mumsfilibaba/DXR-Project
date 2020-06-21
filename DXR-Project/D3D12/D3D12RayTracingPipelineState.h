#pragma once
#include "D3D12Buffer.h"
#include "Defines.h"

#include <wrl/client.h>

struct ShaderBindingTableData
{
public:
	~ShaderBindingTableData()
	{
		SAFEDELETE(Resource);
	}

public:
	Uint64			SizeInBytes		= 0;
	Uint32			StrideInBytes	= 0;
	D3D12Buffer*	Resource		= nullptr;
};

class D3D12RayTracingPipelineState : public D3D12DeviceChild
{
public:
	D3D12RayTracingPipelineState(D3D12Device* InDevice);
	~D3D12RayTracingPipelineState();

	bool Initialize();

	ID3D12StateObject* GetStateObject() const
	{
		return DXRStateObject.Get();
	}

	ID3D12RootSignature* GetGlobalRootSignature() const
	{
		return GlobalRootSignature.Get();
	}

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE				GetRayGenerationShaderRecord()	const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	GetMissShaderTable()			const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	GetHitGroupTable()				const;

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& InName) override;

private:
	bool CreatePipeline();
	bool CreateBindingTable();

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	GlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12StateObject>	DXRStateObject;

	ShaderBindingTableData RayGenTable;
	ShaderBindingTableData MissTable;
	ShaderBindingTableData HitTable;
};