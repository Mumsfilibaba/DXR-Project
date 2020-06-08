#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
}

bool D3D12DescriptorHeap::Init(D3D12_DESCRIPTOR_HEAP_TYPE Type, Uint32 DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.Flags				= Flags;
	HeapDesc.NodeMask			= 0;
	HeapDesc.NumDescriptors		= DescriptorCount;
	HeapDesc.Type				= Type;

	HRESULT hResult = Device->GetDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Heap));
	if (SUCCEEDED(hResult))
	{
		DescriptorSize = Device->GetDevice()->GetDescriptorHandleIncrementSize(Type);

		::OutputDebugString("[D3D12DescrtiptorHeap]: Created DescriptorHeap\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12DescrtiptorHeap]: Failed to create DescriptorHeap\n");
		return false;
	}
}
