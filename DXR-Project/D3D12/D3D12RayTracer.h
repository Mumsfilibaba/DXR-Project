#pragma once
#include "D3D12DeviceChild.h"

class D3D12RayTracer : public D3D12DeviceChild
{
	D3D12RayTracer(D3D12RayTracer&& Other)		= delete;
	D3D12RayTracer(const D3D12RayTracer& Other) = delete;

	D3D12RayTracer& operator=(D3D12RayTracer&& Other)		= delete;
	D3D12RayTracer& operator=(const D3D12RayTracer& Other)	= delete;

public:
	D3D12RayTracer(D3D12Device* Device);
	~D3D12RayTracer();

	bool Init(class D3D12CommandList* CommandList);

private:
	Microsoft::WRL::ComPtr<ID3D12Device5>				DXRDevice;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	DXRCommandList;
	Microsoft::WRL::ComPtr<ID3D12StateObject>			DXRStateObject;
};