#include "ClassType.h"

ClassType::ClassType(
    const char* InName,
    const ClassType* InSuperClass,
    uint32 SizeInBytes )
    : Name( InName )
    , SuperClass( InSuperClass )
    , SizeInBytes( SizeInBytes )
{
}

bool ClassType::IsSubClassOf( const ClassType* Class ) const
{
    for ( const ClassType* Current = this; Current; Current = Current->GetSuperClass() )
    {
        if ( Current == Class )
        {
            return true;
        }
    }

    return false;
}
