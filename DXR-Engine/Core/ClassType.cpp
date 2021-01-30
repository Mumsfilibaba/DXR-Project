#include "ClassType.h"

ClassType::ClassType(
    const Char* InName,
    const ClassType* InSuperClass,
    UInt32 SizeInBytes)
    : Name(InName)
    , SuperClass(InSuperClass)
    , SizeInBytes(SizeInBytes)
{
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
