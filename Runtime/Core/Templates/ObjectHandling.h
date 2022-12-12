#pragma once
#include "Utility.h"
#include "TypeTraits.h"

#include "Core/Core.h"
#include "Core/Memory/Memory.h"

template<typename ElementType, typename SizeType>
FORCEINLINE void DefaultConstructElements(void* StartAddress, SizeType Count) noexcept
{
    if CONSTEXPR (TIsTrivial<ElementType>::Value)
    {
        FMemory::Memzero(StartAddress, sizeof(ElementType) * Count);
    }
    else
    {
        ElementType* CurrentElement = reinterpret_cast<ElementType*>(StartAddress);
        while (Count)
        {
            new(CurrentElement) ElementType();
            ++CurrentElement;
            --Count;
        }
    }
}


template<typename ElementType>
FORCEINLINE void DefaultConstructElement(void* Address) noexcept
{
    DefaultConstructElements<ElementType>(Address, 1);
}

template<typename ElementType, typename SizeType>
FORCEINLINE void ConstructElementsFrom(void* RESTRICT StartAddress, SizeType Count, const ElementType& Element) noexcept
{
    if CONSTEXPR(TIsTrivial<ElementType>::Value && sizeof(ElementType) == sizeof(uint8))
    {
        FMemory::Memset(StartAddress, static_cast<uint8>(Element), sizeof(ElementType) * Count);
    }
    else
    {
        ElementType* CurrentElement = reinterpret_cast<ElementType*>(StartAddress);
        while (Count)
        {
            new(CurrentElement) ElementType(Element);
            ++CurrentElement;
            --Count;
        }
    }
}

template<typename ElementType, typename SizeType>
FORCEINLINE void ConstructElementsFrom(void* RESTRICT StartAddress, SizeType Count, ElementType&& Element) noexcept
{
    if CONSTEXPR(TIsTrivial<ElementType>::Value && sizeof(ElementType) == sizeof(uint8))
    {
        FMemory::Memset(StartAddress, static_cast<uint8>(Forward<ElementType>(Element)), sizeof(ElementType) * Count);
    }
    else
    {
        ElementType* CurrentElement = reinterpret_cast<ElementType*>(StartAddress);
        while (Count)
        {
            new(CurrentElement) ElementType(Forward<ElementType>(Element));
            ++CurrentElement;
            --Count;
        }
    }
}


template<typename ElementType, typename SizeType>
FORCEINLINE void CopyConstructElements(void* RESTRICT StartAddress, const ElementType* RESTRICT Source, SizeType Count) noexcept
{
    if CONSTEXPR (TIsTrivial<ElementType>::Value)
    {
        FMemory::Memcpy(StartAddress, Source, sizeof(ElementType) * Count);
    }
    else
    {
        ElementType* CurrentElement = reinterpret_cast<ElementType*>(StartAddress);
        while (Count)
        {
            new(CurrentElement) ElementType(*Source);
            ++CurrentElement;
            ++Source;
            --Count;
        }
    }
}


template<typename ElementType>
FORCEINLINE void CopyConstructElement(void* const RESTRICT Address, const ElementType* RESTRICT Source) noexcept
{
    CopyConstructElements<ElementType>(Address, Source, 1);
}


template<typename ElementType, typename SizeType>
FORCEINLINE void CopyAssignElements(ElementType* RESTRICT Destination, const ElementType* RESTRICT Source, SizeType Count) noexcept
{
    if CONSTEXPR (TIsTrivial<ElementType>::Value)
    {
        FMemory::Memcpy(Destination, Source, sizeof(ElementType) * Count);
    }
    else
    {
        while (Count)
        {
            *Destination = *Source;
            ++Destination;
            ++Source;
            --Count;
        }
    }
}


template<typename ElementType>
FORCEINLINE void CopyAssignElement(ElementType* RESTRICT Destination, const ElementType* RESTRICT Source) noexcept
{
    CopyAssignElements<ElementType>(Destination, Source, 1);
}


template<typename ElementType, typename SizeType>
FORCEINLINE void MoveConstructElements(void* StartAddress, const ElementType* Source, SizeType Count) noexcept
{
    if CONSTEXPR (TIsReallocatable<ElementType>::Value)
    {
        FMemory::Memexchange(StartAddress, Source, sizeof(ElementType) * Count);
    }
    else
    {
        ElementType* CurrentElement = reinterpret_cast<ElementType*>(StartAddress);
        while (Count)
        {
            new(CurrentElement) ElementType(Move(*Source));
            ++CurrentElement;
            ++Source;
            --Count;
        }
    }
}


template<typename ElementType>
FORCEINLINE void MoveConstructElement(void* StartAddress, const ElementType* Source) noexcept
{
    MoveConstructElements<ElementType>(StartAddress, Source, 1);
}


