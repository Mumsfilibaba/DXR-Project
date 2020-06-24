#pragma once
#include "D3D12Buffer.h"

#include <memory>

class D3D12UploadStack
{
public:
	D3D12UploadStack();
	~D3D12UploadStack();

	bool Initialize(class D3D12Device* Device);

	void Reset();
	void Close();

	void* Allocate(Uint64 SizeInBytes);

	FORCEINLINE D3D12Buffer* GetBuffer() const
	{
		return Buffer.get();
	}

	FORCEINLINE Uint64 GetOffset() const
	{
		return Offset;
	}

private:
	Byte*							CPUMemory = nullptr;
	std::shared_ptr<D3D12Buffer>	Buffer;
	
	Uint64 Offset		= 0;
	Uint64 BufferSize	= 0;
};