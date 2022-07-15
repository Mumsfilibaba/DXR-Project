#pragma once
#include "Core/Core.h"
#include "Core/Templates/ClassUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMemoryStack

class FMemoryStack : FNonCopyable
{
	struct FMemoryChunk
	{
		void* Data()
		{
			return reinterpret_cast<uint8*>(this) + sizeof(FMemoryChunk);
		}

		FMemoryChunk* Next;
		int32         Size;
	};

public:
	FMemoryStack()
		: TopChunk(nullptr)
	{ }

	explicit FMemoryStack(int32 Size)
		: TopChunk(nullptr)
	{
		AllocateNewChunk(Size);
	}
	
	FMemoryStack(FMemoryStack&& Other)
		: TopChunk(Other.TopChunk)
	{
		Other.TopChunk = nullptr;
	}

	~FMemoryStack()
	{
		FreeAllChunks();
	}

	void* Allocate(int32 Size, int32 Alignment = STANDARD_ALIGNMENT)
	{
		Check(Size      > 0);
		Check(Alignment > 0);

		int32 AlignedSize = 
	}

	void* PushBytes(int32 Size, int32 Alignment = STANDARD_ALIGNMENT);
	void  Pop();

	bool IsEmpty() const;

private:
	void* AllocateNewChunk(int32 Size);
	void  FreeAllChunks();

	FMemoryChunk* TopChunk;
};