#pragma once
#include "Resource.h"

/*
* EBufferFlag
*/

typedef Uint32 BufferFlags;
enum EBufferFlag : BufferFlags
{
	BUFFER_FLAG_NONE				= 0,
	BUFFER_FLAG_UNORDERED_ACCESS	= FLAG(1),
	BUFFER_FLAG_CONSTANT_BUFFER		= FLAG(2),
	BUFFER_FLAG_SHADER_RESOURCE		= FLAG(3),
};

/*
* BufferInitializer
*/

struct BufferInitializer
{
	inline BufferInitializer()
		: Flags(0)
		, SizeInBytes(0)
		, MemoryType(EMemoryType::MEMORY_TYPE_GPU)
	{
	}

	inline BufferInitializer(BufferFlags InFlags, Uint64 InSizeInBytes, EMemoryType InMemoryType)
		: Flags(InFlags)
		, SizeInBytes(InSizeInBytes)
		, MemoryType(InMemoryType)
	{
	}

	BufferFlags Flags;
	Uint64		SizeInBytes;
	EMemoryType MemoryType;
};

/*
* Buffer
*/

class Buffer : public Resource
{
public:
	Buffer() = default;
	~Buffer() = default;

	virtual bool Initialize(const BufferInitializer& InInitializer) = 0;

	// Casting functions
	virtual Buffer* AsBuffer() override
	{
		return this;
	}

	virtual const Buffer* AsBuffer() const override
	{
		return this;
	}

	virtual Uint64 GetSizeInBytes() const
	{
		return Initializer.SizeInBytes;
	}

	virtual Uint64 GetDeviceAddress() const = 0;

protected:
	BufferInitializer Initializer;
};
