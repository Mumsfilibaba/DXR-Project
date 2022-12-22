#include "Class.h"

FClass::FClass(const FClass* InSuperClass, const FClassInfo& ClassDescription)
    : Name(nullptr)
    , SuperClass(InSuperClass)
    , SizeInBytes(0)
    , Alignment(0)
{
    Name        = ClassDescription.Name;
    SizeInBytes = ClassDescription.SizeOf;
    Alignment   = ClassDescription.Alignment;
}

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
