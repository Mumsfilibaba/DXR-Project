#include "Memory.h"

#include <cstdlib>
#include <cstring>

#ifdef PLATFORM_WINDOWS
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
        return malloc( Size );
    }
    else
    {
        return nullptr;
    }
}

void* Memory::Realloc( void* Pointer, uint64 Size ) noexcept
{
    return realloc( Pointer, Size );
}

void Memory::Free( void* Ptr ) noexcept
{
    free( Ptr );
}

void* Memory::Memset( void* Destination, uint8 Value, uint64 Size ) noexcept
{
    return memset( Destination, static_cast<int>(Value), Size );
}

void* Memory::Memzero( void* Destination, uint64 Size ) noexcept
{
    return memset( Destination, 0, Size );
}

void* Memory::Memcpy( void* Destination, const void* Source, uint64 Size ) noexcept
{
    return memcpy( Destination, Source, Size );
}

void* Memory::Memmove( void* Destination, const void* Source, uint64 Size ) noexcept
{
    return memmove( Destination, Source, Size );
}

bool Memory::Memcmp( const void* LHS, const void* RHS, uint64 Size )  noexcept
{
    return (memcmp( LHS, RHS, Size ) == 0);
}

void Memory::Memswap( void* LHS, void* RHS, uint64 SizeInBytes ) noexcept
{
    Assert(LHS != nullptr && RHS != nullptr);

    // Move 8 bytes at a time 
    uint64* Left  = reinterpret_cast<uint64*>(LHS);
    uint64* Right = reinterpret_cast<uint64*>(RHS);

    while ( SizeInBytes >= 8 )
    {
        uint64 Temp = *Left;
        *Left  = *Right;
        *Right = Temp;

        Left++;
        Right++;

        SizeInBytes -= 8;
    }

    // Move remaining bytes
    uint8* LeftBytes  = reinterpret_cast<uint8*>(LHS);
    uint8* RightBytes = reinterpret_cast<uint8*>(RHS);

    while ( SizeInBytes )
    {
        uint8 Temp  = *LeftBytes;
        *LeftBytes  = *RightBytes;
        *RightBytes = Temp;

        LeftBytes++;
        RightBytes++;

        SizeInBytes--;
    }
}
