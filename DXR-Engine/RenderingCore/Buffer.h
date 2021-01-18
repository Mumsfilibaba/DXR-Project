#pragma once
#include "Resource.h"
#include "ResourceViews.h"

class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class StructuredBuffer;

/*
* EBufferUsage
*/

enum EBufferUsage : UInt32
{
	BufferUsage_None	= 0,	
	BufferUsage_Default	= FLAG(1), // GPU Memory
	BufferUsage_Dynamic	= FLAG(2), // CPU Memory
	BufferUsage_UAV		= FLAG(3), // Can be used in UnorderedAccessViews
	BufferUsage_SRV		= FLAG(4), // Can be used in ShaderResourceViews
};

/*
* Range
*/

struct Range
{
	Range() = default;

	inline Range(UInt32 InOffset, UInt32 InSize)
		: Offset(InOffset)
		, Size(InSize)
	{
	}

	UInt32 Offset	= 0;
	UInt32 Size		= 0;
};

/*
* Buffer
*/

class Buffer : public Resource
{
public:
	inline Buffer(UInt32 InSizeInBytes, UInt32 InUsage)
		: SizeInBytes(InSizeInBytes)
		, Usage(InUsage)
	{
	}

	// Casting functions
	virtual Buffer* AsBuffer() override
	{
		return this;
	}

	virtual const Buffer* AsBuffer() const override
	{
		return this;
	}

	virtual VertexBuffer* AsVertexBuffer()
	{
		return nullptr;
	}

	virtual const VertexBuffer* AsVertexBuffer() const
	{
		return nullptr;
	}

	virtual IndexBuffer* AsIndexBuffer()
	{
		return nullptr;
	}

	virtual const IndexBuffer* AsIndexBuffer() const
	{
		return nullptr;
	}

	virtual ConstantBuffer* AsConstantBuffer()
	{
		return nullptr;
	}

	virtual const ConstantBuffer* AsConstantBuffer() const
	{
		return nullptr;
	}

	virtual StructuredBuffer* AsStructuredBuffer()
	{
		return nullptr;
	}

	virtual const StructuredBuffer* AsStructuredBuffer() const
	{
		return nullptr;
	}

	// Buffer functions
	virtual UInt64 GetSizeInBytes() const
	{
		return SizeInBytes;
	}

	virtual UInt64 GetRequiredAlignment() const
	{
		return 0;
	}

	// Map
	virtual Void*	Map(const Range* MappedRange)	 = 0;
	virtual void	Unmap(const Range* WrittenRange) = 0;

	// Usage
	FORCEINLINE UInt32 GetUsage() const
	{
		return Usage;
	}

	FORCEINLINE bool HasShaderResourceUsage() const
	{
		return Usage & BufferUsage_SRV;
	}

	FORCEINLINE bool HasUnorderedAccessUsage() const
	{
		return Usage & BufferUsage_UAV;
	}

	FORCEINLINE bool HasDynamicUsage() const
	{
		return Usage & BufferUsage_Dynamic;
	}

protected:
	UInt32 SizeInBytes;
	UInt32 Usage;
};

/*
* VertexBuffer
*/

class VertexBuffer : public Buffer
{
public:
	inline VertexBuffer(UInt32 SizeInBytes, UInt32 InStride, UInt32 Usage)
		: Buffer(SizeInBytes, Usage)
		, Stride(InStride)
	{
	}

	// Casting functions
	virtual VertexBuffer* AsVertexBuffer() override
	{
		return this;
	}

	virtual const VertexBuffer* AsVertexBuffer() const override
	{
		return this;
	}

	FORCEINLINE UInt32 GetStride() const
	{
		return Stride;
	}

protected:
	UInt32 Stride;
};

/*
* EIndexFormat
*/

enum class EIndexFormat
{
	IndexFormat_UInt16 = 1,
	IndexFormat_UInt32 = 2
};

/*
* IndexBuffer
*/

class IndexBuffer : public Buffer
{
public:
	inline IndexBuffer(UInt32 SizeInBytes, EIndexFormat InIndexFormat, UInt32 Usage)
		: Buffer(SizeInBytes, Usage)
		, IndexFormat(InIndexFormat)
	{
	}
	
	// Casting functions
	virtual IndexBuffer* AsIndexBuffer() override
	{
		return this;
	}

	virtual const IndexBuffer* AsIndexBuffer() const override
	{
		return this;
	}

	FORCEINLINE EIndexFormat GetIndexFormat() const
	{
		return IndexFormat;
	}

protected:
	EIndexFormat IndexFormat;
};

/*
* ConstantBuffer
*/

class ConstantBuffer : public Buffer
{
public:
	inline ConstantBuffer(UInt32 SizeInBytes, UInt32 Usage)
		: Buffer(SizeInBytes, Usage)
	{
	}
	
	// Casting functions
	virtual ConstantBuffer* AsConstantBuffer() override
	{
		return this;
	}

	virtual const ConstantBuffer* AsConstantBuffer() const override
	{
		return this;
	}
};

/*
* StructuredBuffer
*/

class StructuredBuffer : public Buffer
{
public:
	inline StructuredBuffer(UInt32 SizeInBytes, UInt32 InStride, UInt32 Usage)
		: Buffer(SizeInBytes, Usage)
		, Stride(InStride)
	{
	}

	// Casting functions
	virtual StructuredBuffer* AsStructuredBuffer() override
	{
		return this;
	}

	virtual const StructuredBuffer* AsStructuredBuffer() const override
	{
		return this;
	}

	FORCEINLINE UInt32 GetStride() const
	{
		return Stride;
	}

protected:
	UInt32 Stride;
};