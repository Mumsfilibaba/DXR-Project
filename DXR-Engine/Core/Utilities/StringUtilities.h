#pragma once
#include "Core/Containers/String.h"

inline void ConvertBackslashes( CString& OutString )
{
    auto Position = OutString.Find( '\\' );
    while ( Position != CString::NPos )
    {
        OutString.Replace( '/', Position );
        Position = OutString.Find( '\\', Position + 1 );
    }
}
