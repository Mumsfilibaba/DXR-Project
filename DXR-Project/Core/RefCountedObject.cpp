#include "PreCompiled.h"
#include "RefCountedObject.h"

RefCountedObject::RefCountedObject()
	: StrongReferences(0)
{
}

RefCountedObject::~RefCountedObject()
{
}

Uint32 RefCountedObject::AddRef()
{
	return ++StrongReferences;
}

Uint32 RefCountedObject::Release()
{
	Uint32 NewRefCount = --StrongReferences;
	if (StrongReferences <= 0)
	{
		delete this;
	}

	return NewRefCount;
}
