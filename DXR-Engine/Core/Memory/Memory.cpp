#include "Memory.h"

#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <crtdbg.h>
#endif

void* Memory::Malloc( uint64 Size ) noexcept
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

void* Memory::Realloc( void* Pointer, uint64 Size ) noexcept
{
    return ::realloc( Pointer, Size );
}

void Memory::Free( void* Ptr ) noexcept
{
    ::free( Ptr );
}

char* Memory::Strcpy( char* Destination, const char* Source ) noexcept
{
    return ::strcpy( Destination, Source );
}

void* Memory::Memset( void* Destination, uint8 Value, uint64 Size ) noexcept
{
    return ::memset( Destination, static_cast<int>(Value), Size );
}

void* Memory::Memzero( void* Destination, uint64 Size ) noexcept
{
    return ::memset( Destination, 0, Size );
}

void* Memory::Memcpy( void* Destination, const void* Source, uint64 Size ) noexcept
{
    return ::memcpy( Destination, Source, Size );
}

void* Memory::Memmove( void* Destination, const void* Source, uint64 Size ) noexcept
{
    return ::memmove( Destination, Source, Size );
}

 bool Memcmp( const void* LHS, const void* RHS, uint64 Size)  noexcept
 {
     return (::memcmp(LHS, RHS, Size) == 0);
 }
