#pragma once
#include "Defines.h"
#include "Types.h"

/*
* Class for storing ClassInfo
*/

class ClassType
{
public:
	ClassType(const Char* InName, const ClassType* InSuperClass);
	~ClassType() = default;

	bool IsSubClassOf(const ClassType* Class) const;

	template<typename T>
	FORCEINLINE bool IsSubClassOf() const
	{
		return IsSubClassOf(T::GetStaticClass());
	}

	FORCEINLINE const Char* GetName() const
	{
		return Name;
	}

	FORCEINLINE const ClassType* GetSuperClass() const
	{
		return SuperClass;
	}

private:
	const Char* Name;
	const ClassType* SuperClass;
};