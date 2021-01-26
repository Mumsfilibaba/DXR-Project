#pragma once
#include "Core.h"

// ClassType stores info about a class, for now inheritance

class ClassType
{
public:
    ClassType(const Char* InName, const ClassType* InSuperClass);
    
    Bool IsSubClassOf(const ClassType* Class) const;

    template<typename T>
    FORCEINLINE Bool IsSubClassOf() const
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