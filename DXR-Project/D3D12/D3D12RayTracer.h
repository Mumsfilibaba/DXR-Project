#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"

#include "Types.h"
#include "Defines.h"

#include "Application/GenericEventHandler.h"

#include "Rendering/Camera.h"

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

class D3D12CommandList;

class D3D12RayTracer : public D3D12DeviceChild, public GenericEventHandler
{
public:
	D3D12RayTracer(D3D12Device* Device);
	~D3D12RayTracer();

	bool Init(D3D12CommandList* CommandList, class D3D12CommandQueue* CommandQueue);

	void Render(ID3D12Resource* BackBuffer, D3D12CommandList* CommandList);

	virtual void OnKeyDown(Uint32 KeyCode)		override;
	virtual void OnMouseMove(Int32 x, Int32 y)	override;

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject>			DXRStateObject;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>			GlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> DXRStateProperties;

	class D3D12DescriptorHeap*	ResourceHeap	= nullptr;
	class D3D12Texture*			ResultTexture	= nullptr;

	D3D12Buffer*				CameraBuffer	= nullptr;
	D3D12Buffer*				VertexBuffer	= nullptr;
	D3D12Buffer*				IndexBuffer		= nullptr;
	D3D12AccelerationStructure* TopLevelAS		= nullptr;
	D3D12AccelerationStructure* BottomLevelAS	= nullptr;

	Camera SceneCamera;

	ShaderTableData RayGenTable;
	ShaderTableData MissTable;
	ShaderTableData HitTable;

	D3D12_GPU_DESCRIPTOR_HANDLE CameraBufferGPUHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE CameraBufferCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE OutputCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE AccelerationStructureCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE VertexBufferCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE IndexBufferCPUHandle;

	bool IsCameraAcive = false;
};