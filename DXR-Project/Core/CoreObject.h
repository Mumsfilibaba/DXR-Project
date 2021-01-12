#pragma once
#include "ClassType.h"

/*
* Helper macros
*/

#define CORE_OBJECT(TCoreObject, TSuperClass) \
private: \
	typedef TCoreObject This; \
	typedef TSuperClass Super; \
\
public: \
	static ClassType* GetStaticClass() \
	{ \
		static ClassType ClassInfo(#TCoreObject, Super::GetStaticClass()); \
		return &ClassInfo; \
	}

#define CORE_OBJECT_INIT() \
	this->SetClass(This::GetStaticClass())

/*
* Core Object for Engine (Mostly for RTTI)
*/

class CoreObject
{
public:
	virtual ~CoreObject() = default;

	FORCEINLINE const ClassType* GetClass() const
	{
		return Class;
	}

	static const ClassType* GetStaticClass()
	{
		static ClassType ClassInfo("CoreObject", nullptr);
		return &ClassInfo;
	}

protected:
	FORCEINLINE void SetClass(const ClassType* InClass)
	{
		Class = InClass;
	}

private:
	const ClassType* Class = nullptr;
};

/*
* Casting between coreobjects
*/

template<typename T>
bool IsSubClassOf(CoreObject* Object)
{
	VALIDATE(Object != nullptr);
	VALIDATE(Object->GetClass() != nullptr);
	return Object->GetClass()->IsSubClassOf<T>();
}

template<typename T>
T* Cast(CoreObject* Object)
{
	return IsSubClassOf<T>(Object) ? static_cast<T*>(Object) : nullptr; 
}
