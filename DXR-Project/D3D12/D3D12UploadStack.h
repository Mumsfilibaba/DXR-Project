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

	void* Allocate(Uint32 SizeInBytes);

	FORCEINLINE D3D12Buffer* GetBuffer() const
	{
		return Buffer;
	}

	FORCEINLINE Uint32 GetOffset() const
	{
		return Offset;
	}

private:
	Byte*			CPUMemory	= nullptr;
	D3D12Buffer*	Buffer		= nullptr;
	
	Uint32 Offset		= 0;
	Uint32 BufferSize	= 0;
};