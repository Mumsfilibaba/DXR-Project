#pragma once
#include "RenderingCore/Buffer.h"

#include "D3D12Resource.h"

/*
* D3D12VertexBuffer
*/

class D3D12VertexBuffer : public VertexBuffer, public D3D12Resource
{
	friend class D3D12RenderingAPI;

public:
	D3D12VertexBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, Uint32 VertexStride);
	~D3D12VertexBuffer();

	// Buffer functions
	virtual Uint64 GetSizeInBytes() const
	{
		return 0;
	}

	virtual Uint64 GetVirtualGPUAddress() const
	{
		return 0;
	}

	// Map
	virtual VoidPtr Map()
	{
		return nullptr;
	}

	virtual void Unmap()
	{
	}

	FORCEINLINE const D3D12_VERTEX_BUFFER_VIEW& GetView() const
	{
		return VertexBufferView;
	}

private:
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
};

/*
* D3D12IndexBuffer
*/

class D3D12IndexBuffer : public IndexBuffer, public D3D12Resource
{
	friend class D3D12RenderingAPI;

public:
	D3D12IndexBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, EFormat IndexFormat);
	~D3D12IndexBuffer();

	// Buffer functions
	virtual Uint64 GetSizeInBytes() const
	{
		return 0;
	}

	virtual Uint64 GetVirtualGPUAddress() const
	{
		return 0;
	}

	// Map
	virtual VoidPtr Map()
	{
		return nullptr;
	}

	virtual void Unmap()
	{
	}

	FORCEINLINE const D3D12_INDEX_BUFFER_VIEW& GetView() const
	{
		return IndexBufferView;
	}

private:
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
};

/*
* D3D12ConstantBuffer
*/

class D3D12ConstantBuffer : public ConstantBuffer, public D3D12Resource
{
	friend class D3D12RenderingAPI;

public:
	D3D12ConstantBuffer(D3D12Device* InDevice, Uint32 SizeInBytes);
	~D3D12ConstantBuffer();

	// Buffer functions
	virtual Uint64 GetSizeInBytes() const
	{
		return 0;
	}

	virtual Uint64 GetVirtualGPUAddress() const
	{
		return 0;
	}

	// Map
	virtual VoidPtr Map()
	{
		return nullptr;
	}

	virtual void Unmap()
	{
	}

private:
	class D3D12ConstantBufferView* View;
};

/*
* D3D12StructuredBuffer
*/

class D3D12StructuredBuffer : public StructuredBuffer, public D3D12Resource
{
	friend class D3D12RenderingAPI;

public:
	D3D12StructuredBuffer(D3D12Device* InDevice, Uint32 ElementCount, Uint32 StructuredStride);
	~D3D12StructuredBuffer();

	// Buffer functions
	virtual Uint64 GetSizeInBytes() const
	{
		return 0;
	}

	virtual Uint64 GetVirtualGPUAddress() const
	{
		return 0;
	}

	// Map
	virtual VoidPtr Map()
	{
		return nullptr;
	}

	virtual void Unmap()
	{
	}

private:
	class D3D12ShaderResourceView* View;
};

/*
* D3D12ByteAddressBuffer
*/

class D3D12ByteAddressBuffer : public ByteAddressBuffer, public D3D12Resource
{
	friend class D3D12RenderingAPI;

public:
	D3D12ByteAddressBuffer(D3D12Device* InDevice, Uint32 SizeInBytes);
	~D3D12ByteAddressBuffer();

	// Buffer functions
	virtual Uint64 GetSizeInBytes() const
	{
		return 0;
	}

	virtual Uint64 GetVirtualGPUAddress() const
	{
		return 0;
	}

	// Map
	virtual VoidPtr Map()
	{
		return nullptr;
	}

	virtual void Unmap()
	{
	}

private:
	class D3D12ShaderResourceView* View;
};