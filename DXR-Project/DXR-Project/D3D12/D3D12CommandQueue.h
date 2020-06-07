#pragma once
#include <d3d12.h>

#include <wrl/client.h>

class D3D12GraphicsDevice;

class D3D12CommandQueue
{
	D3D12CommandQueue(D3D12CommandQueue&& Other)		= delete;
	D3D12CommandQueue(const D3D12CommandQueue& Other)	= delete;

	D3D12CommandQueue& operator=(D3D12CommandQueue&& Other)			= delete;
	D3D12CommandQueue& operator=(const D3D12CommandQueue& Other)	= delete;

public:
	D3D12CommandQueue(D3D12GraphicsDevice* Device);
	~D3D12CommandQueue();

	bool Init();

	ID3D12CommandQueue* GetQueue() const
	{
		return Queue.Get();
	}

private:
	D3D12GraphicsDevice*						Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	Queue;
};

