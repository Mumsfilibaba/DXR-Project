#pragma once
#include "Core.h"

class Memory
{
public:
    static void* Malloc( uint64 Size ) noexcept;
    static void* Realloc( void* Pointer, uint64 Size ) noexcept;
    static void  Free( void* Ptr ) noexcept;

    template<typename T>
    static T* Malloc( uint32 Count ) noexcept
    {
        return reinterpret_cast<T*>(Malloc( sizeof( T ) * Count ));
    }

    template<typename T>
    static T* Realloc( T* Pointer, uint64 Count ) noexcept
    {
        return reinterpret_cast<T*>(Realloc( Pointer, sizeof( T ) * Count ));
    }

    static void* Memset( void* Destination, uint8 Value, uint64 Size ) noexcept;
    static void* Memzero( void* Destination, uint64 Size ) noexcept;

    template<typename T>
    static T* Memzero( T* Destination ) noexcept
    {
        return reinterpret_cast<T*>(Memzero( Destination, sizeof( T ) ));
    }

    static void* Memcpy( void* Destination, const void* Source, uint64 Size ) noexcept;

    template<typename T>
    static T* Memcpy( T* Destination, const T* Source ) noexcept
    {
        return reinterpret_cast<T*>(Memcpy( Destination, Source, sizeof( T ) ));
    }

    static void* Memmove( void* Destination, const void* Source, uint64 Size ) noexcept;

    static char* Strcpy( char* Destination, const char* Source ) noexcept;

    // TODO: Check if we need all the information form memcmp and refactor in that case
    static bool Memcmp( const void* LHS, const void* RHS, uint64 Size) noexcept;

    template<typename T>
    static bool Memcmp( const T* LHS, const T* RHS, uint64 Count ) noexcept
    {
        return Memcmp( LHS, RHS, sizeof(T) * Count);
    }
};