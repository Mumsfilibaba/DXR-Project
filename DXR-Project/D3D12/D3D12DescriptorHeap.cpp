#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
}

bool D3D12DescriptorHeap::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE Type, Uint32 DescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
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

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart() const
{
	return Heap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUDescriptorHandleAt(Uint32 DescriptorIndex) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE Handle = GetGPUDescriptorHandleForHeapStart();
	Handle.ptr += GetDescriptorSize() * DescriptorIndex;
	return Handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart() const
{
	return Heap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUDescriptorHandleAt(Uint32 DescriptorIndex) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += GetDescriptorSize() * DescriptorIndex;
	return Handle;
}

void D3D12DescriptorHeap::SetName(const std::string& InName)
{
	Heap->SetName(ConvertToWide(InName).c_str());
}
