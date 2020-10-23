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

enum EBufferUsage : Uint32
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
	inline Range()
		: Offset(0)
		, Size(0)
	{
	}

	inline Range(Uint32 InOffset, Uint32 InSize)
		: Offset(InOffset)
		, Size(InSize)
	{
	}

	Uint32 Offset;
	Uint32 Size;
};

/*
* Buffer
*/

class Buffer : public Resource
{
public:
	inline Buffer(Uint32 InSizeInBytes, Uint32 InUsage)
		: SizeInBytes(InSizeInBytes)
		, Usage(InUsage)
	{
	}

	~Buffer() = default;

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
	virtual Uint64 GetSizeInBytes() const
	{
		return SizeInBytes;
	}

	FORCEINLINE bool HasShaderResourceUsage() const
	{
		return Usage & BufferUsage_SRV;
	}

	FORCEINLINE bool HasUnorderedAccessUsage() const
	{
		return Usage & BufferUsage_UAV;
	}

	// Map
	virtual VoidPtr	Map(const Range* MappedRange)		= 0;
	virtual void	Unmap(const Range* WrittenRange) 	= 0;

protected:
	Uint32 SizeInBytes;
	Uint32 Usage;
};

/*
* VertexBuffer
*/

class VertexBuffer : public Buffer
{
public:
	inline VertexBuffer(Uint32 SizeInBytes, Uint32 InStride, Uint32 Usage)
		: Buffer(SizeInBytes, Usage)
		, Stride(InStride)
	{
	}
	
	~VertexBuffer()	= default;

	// Casting functions
	virtual VertexBuffer* AsVertexBuffer() override
	{
		return this;
	}

	virtual const VertexBuffer* AsVertexBuffer() const override
	{
		return this;
	}

protected:
	Uint32 Stride;
};

/*
* EIndexFormat
*/

enum class EIndexFormat
{
	IndexFormat_Uint16 = 1,
	IndexFormat_Uint32 = 2
};

/*
* IndexBuffer
*/

class IndexBuffer : public Buffer
{
public:
	inline IndexBuffer(Uint32 SizeInBytes, EIndexFormat InIndexFormat, Uint32 Usage)
		: Buffer(SizeInBytes, Usage)
		, IndexFormat(InIndexFormat)
	{
	}
	
	~IndexBuffer()	= default;

	// Casting functions
	virtual IndexBuffer* AsIndexBuffer() override
	{
		return this;
	}

	virtual const IndexBuffer* AsIndexBuffer() const override
	{
		return this;
	}

	virtual EIndexFormat GetIndexFormat() const
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
	inline ConstantBuffer(Uint32 SizeInBytes, Uint32 Usage)
		: Buffer(SizeInBytes, Usage)
	{
	}
	
	~ConstantBuffer()	= default;

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
	inline StructuredBuffer(Uint32 SizeInBytes, Uint32 InStride, Uint32 Usage)
		: Buffer(SizeInBytes, Usage)
		, Stride(InStride)
	{
	}

	~StructuredBuffer()	= default;

	// Casting functions
	virtual StructuredBuffer* AsStructuredBuffer() override
	{
		return this;
	}

	virtual const StructuredBuffer* AsStructuredBuffer() const override
	{
		return this;
	}

	virtual Uint32 GetStride() const
	{
		return Stride;
	}

protected:
	Uint32 Stride;
};

/*
* StructuredBufferRef
*/

struct StructuredBufferRef
{
	inline StructuredBufferRef(const TSharedRef<StructuredBuffer>& InBuffer, const TSharedRef<ShaderResourceView>& InSRV)
		: Buffer(InBuffer)
		, SRV(InSRV)
	{
	}

	TSharedRef<StructuredBuffer> Buffer;
	TSharedRef<ShaderResourceView> SRV;
};

/*
* RWStructuredBufferRef
*/

struct RWStructuredBufferRef
{
	inline RWStructuredBufferRef(const TSharedRef<StructuredBuffer>& InBuffer, const TSharedRef<ShaderResourceView>& InSRV, const TSharedRef<UnorderedAccessView>& InUAV)
		: Buffer(InBuffer)
		, SRV(InSRV)
		, UAV(InUAV)
	{
	}

	TSharedRef<StructuredBuffer>	Buffer;
	TSharedRef<ShaderResourceView>	SRV;
	TSharedRef<UnorderedAccessView> UAV;
};