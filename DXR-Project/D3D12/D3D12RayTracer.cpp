#include "D3D12RayTracer.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"

D3D12RayTracer::D3D12RayTracer(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12RayTracer::~D3D12RayTracer()
{
}

bool D3D12RayTracer::Init(D3D12CommandList* CommandList)
{
	using namespace Microsoft::WRL;

	// Get DXR Interfaces 
	ComPtr<ID3D12Device> MainDevice(Device->GetDevice());
	if (FAILED(MainDevice.As<ID3D12Device5>(&DXRDevice)))
	{
		::OutputDebugString("[D3D12RayTracer]: Failed to retrive DXR-Device");
		return false;
	}

	ComPtr<ID3D12CommandList> MainCommandList(CommandList->GetCommandList());
	if (FAILED(MainCommandList.As<ID3D12GraphicsCommandList4>(&DXRCommandList)))
	{
		::OutputDebugString("[D3D12RayTracer]: Failed to retrive DXR-CommandList");
		return false;
	}

	// Create state object
	std::vector<D3D12_STATE_SUBOBJECT> SubObjects;

	D3D12_STATE_SUBOBJECT Object = { };
	Object.Type = D3D12_STATE_SUB

	D3D12_STATE_OBJECT_DESC RayTracingPipeline = { };
	RayTracingPipeline.Type				= D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	RayTracingPipeline.NumSubobjects	= static_cast<Uint32>(SubObjects.size());
	RayTracingPipeline.pSubobjects		= SubObjects.data();




	return true;
}
