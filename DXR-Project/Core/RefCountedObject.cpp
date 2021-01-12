#include "RefCountedObject.h"

/*
* RefCountedObject
*/

RefCountedObject::RefCountedObject()
	: StrongReferences(0)
{
	AddRef();
}

RefCountedObject::RefCountType RefCountedObject::AddRef()
{
	return ++StrongReferences;
}

RefCountedObject::RefCountType RefCountedObject::Release()
{
	RefCountType NewRefCount = --StrongReferences;
	if (StrongReferences <= 0)
	{
		delete this;
	}

	return NewRefCount;
}
