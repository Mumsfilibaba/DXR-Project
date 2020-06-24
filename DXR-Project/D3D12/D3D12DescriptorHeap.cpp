#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, Heap(nullptr)
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

		// Create Handles
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = Heap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = Heap->GetGPUDescriptorHandleForHeapStart();
		FreeHandles.reserve(DescriptorCount);

		for (Uint32 Index = 0; Index < DescriptorCount; Index++)
		{
			FreeHandles.emplace_back(CPUHandle, GPUHandle);

			CPUHandle.ptr += GetDescriptorSize() * Index;
			GPUHandle.ptr += GetDescriptorSize() * Index;
		}

		return true;
	}
	else
	{
		::OutputDebugString("[D3D12DescrtiptorHeap]: Failed to create DescriptorHeap\n");
		return false;
	}
}

D3D12DescriptorHandle D3D12DescriptorHeap::Allocate()
{
	D3D12DescriptorHandle Handle = FreeHandles.front();
	FreeHandles.erase(FreeHandles.begin());

	return Handle;
}

void D3D12DescriptorHeap::Free(const D3D12DescriptorHandle& DescriptorHandle)
{
	FreeHandles.emplace_back(DescriptorHandle);
}

void D3D12DescriptorHeap::SetName(const std::string& InName)
{
	Heap->SetName(ConvertToWide(InName).c_str());
}
