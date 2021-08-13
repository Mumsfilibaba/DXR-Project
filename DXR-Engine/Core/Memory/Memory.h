#pragma once
#include "Core.h"

/* Class handling memory */
class Memory
{
public:
    static void* Malloc( uint64 Size ) noexcept;
    static void* Realloc( void* Pointer, uint64 Size ) noexcept;
    static void  Free( void* Ptr ) noexcept;

    template<typename T>
    static FORCEINLINE T* Malloc( uint32 Count ) noexcept
    {
        return reinterpret_cast<T*>(Malloc( sizeof( T ) * Count ));
    }

    template<typename T>
    static FORCEINLINE T* Realloc( T* Pointer, uint64 Count ) noexcept
    {
        return reinterpret_cast<T*>(Realloc( reinterpret_cast<void*>(Pointer), sizeof( T ) * Count ));
    }

    static void* Memset( void* Destination, uint8 Value, uint64 Size ) noexcept;
    static void* Memzero( void* Destination, uint64 Size ) noexcept;

    template<typename T>
    static FORCEINLINE T* Memzero( T* Destination ) noexcept
    {
        return reinterpret_cast<T*>(Memzero( reinterpret_cast<void*>(Destination), sizeof( T ) ));
    }

    static void* Memcpy( void* Destination, const void* Source, uint64 Size ) noexcept;

    template<typename T>
    static FORCEINLINE T* Memcpy( T* Destination, const T* Source ) noexcept
    {
        return reinterpret_cast<T*>(Memcpy( reinterpret_cast<void*>(Destination), reinterpret_cast<const void*>(Source), sizeof( T ) ));
    }

    static void* Memmove( void* Destination, const void* Source, uint64 Size ) noexcept;

    static char* Strcpy( char* Destination, const char* Source ) noexcept;

    // TODO: Check if we need all the information form memcmp and refactor in that case
    static bool Memcmp( const void* LHS, const void* RHS, uint64 Size ) noexcept;

    template<typename T>
    static FORCEINLINE bool Memcmp( const T* LHS, const T* RHS, uint64 Count ) noexcept
    {
        return Memcmp( reinterpret_cast<const void*>(LHS), reinterpret_cast<const void*>(RHS), sizeof( T ) * Count );
    }

    static FORCEINLINE void Memexchange( void* Destination, void* Source, uint64 Size ) noexcept
    {
        if ( Destination != Source )
        {
            Memcpy( Destination, Source, Size );
            Memzero( Source, Size );
        }
    }

    template<typename T>
    static FORCEINLINE T* Memexchange( void* Destination, void* Source ) noexcept
    {
        return Memexchange( reinterpret_cast<void*>(Destination), reinterpret_cast<void*>(Source), sizeof( T ) );
    }
};