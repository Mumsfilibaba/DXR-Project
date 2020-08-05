#pragma once
#include "Defines.h"
#include "Types.h"

#define CORE_OBJECT(Name)\
public: \
	static CoreObject* GetStaticClass() \
	{ \
		static Name DefaultObject; \
		return &DefaultObject; \
	}

class CoreObject
{
public:
	CoreObject();
	virtual ~CoreObject();

	bool IsInstanceOf(CoreObject* Other) const;

	template<typename T>
	FORCEINLINE bool IsInstanceOf(CoreObject* Other) const
	{
		return IsInstanceOf(T::GetStaticClass());
	}
};

class Derived : public CoreObject
{
	CORE_OBJECT(Derived);

public:
	Derived();
	~Derived();
};