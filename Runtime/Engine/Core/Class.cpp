#include "Class.h"

FClass::FClass(const FClass* InSuperClass, const FClassInfo& InClassInfo)
    : SuperClass(InSuperClass)
    , ClassInfo(InClassInfo)
{ }

bool FClass::IsSubClassOf(const FClass* Class) const
{
    for (const FClass* Current = this; Current; Current = Current->GetSuperClass())
    {
        if (Current == Class)
        {
            return true;
        }
    }

    return false;
}
