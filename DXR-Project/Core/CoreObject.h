#pragma once
#include "Defines.h"
#include "Types.h"

class CoreObject
{
public:
	CoreObject();
	virtual ~CoreObject();

	template<typename T>
	FORCEINLINE bool IsOfType() const
	{
		return false;
	}
};

class Derived : public CoreObject
{
public:

};