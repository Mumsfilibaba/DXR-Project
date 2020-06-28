#include "D3D12UploadStack.h"

D3D12UploadStack::D3D12UploadStack()
{
}

D3D12UploadStack::~D3D12UploadStack()
{
}

bool D3D12UploadStack::Initialize(D3D12Device* Device)
{
	BufferSize = 1024 * 1024;

	BufferProperties Properties = { };
	Properties.Flags			= D3D12_RESOURCE_FLAG_NONE;
	Properties.HeapProperties	= HeapProps::UploadHeap();
	Properties.SizeInBytes		= BufferSize;
	Properties.InitalState		= D3D12_RESOURCE_STATE_GENERIC_READ;

	Buffer = std::shared_ptr<D3D12Buffer>(new D3D12Buffer(Device));
	if (!Buffer->Initialize(Properties))
	{
		return false;
	}

	return true;
}

void D3D12UploadStack::Reset()
{
	CPUMemory	= reinterpret_cast<Byte*>(Buffer->Map());
	Offset		= 0;
}

void D3D12UploadStack::Close()
{
	Buffer->Unmap();
	CPUMemory = nullptr;
}

void* D3D12UploadStack::Allocate(Uint64 SizeInBytes)
{
	const Uint64 TempAllocatedBytes = Offset + SizeInBytes;
	if (TempAllocatedBytes > BufferSize)
	{
		return nullptr;
	}
	else
	{
		Offset = TempAllocatedBytes;

		Byte* CurrentPointer = CPUMemory;
		CPUMemory = CPUMemory + SizeInBytes;
		return reinterpret_cast<void*>(CurrentPointer);
	}
}
