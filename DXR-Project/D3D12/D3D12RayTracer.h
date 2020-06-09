#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"

#include "Types.h"
#include "Defines.h"

class D3D12AccelerationStructure;

struct ShaderTableData
{
	Uint64			SizeInBytes		= 0;
	Uint32			StrideInBytes	= 0;
	D3D12Buffer*	Resource		= nullptr;

	~ShaderTableData()
	{
		SAFEDELETE(Resource);
	}
};

class D3D12RayTracer : public D3D12DeviceChild
{
public:
	D3D12RayTracer(D3D12Device* Device);
	~D3D12RayTracer();

	bool Init(class D3D12CommandList* CommandList, class D3D12CommandQueue* CommandQueue);

	void Render(ID3D12Resource* BackBuffer);

private:
	Microsoft::WRL::ComPtr<ID3D12Device5>				DXRDevice;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	DXRCommandList;
	Microsoft::WRL::ComPtr<ID3D12StateObject>			DXRStateObject;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>			GlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> DXRStateProperties;

	class D3D12DescriptorHeap*	ResourceHeap	= nullptr;
	class D3D12Texture*			ResultTexture	= nullptr;

	D3D12Buffer*				VertexBuffer	= nullptr;
	D3D12AccelerationStructure* TopLevelAS		= nullptr;
	D3D12AccelerationStructure* BottomLevelAS	= nullptr;

	ShaderTableData RayGenTable;
	ShaderTableData MissTable;
	ShaderTableData HitTable;

	D3D12_CPU_DESCRIPTOR_HANDLE OutputCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE AccelerationStructureCPUHandle;
};