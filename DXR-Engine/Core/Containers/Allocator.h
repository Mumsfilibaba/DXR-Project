#pragma once
#include "Core/Types.h"

#include <cstdlib>

struct Mallocator
{
    FORCEINLINE void* Allocate( uint32 Size )
    {
        return malloc( Size );
    }

    FORCEINLINE void Free( void* Ptr )
    {
        free( Ptr );
    }
};