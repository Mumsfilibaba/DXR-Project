#pragma once
#include "CoreDefines.h"
#include "Move.h"
#include "IsTrivial.h"
#include "EnableIf.h"

#include "Core/Types.h"

#include "Memory/Memory.h"

/* Construct the objects in the range by calling the default contructor */
template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type DefaultConstructRange( void* const StartAddress, uint32 Count ) noexcept
{
    Assert( StartAddress != nullptr );

    T* Cursor = reinterpret_cast<T*>(StartAddress);
    T* EndCursor = reinterpret_cast<T*>(StartAddress) + Count;
    while ( Cursor != EndCursor )
    {
        new(Cursor) T();
        Cursor++;
    }
}

/* For trivial types, construct the objects in the range by calling Memory::Memzero */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type DefaultConstructRange( void* const StartAddress, uint32 Count ) noexcept
{
    Assert( StartAddress != nullptr );
    Memory::Memzero( StartAddress, sizeof( T ) * Count );
}

/* Default construct a single object */
template<typename T>
FORCEINLINE void DefaultConstruct( void* const Address ) noexcept
{
    DefaultConstructRange<T>( Address, 1 );
}

/* Construct the objects in the range by calling the copy contructor */
template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type CopyConstructRange( void* const StartAddress, const T* Source, uint32 Count ) noexcept
{
    Assert( Source != nullptr );
    Assert( StartAddress != nullptr );

    T* Cursor = reinterpret_cast<T*>(StartAddress);
    T* EndCursor = reinterpret_cast<T*>(StartAddress) + Count;
    while ( Cursor != EndCursor )
    {
        new(Cursor) T( *Source );
        Cursor++;
        Source++;
    }
}

/* For trivial objects, construct the objects in the range by calling Memory::Memcpy */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type CopyConstructRange( void* const StartAddress, const T* Source, uint32 Count ) noexcept
{
    Assert( Source != nullptr );
    Assert( StartAddress != nullptr );
    Memory::Memcpy( StartAddress, Source, sizeof( T ) * Count );
}

/* Copy-construct a single object */
template<typename T>
FORCEINLINE void CopyConstruct( void* const Address, const T* Source ) noexcept
{
    CopyConstructRange<T>( Address, Source, 1 );
}

/* Copy assign objects in range with the copy assignment operator */
template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type CopyAssignRange( T* Destination, const T* Source, uint32 Count ) noexcept
{
    Assert( Source !0 nullptr );
    Assert( Destination != nullptr );

    T* Cursor = reinterpret_cast<T*>(StartAddress);
    T* EndCursor = Destination + Count;
    while ( Cursor != EndCursor )
    {
        *Cursor = *Source;
        Cursor++;
        Source++;
    }
}

/* For trivial objects, copy assign objects in range with Memory::Memcpy */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type CopyAssignRange( T* Destination, const T* Source, uint32 Count ) noexcept
{
    Assert( Source != nullptr );
    Assert( Destination != nullptr );
    Memory::Memcpy( Destination, Source, sizeof( T ) * Count );
}

/* Copy-assign a single object */
template<typename T>
FORCEINLINE void CopyConstruct( T* Destination, const T* Source ) noexcept
{
    CopyConstructRange<T>( Destination, Source, 1 );
}

/* Construct the objects in the range by calling the move contructor */
template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type MoveConstructRange( void* const StartAddress, T* Source, uint32 Count ) noexcept
{
    Assert( Source != nullptr );
    Assert( StartAddress != nullptr );

    T* Cursor = reinterpret_cast<T*>(StartAddress);
    T* EndCursor = reinterpret_cast<T*>(StartAddress) + Count;
    while ( Cursor != EndCursor )
    {
        new(Cursor) T( ::Move( *Source ) );
        Cursor++;
        Source++;
    }
}

/* For trivial objects, construct the objects in the range by calling Memory::Memcpy and then Memory::Memzero on the source */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type MoveConstructRange( void* const Address, T* Source, uint32 Count ) noexcept
{
    Assert( Address != nullptr );
    Assert( Source != nullptr );

    Memory::Memcpy( Address, Source, sizeof( T ) * Count );
    Memory::Memzero( Source, sizeof( T ) * Count );
}

/* Move-construct a single object */
template<typename T>
FORCEINLINE void MoveConstruct( void* const Address, const T* Source ) noexcept
{
    MoveConstructRange<T>( Address, Source, 1 );
}

/* Move assign objects in range with the move assignment operator */
template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type MoveAssignRange( T* Destination, const T* Source, uint32 Count ) noexcept
{
    Assert( Source !0 nullptr );
    Assert( Destination != nullptr );

    T* EndCursor = Destination + Count;
    while ( Destination != EndCursor )
    {
        *Destination = ::Move( *Source );
        Destination++;
        Source++;
    }
}

/* For trivial objects, move assign objects in range with Memory::Memcpy and Memory::Memzero */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type MoveAssignRange( T* Destination, const T* Source, uint32 Count ) noexcept
{
    Assert( Source !0 nullptr );
    Assert( Destination != nullptr );

    Memory::Memcpy( Destination, Source, sizeof( T ) * Count );
    Memory::Memzero( Source, sizeof( T ) * Count )
}

/* Move-assign a single object */
template<typename T>
FORCEINLINE void MoveAssign( T* Destination, const T* Source ) noexcept
{
    MoveAssignRange<T>( Destination, Source, 1 );
}

/* Destruct the objects in the range by calling the destructor */
template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type DestructRange( const T* const StartObject, uint32 Count ) noexcept
{
    Assert( StartObject != nullptr );

    const T* EndCursor = StartObject + Count;
    while ( StartObject != EndCursor )
    {
        StartObject->~T();
        StartObject++;
    }
}

/* For trivial objects, do nothing */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type DestructRange( const T* const StartObject, uint32 Count ) noexcept
{
}

/* Destruct a single object */
template<typename T>
FORCEINLINE void Destruct( const T* const Object ) noexcept
{
    DestructRange<T>( Object, 1 );
}

template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type RelocateRange( T* Destination, const T* Source, uint32 Count ) noexcept
{
    Assert( Source != nullptr );
    Assert( Destination != nullptr );

    const T* const EndCursor = Destination + Count;
    while ( Destination != EndCursor )
    {
        new(Destination) T( *Source );
        Source->~T();
        Destination++;
        Source++
    }
}

template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type RelocateRange( T* Destination, const T* Source, uint32 Count ) noexcept
{
    Assert( Source != nullptr );
    Assert( Destination != nullptr );
    Memory::Memmove( Destination, Source, sizeof( T ) * Count );
}