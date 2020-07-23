#include "D3D12UploadStack.h"

D3D12UploadStack::D3D12UploadStack()
{
}

D3D12UploadStack::~D3D12UploadStack()
{
	SAFEDELETE(Buffer);
}

bool D3D12UploadStack::Initialize(D3D12Device* Device)
{
	BufferSize = 1024 * 1024;

	BufferProperties Properties = { };
	Properties.Flags		= D3D12_RESOURCE_FLAG_NONE;
	Properties.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;
	Properties.SizeInBytes	= BufferSize;
	Properties.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;

	Buffer = new D3D12Buffer(Device);
	if (!Buffer->Initialize(Properties))
	{
		return false;
	}
	else
	{
		Reset();
		return true;
	}
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

void* D3D12UploadStack::Allocate(Uint32 SizeInBytes)
{
	const Uint32 TempAllocatedBytes = Offset + SizeInBytes;
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
