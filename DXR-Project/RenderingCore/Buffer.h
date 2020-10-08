#pragma once
#include "Resource.h"

class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class StructuredBuffer;
class ByteAddressBuffer;

/*
* Buffer
*/

class Buffer : public Resource
{
public:
	Buffer()	= default;
	~Buffer()	= default;

	virtual bool Initialize() = 0;

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

	virtual ByteAddressBuffer* AsByteAddressBuffer()
	{
		return nullptr;
	}

	virtual const ByteAddressBuffer* AsByteAddressBuffer() const
	{
		return nullptr;
	}

	// Buffer functions
	virtual Uint64 GetSizeInBytes() const
	{
		return 0;
	}

	virtual Uint64 GetDeviceAddress() const
	{
		return 0;
	}
};

/*
* VertexBuffer
*/

class VertexBuffer : public Buffer
{
public:
	VertexBuffer()	= default;
	~VertexBuffer()	= default;

	// Casting functions
	virtual VertexBuffer* AsVertexBuffer()
	{
		return this;
	}

	virtual const VertexBuffer* AsVertexBuffer() const
	{
		return this;
	}
};

/*
* IndexBuffer
*/

class IndexBuffer : public Buffer
{
public:
	IndexBuffer()	= default;
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

};

/*
* ConstantBuffer
*/

class ConstantBuffer : public Buffer
{
public:
	ConstantBuffer()	= default;
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
	StructuredBuffer()	= default;
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
};

/*
* ByteAddressBuffer
*/

class ByteAddressBuffer : public Buffer
{
public:
	ByteAddressBuffer()		= default;
	~ByteAddressBuffer()	= default;

	// Casting functions
	virtual ByteAddressBuffer* AsByteAddressBuffer() override
	{
		return this;
	}

	virtual const ByteAddressBuffer* AsByteAddressBuffer() const override
	{
		return this;
	}
};