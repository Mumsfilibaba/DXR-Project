#pragma once
#include "Defines.h"
#include "Types.h"

class RefCountedObject
{
public:
	RefCountedObject();
	virtual ~RefCountedObject();

	Uint32 AddRef();
	Uint32 Release();

	FORCEINLINE Uint32 GetRefCount() const
	{
		return StrongReferences;
	}

private:
	Uint32 StrongReferences;
};