#pragma once
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "Move.h"
#include "IsTrivial.h"
#include "EnableIf.h"
#include "Not.h"
#include "And.h"
#include "IsMovable.h"
#include "IsCopyable.h"

#include "Core/Memory/Memory.h"

/* Construct the objects in the range by calling the default contructor */
template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type DefaultConstructRange( void* const StartAddress, uint32 Count ) noexcept
{
    while ( Count )
    {
        new(StartAddress) T();
        StartAddress = reinterpret_cast<T*>(StartAddress)++;
        Count--;
    }
}

/* For trivial types, construct the objects in the range by calling Memory::Memzero */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type DefaultConstructRange( void* const StartAddress, uint32 Count ) noexcept
{
    Memory::Memzero( StartAddress, sizeof( T ) * Count );
}

/* Default construct a single object */
template<typename T>
FORCEINLINE void DefaultConstruct( void* const Address ) noexcept
{
    DefaultConstructRange<T>( Address, 1 );
}

/* Construct range and initialize all values to a certain default value */
template<typename T, typename FromType = T>
FORCEINLINE typename TEnableIf<TIsConstructible<T, typename TAddLeftReference<const FillType>::Type>::Value>::Type ConstructRangeFrom( void* const StartAddress, uint32 Count, const FillType& Element ) noexcept
{
    while ( Count )
    {
        new(StartAddress) T( Element );
        StartAddress = reinterpret_cast<T*>(StartAddress)++;
        Count--;
    }
}

/* Construct range and initialize all values to a certain default value */
template<typename T, typename FromType = T>
FORCEINLINE typename TEnableIf<TIsConstructible<T, typename TAddLeftReference<const FillType>::Type>::Value>::Type  ConstructRangeFrom( void* const StartAddress, uint32 Count, FromType&& Element ) noexcept
{
    while ( Count )
    {
        new(StartAddress) T( ::Forward<FromType>( Element ) );
        StartAddress = reinterpret_cast<T*>(StartAddress)++;
        Count--;
    }
}

/* Construct the objects in the range by calling the copy contructor */
template<typename T>
FORCEINLINE typename TEnableIf<!TIsTrivial<T>::Value>::Type CopyConstructRange( void* const StartAddress, const T* Source, uint32 Count ) noexcept
{
    while ( Count )
    {
        new(StartAddress) T( *Source );
        StartAddress = reinterpret_cast<T*>(StartAddress)++;
        Source++;
        Count--;
    }
}

/* For trivial objects, construct the objects in the range by calling Memory::Memcpy */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type CopyConstructRange( void* const StartAddress, const T* Source, uint32 Count ) noexcept
{
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
    while ( Count )
    {
        *StartAddress = *Source;
        StartAddress++;
        Source++;
        Count--;
    }
}

/* For trivial objects, copy assign objects in range with Memory::Memcpy */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type CopyAssignRange( T* Destination, const T* Source, uint32 Count ) noexcept
{
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
    while ( Count )
    {
        new(StartAddress) T( ::Move( *Source ) );
        StartAddress = reinterpret_cast<T*>(StartAddress)++;
        Source++;
        Count--;
    }
}

/* For trivial objects, construct the objects in the range by calling Memory::Memcpy and then Memory::Memzero on the source */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type MoveConstructRange( void* const Address, T* Source, uint32 Count ) noexcept
{
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
    while ( Count )
    {
        *Destination = ::Move( *Source );
        Destination++;
        Source++;
        Count--;
    }
}

/* For trivial objects, move assign objects in range with Memory::Memcpy and Memory::Memzero */
template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type MoveAssignRange( T* Destination, const T* Source, uint32 Count ) noexcept
{
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
    while ( Count )
    {
        (StartObject++)->~T();
        Count--;
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

/* Relocates the range to a new memory location, the memory at destination is assumed to be trivial or empty */
template<typename T>
FORCEINLINE typename TEnableIf<TAnd<TNot<TIsTrivial<T>>, TIsMoveConstructable<T>>::Value>::Type RelocateRange( void* Destination, const T* Source, uint32 Count ) noexcept
{
    // Ensures that the function works for overlapping ranges
    if((Source < Destination) && (Destination < Source + Count))
    {
        Destination = reinterpret_cast<T*>(Destination) + Count;
        Source = Source + Count;

        while ( Count )
        {
            new(Destination) T( ::Move( *Source ) );
            Destination = reinterpret_cast<T*>(Destination)--;
            (Source--)->~T();
            Count--;
        }
    }
    else
    {
        while ( Count )
        {
            new(Destination) T( ::Move( *Source ) );
            Destination = reinterpret_cast<T*>(Destination)++;
            (Source++)->~T();
            Count--;
        }
    }
}

template<typename T>
FORCEINLINE typename TEnableIf<TAnd<TNot<TIsTrivial<T>>, TIsCopyConstructable<T>>::Value>::Type RelocateRange( void* Destination, const T* Source, uint32 Count ) noexcept
{
    // Ensures that the function works for overlapping ranges
    if((Source < Destination) && (Destination < Source + Count))
    {
        Destination = reinterpret_cast<T*>(Destination) + Count;
        Source = Source + Count;

        while ( Count )
        {
            new(Destination) T( *Source );
            Destination = reinterpret_cast<T*>(Destination)--;
            (Source--)->~T();
            Count--;
        }
    }
    else
    {
        while ( Count )
        {
            new(Destination) T( *Source );
            Destination = reinterpret_cast<T*>(Destination)++;
            (Source++)->~T();
            Count--;
        }
    }
}

template<typename T>
FORCEINLINE typename TEnableIf<TIsTrivial<T>::Value>::Type RelocateRange( void* Destination, const T* Source, uint32 Count ) noexcept
{
    Memory::Memmove( Destination, reinterpret_cast<void*>(Source), sizeof( T ) * Count );
}