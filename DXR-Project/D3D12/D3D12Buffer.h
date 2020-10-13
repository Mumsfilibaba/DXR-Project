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
	D3D12VertexBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, Uint32 VertexStride, Uint32 Usage);
	~D3D12VertexBuffer() = default;

	// Map
	virtual VoidPtr Map() override
	{
		return nullptr;
	}

	virtual void Unmap() override
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
	D3D12IndexBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, EIndexFormat IndexFormat, Uint32 Usage);
	~D3D12IndexBuffer() = default;

	// Map
	virtual VoidPtr Map() override
	{
		return nullptr;
	}

	virtual void Unmap() override
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
	D3D12ConstantBuffer(D3D12Device* InDevice, Uint32 SizeInBytes, Uint32 Usage);
	~D3D12ConstantBuffer() = default;

	// Map
	virtual VoidPtr Map() override
	{
		return nullptr;
	}

	virtual void Unmap() override
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
	D3D12StructuredBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, Uint32 InStride, Uint32 InUsage);
	~D3D12StructuredBuffer() = default;

	// Map
	virtual VoidPtr Map() override
	{
		return nullptr;
	}

	virtual void Unmap() override
	{
	}
};