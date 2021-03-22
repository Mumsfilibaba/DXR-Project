#pragma once
#include "Core.h"

class GenericThread
{
public:
	virtual ~GenericThread() = default;

	virtual void Sleep();

	static GenericThread* Create();
};