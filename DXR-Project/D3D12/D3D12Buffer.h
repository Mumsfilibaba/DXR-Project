#pragma once
#include "D3D12Resource.h"
#include "D3D12Views.h"

/*
* BufferProperties
*/

struct BufferProperties
{
	std::string				Name;
	D3D12_RESOURCE_FLAGS	Flags;
	UInt32					SizeInBytes;
	D3D12_RESOURCE_STATES	InitalState;
	EMemoryType				MemoryType;
};

/*
* D3D12Buffer
*/

class D3D12Buffer : public D3D12Resource
{
public:
	D3D12Buffer(D3D12Device* InDevice);
	~D3D12Buffer();

	bool Initialize(const BufferProperties& Properties);

	FORCEINLINE Void* Map()
	{
		Void* HostMemory = nullptr;
		if (SUCCEEDED(Resource->Map(0, nullptr, &HostMemory)))
		{
			return HostMemory;
		}
		else
		{
			return nullptr;
		}
	}

	FORCEINLINE void Unmap()
	{
		Resource->Unmap(0, nullptr);
	}

	FORCEINLINE void SetConstantBufferView(TSharedPtr<D3D12ConstantBufferView> InConstantBufferView)
	{
		ConstantBufferView = InConstantBufferView;
	}

	FORCEINLINE TSharedPtr<D3D12ConstantBufferView> GetConstantBufferView() const
	{
		return ConstantBufferView;
	}

	FORCEINLINE UInt32 GetSizeInBytes() const
	{
		return SizeInBytes;
	}

private:
	TSharedPtr<D3D12ConstantBufferView> ConstantBufferView;

	UInt32 SizeInBytes = 0;
};