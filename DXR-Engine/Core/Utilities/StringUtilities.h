#pragma once
#include "Core/Containers/String.h"

inline void ConvertBackslashes( CString& OutString )
{
    auto Position = OutString.Find( '\\' );
    while ( Position != CString::InvalidPosition )
    {
		OutString.Replace( '/', Position );
		Position = OutString.Find( '\\', Position + 1 );
    }
}
