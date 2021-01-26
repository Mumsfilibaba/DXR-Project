#include "ClassType.h"

ClassType::ClassType(const Char* InName, const ClassType* InSuperClass)
    : Name(InName)
    , SuperClass(InSuperClass)
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
