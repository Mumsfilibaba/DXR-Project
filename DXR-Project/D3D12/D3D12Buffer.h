#pragma once
#include "RenderingCore/Buffer.h"

#include "D3D12Resource.h"

/*
* D3D12Buffer
*/

class D3D12Buffer : public D3D12Resource
{
public:
	inline D3D12Buffer::D3D12Buffer(D3D12Device* InDevice)
		: D3D12Resource(InDevice)
	{
	}

	~D3D12Buffer() = default;

	FORCEINLINE UInt64 GetAllocatedSize() const
	{
		return Desc.Width;
	}
};

/*
* D3D12VertexBuffer
*/

class D3D12VertexBuffer : public VertexBuffer, public D3D12Buffer
{
	friend class D3D12RenderingAPI;

public:
	inline D3D12VertexBuffer::D3D12VertexBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, UInt32 InStride, UInt32 InUsage)
		: VertexBuffer(InSizeInBytes, InStride, InUsage)
		, D3D12Buffer(InDevice)
		, VertexBufferView()
	{
	}
	
	~D3D12VertexBuffer() = default;

	// Map
	virtual Void* Map(const Range* MappedRange) override
	{
		return D3D12Resource::Map(MappedRange);
	}

	virtual void Unmap(const Range* WrittenRange) override
	{
		D3D12Resource::Unmap(WrittenRange);
	}

	virtual UInt64 GetRequiredAlignment() const override final
	{
		return 1;
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

class D3D12IndexBuffer : public IndexBuffer, public D3D12Buffer
{
	friend class D3D12RenderingAPI;

public:
	inline D3D12IndexBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, EIndexFormat InIndexFormat, UInt32 InUsage)
		: IndexBuffer(InSizeInBytes, InIndexFormat, InUsage)
		, D3D12Buffer(InDevice)
		, IndexBufferView()
	{
	}

	~D3D12IndexBuffer() = default;

	// Map
	virtual Void* Map(const Range* MappedRange) override
	{
		return D3D12Resource::Map(MappedRange);
	}

	virtual void Unmap(const Range* WrittenRange) override
	{
		D3D12Resource::Unmap(WrittenRange);
	}

	virtual UInt64 GetRequiredAlignment() const override final
	{
		return 1;
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

class D3D12ConstantBuffer : public ConstantBuffer, public D3D12Buffer
{
	friend class D3D12RenderingAPI;

public:
	inline D3D12ConstantBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, UInt32 InUsage)
		: ConstantBuffer(InSizeInBytes, InUsage)
		, D3D12Buffer(InDevice)
		, View(nullptr)
	{
	}

	~D3D12ConstantBuffer() = default;

	// Map
	virtual Void* Map(const Range* MappedRange) override
	{
		return D3D12Resource::Map(MappedRange);
	}

	virtual void Unmap(const Range* WrittenRange) override
	{
		D3D12Resource::Unmap(WrittenRange);
	}

	virtual UInt64 GetRequiredAlignment() const override final
	{
		return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	}

private:
	class D3D12ConstantBufferView* View;
};

/*
* D3D12StructuredBuffer
*/

class D3D12StructuredBuffer : public StructuredBuffer, public D3D12Buffer
{
	friend class D3D12RenderingAPI;

public:
	inline D3D12StructuredBuffer(D3D12Device* InDevice, UInt32 InSizeInBytes, UInt32 InStride, UInt32 InUsage)
		: StructuredBuffer(InSizeInBytes, InStride, InUsage)
		, D3D12Buffer(InDevice)
	{
	}

	~D3D12StructuredBuffer() = default;

	// Map
	virtual Void* Map(const Range* MappedRange) override
	{
		return D3D12Resource::Map(MappedRange);
	}

	virtual void Unmap(const Range* WrittenRange) override
	{
		D3D12Resource::Unmap(WrittenRange);
	}

	virtual UInt64 GetRequiredAlignment() const override final
	{
		return 1;
	}
};

/*
* Cast a buffer to correct type
*/

inline D3D12Buffer* D3D12BufferCast(Buffer* Buffer)
{
	if (Buffer->AsVertexBuffer())
	{
		return static_cast<D3D12VertexBuffer*>(Buffer);
	}
	else if (Buffer->AsIndexBuffer())
	{
		return static_cast<D3D12IndexBuffer*>(Buffer);
	}
	else if (Buffer->AsConstantBuffer())
	{
		return static_cast<D3D12ConstantBuffer*>(Buffer);
	}
	else if (Buffer->AsStructuredBuffer())
	{
		return static_cast<D3D12StructuredBuffer*>(Buffer);
	}
	else
	{
		return nullptr;
	}
}

inline const D3D12Buffer* D3D12BufferCast(const Buffer* Buffer)
{
	if (Buffer->AsVertexBuffer())
	{
		return static_cast<const D3D12VertexBuffer*>(Buffer);
	}
	else if (Buffer->AsIndexBuffer())
	{
		return static_cast<const D3D12IndexBuffer*>(Buffer);
	}
	else if (Buffer->AsConstantBuffer())
	{
		return static_cast<const D3D12ConstantBuffer*>(Buffer);
	}
	else if (Buffer->AsStructuredBuffer())
	{
		return static_cast<const D3D12StructuredBuffer*>(Buffer);
	}
	else
	{
		return nullptr;
	}
}
