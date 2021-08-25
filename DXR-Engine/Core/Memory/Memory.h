#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"

/* Class handling memory */
class Memory
{
public:
    static void* Malloc( uint64 Size ) noexcept;
    static void* Realloc( void* Pointer, uint64 Size ) noexcept;
    static void  Free( void* Ptr ) noexcept;

    static void* Memmove( void* Destination, const void* Source, uint64 Size ) noexcept;
    static void* Memcpy( void* Destination, const void* Source, uint64 Size ) noexcept;
    static void* Memset( void* Destination, uint8 Value, uint64 Size ) noexcept;
    static void* Memzero( void* Destination, uint64 Size ) noexcept;

    // TODO: Check if we need all the information from memcmp and refactor in that case
    static bool Memcmp( const void* LHS, const void* RHS, uint64 Size ) noexcept;

    static void Memswap( void* LHS, void* RHS, uint64 Size ) noexcept;

    static FORCEINLINE void Memexchange( void* Destination, void* Source, uint64 Size ) noexcept
    {
        if ( Destination != Source )
        {
            Memcpy( Destination, Source, Size );
            Memzero( Source, Size );
        }
    }

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

    template<typename T>
    static FORCEINLINE T* Memzero( T* Destination ) noexcept
    {
        return reinterpret_cast<T*>(Memzero( reinterpret_cast<void*>(Destination), sizeof( T ) ));
    }

    template<typename T>
    static FORCEINLINE T* Memcpy( T* Destination, const T* Source ) noexcept
    {
        return reinterpret_cast<T*>(Memcpy( reinterpret_cast<void*>(Destination), reinterpret_cast<const void*>(Source), sizeof( T ) ));
    }

    template<typename T>
    static FORCEINLINE bool Memcmp( const T* LHS, const T* RHS, uint64 Count ) noexcept
    {
        return Memcmp( reinterpret_cast<const void*>(LHS), reinterpret_cast<const void*>(RHS), sizeof( T ) * Count );
    }

    template<typename T>
    static FORCEINLINE T* Memexchange( void* Destination, void* Source ) noexcept
    {
        return Memexchange( reinterpret_cast<void*>(Destination), reinterpret_cast<void*>(Source), sizeof( T ) );
    }
};