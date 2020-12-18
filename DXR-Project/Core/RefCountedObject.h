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

	UInt32 AddRef();
	UInt32 Release();

	FORCEINLINE UInt32 GetRefCount() const
	{
		return StrongReferences;
	}

private:
	UInt32 StrongReferences;
};