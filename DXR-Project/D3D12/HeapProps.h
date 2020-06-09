#pragma once
#include <d3d12.h>

struct HeapProps
{
public:
	static D3D12_HEAP_PROPERTIES UploadHeap()
	{
		D3D12_HEAP_PROPERTIES UploadHeapProperties =
		{
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0,
		};

		return UploadHeapProperties;
	}

	static D3D12_HEAP_PROPERTIES DefaultHeap()
	{
		D3D12_HEAP_PROPERTIES UploadHeapProperties =
		{
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0,
		};

		return UploadHeapProperties;
	}
};