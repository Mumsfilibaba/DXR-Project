#pragma once
#include "Defines.h"
#include "Types.h"

/*
* RefCountedObject
*/

class RefCountedObject
{
public:
	RefCountedObject();
	virtual ~RefCountedObject();

	uint32 AddRef();
	uint32 Release();

	FORCEINLINE uint32 GetRefCount() const
	{
		return StrongReferences;
	}

private:
	uint32 StrongReferences;
};