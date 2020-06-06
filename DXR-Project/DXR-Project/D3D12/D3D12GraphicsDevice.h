#pragma once
#include <d3d12.h>

class D3D12GraphicsDevice
{
	D3D12GraphicsDevice(D3D12GraphicsDevice&& Other) = delete;
	D3D12GraphicsDevice(const D3D12GraphicsDevice& Other) = delete;

	D3D12GraphicsDevice& operator=(D3D12GraphicsDevice&& Other) = delete;
	D3D12GraphicsDevice& operator=(const D3D12GraphicsDevice& Other) = delete;

public:
	D3D12GraphicsDevice();
	~D3D12GraphicsDevice();

	bool Init();

	static D3D12GraphicsDevice* Create();
	static D3D12GraphicsDevice* Get();

private:

};