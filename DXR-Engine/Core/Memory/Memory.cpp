#include "Memory.h"

#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <crtdbg.h>
#endif

void* Memory::Malloc( uint64 Size )
{
    /*
    * Since malloc is not guarateed to return nullptr, we check for it here
    * Source: https://www.cplusplus.com/reference/cstdlib/malloc/
    */
    if ( Size )
    {
        return ::malloc( Size );
    }
    else
    {
        return nullptr;
    }
}

void* Memory::Realloc( void* Pointer, uint64 Size )
{
    return ::realloc( Pointer, Size );
}

void Memory::Free( void* Ptr )
{
    ::free( Ptr );
}

char* Memory::Strcpy( char* Destination, const char* Source )
{
    return ::strcpy( Destination, Source );
}

void* Memory::Memset( void* Destination, uint8 Value, uint64 Size )
{
    return ::memset( Destination, static_cast<int>(Value), Size );
}

void* Memory::Memzero( void* Destination, uint64 Size )
{
    return ::memset( Destination, 0, Size );
}

void* Memory::Memcpy( void* Destination, const void* Source, uint64 Size )
{
    return ::memcpy( Destination, Source, Size );
}

void* Memory::Memmove( void* Destination, const void* Source, uint64 Size )
{
    return ::memmove( Destination, Source, Size );
}
