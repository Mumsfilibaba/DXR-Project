#include "ClassType.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FClassType

FClassType::FClassType(const FClassType* InSuperClass, const FClassDescription& ClassDescription)
    : Name(nullptr)
    , SuperClass(InSuperClass)
    , SizeInBytes(0)
    , Alignment(0)
{
    Name = ClassDescription.Name;
    SizeInBytes = ClassDescription.SizeInBytes;
    Alignment = ClassDescription.Alignment;
}

bool FClassType::IsSubClassOf(const FClassType* Class) const
{
    for (const FClassType* Current = this; Current; Current = Current->GetSuperClass())
    {
        if (Current == Class)
        {
            return true;
        }
    }

    return false;
}
