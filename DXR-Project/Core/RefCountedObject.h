#pragma once
#include "Core.h"

/*
* RefCountedObject
*/

class RefCountedObject
{
	using RefCountType = UInt32;

public:
	RefCountedObject();
	virtual ~RefCountedObject() = default;

	RefCountType AddRef();
	RefCountType Release();

	FORCEINLINE RefCountType GetRefCount() const
	{
		return StrongReferences;
	}

private:
	RefCountType StrongReferences;
};