template<typename ElementType, typename SizeType>
FORCEINLINE void MoveAssignElements(ElementType* Destination, const ElementType* Source, SizeType Count) noexcept
{
    if CONSTEXPR(TIsReallocatable<ElementType>::Value)
    {
        FMemory::Memexchange(Destination, Source, sizeof(ElementType) * Count);
    }
    else
    {
        while (Count)
        {
            *Destination = Move(*Source);
            ++Destination;
            ++Source;
            --Count;
        }
    }
}


template<typename ElementType>
FORCEINLINE void MoveAssignElement(ElementType* Destination, const ElementType* Source) noexcept
{
    MoveAssignElements<ElementType>(Destination, Source, 1);
}


template<typename ElementType, typename SizeType>
FORCEINLINE void DestroyElements(ElementType* StartObject, SizeType Count) noexcept
{
    if CONSTEXPR (TNot<TIsTrivial<ElementType>>::Value)
    {
        while (Count)
        {
            typedef ElementType ElementTypeDestructorType;
            StartObject->~ElementTypeDestructorType();
            ++StartObject;
            --Count;
        }
    }
}


template<typename ElementType>
FORCEINLINE void DestroyElement(ElementType* Object) noexcept
{
    DestroyElements<ElementType>(Object, 1);
}


template<typename ElementType, typename SizeType>
FORCEINLINE void RelocateElements(void* StartAddress, ElementType* Source, SizeType Count) noexcept
{
    static_assert(TIsReallocatable<ElementType>::Value || TIsMoveConstructable<ElementType>::Value || TIsCopyConstructable<ElementType>::Value, "ElementType cannot be relocated");

    if CONSTEXPR (TIsReallocatable<ElementType>::Value)
    {
        FMemory::Memmove(StartAddress, Source, sizeof(ElementType) * Count);
    }
    else
    {
        typedef ElementType ElementTypeDestructorType;
        if CONSTEXPR (TIsMoveConstructable<ElementType>::Value)
        {
            // Ensures that the function works for overlapping ranges
            ElementType* CurrentElement = reinterpret_cast<ElementType*>(StartAddress);
            if ((Source < CurrentElement) && (CurrentElement < Source + Count))
            {
                CurrentElement += Count;
                Source         += Count;

                while (Count)
                {
                    --Source;
                    --CurrentElement;
                    new(CurrentElement) ElementType(Move(*Source));
                    Source->~ElementTypeDestructorType();
                    --Count;
                }
            }
            else
            {
                while (Count)
                {
                    new(CurrentElement) ElementType(Move(*Source));
                    Source->~ElementTypeDestructorType();
                    ++CurrentElement;
                    ++Source;
                    --Count;
                }
            }
        }
        else
        {
            // Ensures that the function works for overlapping ranges
            ElementType* CurrentElement = reinterpret_cast<ElementType*>(StartAddress);
            if ((Source < CurrentElement) && (CurrentElement < Source + Count))
            {
                CurrentElement += Count;
                Source         += Count;

                while (Count)
                {
                    --CurrentElement;
                    --Source;
                    new(CurrentElement) ElementType(*Source);
                    Source->~ElementTypeDestructorType();
                    --Count;
                }
            }
            else
            {
                while (Count)
                {
                    new(CurrentElement) ElementType(*Source);
                    Source->~ElementTypeDestructorType();
                    ++CurrentElement;
                    ++Source;
                    --Count;
                }
            }
        }
    }
}


template<typename ElementType, typename SizeType>
FORCEINLINE bool CompareElements(const ElementType* LHS, const ElementType* RHS, SizeType Count) noexcept
{
    if CONSTEXPR (TIsTrivial<ElementType>::Value)
    {
        return FMemory::Memcmp(LHS, RHS, Count * sizeof(ElementType));
    }
    else
    {
        while (Count)
        {
            if (!(*(LHS++) == *(RHS++)))
            {
                return false;
            }

            --Count;
        }

        return true;
    }
}


template<typename ElementType, typename SizeType>
FORCEINLINE void AssignElements(ElementType* RESTRICT Dst, const ElementType& Element, SizeType Count) noexcept
{
    if CONSTEXPR (TIsTrivial<ElementType>::Value && sizeof(ElementType) == sizeof(uint8))
    {
        FMemory::Memset(Dst, static_cast<uint8>(Element), sizeof(ElementType) * Count);
    }
    else
    {
        while (Count)
        {
            *Dst = Element;
            ++Dst;
            --Count;
        }
    }
}


template<typename ElementType, typename SizeType>
FORCEINLINE ElementType* AssignElementsAndReturn(ElementType* RESTRICT Dst, const ElementType& Element, SizeType Count) noexcept
{
    if CONSTEXPR(TIsTrivial<ElementType>::Value && sizeof(ElementType) == sizeof(uint8))
    {
        return reinterpret_cast<ElementType*>(FMemory::Memset(Dst, static_cast<uint8>(Element), sizeof(ElementType) * Count));
    }
    else
    {
        while (Count)
        {
            *Dst = Element;
            ++Dst;
            --Count;
        }

        return Dst;
    }
}
