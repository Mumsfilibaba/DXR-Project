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
	inline D3D12VertexBuffer::D3D12VertexBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, Uint32 InStride, Uint32 InUsage)
		: VertexBuffer(InSizeInBytes, InStride, InUsage)
		, D3D12Resource(InDevice)
		, VertexBufferView()
	{
	}
	
	~D3D12VertexBuffer() = default;

	// Map
	virtual VoidPtr Map(const Range& MappedRange) override
	{
		return D3D12Resource::Map(MappedRange);
	}

	virtual void Unmap(const Range& WrittenRange) override
	{
		D3D12Resource::Unmap(WrittenRange);
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
	inline D3D12IndexBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, EIndexFormat InIndexFormat, Uint32 InUsage)
		: IndexBuffer(InSizeInBytes, InIndexFormat, InUsage)
		, D3D12Resource(InDevice)
		, IndexBufferView()
	{
	}

	~D3D12IndexBuffer() = default;

	// Map
	virtual VoidPtr Map(const Range& MappedRange) override
	{
		return D3D12Resource::Map(MappedRange);;
	}

	virtual void Unmap(const Range& WrittenRange) override
	{
		D3D12Resource::Unmap(WrittenRange);
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
	inline D3D12ConstantBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, Uint32 InUsage)
		: ConstantBuffer(InSizeInBytes, InUsage)
		, D3D12Resource(InDevice)
		, View(nullptr)
	{
	}

	~D3D12ConstantBuffer() = default;

	// Map
	virtual VoidPtr Map(const Range& MappedRange) override
	{
		return D3D12Resource::Map(MappedRange);;
	}

	virtual void Unmap(const Range& WrittenRange) override
	{
		D3D12Resource::Unmap(WrittenRange);
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
	inline D3D12StructuredBuffer(D3D12Device* InDevice, Uint32 InSizeInBytes, Uint32 InStride, Uint32 InUsage)
		: StructuredBuffer(InSizeInBytes, InStride, InUsage)
		, D3D12Resource(InDevice)
	{
	}

	~D3D12StructuredBuffer() = default;

	// Map
	virtual VoidPtr Map(const Range& MappedRange) override
	{
		return D3D12Resource::Map(MappedRange);;
	}

	virtual void Unmap(const Range& WrittenRange) override
	{
		D3D12Resource::Unmap(WrittenRange);
	}
};