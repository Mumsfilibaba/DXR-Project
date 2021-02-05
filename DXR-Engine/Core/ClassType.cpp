#include "ClassType.h"

#include <cstring>

ClassType::ClassType(const Char* InName, const ClassType* InSuperClass, UInt32 SizeInBytes)
    : Name(InName)
    , SuperClass(InSuperClass)
    , SizeInBytes(SizeInBytes)
{
}

Bool ClassType::IsSame(const ClassType* Class) const
{
    // Since all classes should be a global variable, a class that is same should have same pointer value
    return this == Class;
}

Bool ClassType::IsSubClassOf(const ClassType* Class) const
{
    for (const ClassType* Current = this; Current; Current = Current->GetSuperClass())
    {
        if (Current == Class)
        {
            return true;
        }
    }

    return false;
}
