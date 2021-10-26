#include "ClassType.h"

CClassType::CClassType( const CClassType* InSuperClass, const SClassDescription& ClassDescription )
    : Name( nullptr )
    , SuperClass( InSuperClass )
    , SizeInBytes( 0 )
    , Alignment( 0 )
{
    Name = ClassDescription.Name;
    SizeInBytes = ClassDescription.SizeInBytes;
    Alignment = ClassDescription.Alignment;
}

bool CClassType::IsSubClassOf( const CClassType* Class ) const
{
    for ( const CClassType* Current = this; Current; Current = Current->GetSuperClass() )
    {
        if ( Current == Class )
        {
            return true;
        }
    }

    return false;
}
