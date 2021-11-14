#pragma once
#include "Core/CoreModule.h"

#define STANDARD_ALIGNMENT (__STDCPP_DEFAULT_NEW_ALIGNMENT__)

namespace NMemoryUtils
{
    template<typename T>
    inline constexpr T BytesToMegaBytes( T Bytes )
    {
        return Bytes / T(1024 * 1024);
    }

    template<typename T>
    inline constexpr T MegaBytesToBytes( T MegaBytes )
    {
        return MegaBytes * T( 1024 * 1024 );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Class handling memory */
class CORE_API CMemory
{
public:
    static void* Malloc( uint64 Size ) noexcept;
    static void* Realloc( void* Pointer, uint64 Size ) noexcept;
    static void  Free( void* Ptr ) noexcept;

    static void* Memmove( void* Destination, const void* Source, uint64 Size ) noexcept;
    static void* Memcpy( void* restrict_ptr Destination, const void* restrict_ptr Source, uint64 Size ) noexcept;
    static void* Memset( void* Destination, uint8 Value, uint64 Size ) noexcept;
    static void* Memzero( void* Destination, uint64 Size ) noexcept;

    // TODO: Check if we need all the information from memcmp and refactor in that case
    static bool Memcmp( const void* LHS, const void* RHS, uint64 Size ) noexcept;

    /* Assume that LHS and RHS is not the overlapping */
    static void Memswap( void* restrict_ptr LHS, void* restrict_ptr RHS, uint64 Size ) noexcept;

    static FORCEINLINE void Memexchange( void* restrict_ptr Destination, void* restrict_ptr Source, uint64 Size ) noexcept
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
        const uint64 NumBytes = Count * sizeof( T );
        return reinterpret_cast<T*>(Realloc( reinterpret_cast<void*>(Pointer), NumBytes ));
    }

    template<typename T>
    static FORCEINLINE T* Memzero( T* Destination, uint64 SizeInBytes ) noexcept
    {
        return reinterpret_cast<T*>(Memzero( reinterpret_cast<void*>(Destination), SizeInBytes ));
    }

    template<typename T>
    static FORCEINLINE T* Memzero( T* Destination ) noexcept
    {
        return reinterpret_cast<T*>(Memzero( reinterpret_cast<void*>(Destination), sizeof( T ) ));
    }

    template<typename T>
    static FORCEINLINE T* Memcpy( T* restrict_ptr Destination, const T* restrict_ptr Source ) noexcept
    {
        return reinterpret_cast<T*>(Memcpy( reinterpret_cast<void*>(Destination), reinterpret_cast<const void*>(Source), sizeof( T ) ));
    }

    template<typename T>
    static FORCEINLINE bool Memcmp( const T* LHS, const T* RHS, uint64 Count ) noexcept
    {
        return Memcmp( reinterpret_cast<const void*>(LHS), reinterpret_cast<const void*>(RHS), sizeof( T ) * Count );
    }

    template<typename T>
    static FORCEINLINE T* Memexchange( void* restrict_ptr Destination, void* restrict_ptr Source ) noexcept
    {
        return Memexchange( reinterpret_cast<void*>(Destination), reinterpret_cast<void*>(Source), sizeof( T ) );
    }
};
