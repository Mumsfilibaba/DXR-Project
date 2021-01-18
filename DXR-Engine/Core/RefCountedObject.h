#pragma once
#include "Core.h"

/*
* RefCountedObject
*/

class RefCountedObject
{
public:
	using RefCountType = UInt32;

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