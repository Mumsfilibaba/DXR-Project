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

	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE				GetRayGenerationShaderRecord()	const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	GetMissShaderTable()			const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	GetHitGroupTable()				const;

	FORCEINLINE ID3D12StateObject* GetStateObject() const
	{
		return DXRStateObject.Get();
	}

	FORCEINLINE ID3D12RootSignature* GetGlobalRootSignature() const
	{
		return GlobalRootSignature.Get();
	}

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