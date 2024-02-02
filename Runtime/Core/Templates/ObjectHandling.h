#pragma once
#include "Utility.h"
#include "TypeTraits.h"

#include "Core/Core.h"
#include "Core/Memory/Memory.h"

#if !BUILD_PRODUCTION
    #define MEMZERO_TRIVIAL_OBJECTS (1)
#else
    #define MEMZERO_TRIVIAL_OBJECTS (0)
#endif

template<typename ObjectType, typename SizeType>
FORCEINLINE void DefaultConstructObjects(void* StartAddress, SizeType Count) noexcept
{
    if constexpr (TIsTrivial<ObjectType>::Value)
    {
    #if MEMZERO_TRIVIAL_OBJECTS
        FMemory::Memzero(StartAddress, sizeof(ObjectType) * Count);
    #endif
    }
    else
    {
        ObjectType* CurrentObject = reinterpret_cast<ObjectType*>(StartAddress);
        while (Count)
        {
            new(CurrentObject) ObjectType();
            ++CurrentObject;
            --Count;
        }
    }
}


template<typename ObjectType>
FORCEINLINE void DefaultConstructObject(void* Address) noexcept
{
    if constexpr (TIsTrivial<ObjectType>::Value)
    {
    #if MEMZERO_TRIVIAL_OBJECTS
        FMemory::Memzero(Address, sizeof(ObjectType));
    #endif
    }
    else
    {
        new(Address) ObjectType();
    }
}

template<typename ObjectType, typename SizeType>
FORCEINLINE void ConstructObjectsFrom(void* RESTRICT StartAddress, SizeType Count, const ObjectType& Object) noexcept
{
    constexpr bool bObjectFitInByte = sizeof(ObjectType) == sizeof(uint8);
    if constexpr(TIsTrivial<ObjectType>::Value && bObjectFitInByte)
    {
        FMemory::Memset(StartAddress, static_cast<uint8>(Object), sizeof(ObjectType) * Count);
    }
    else
    {
        ObjectType* CurrentObject = reinterpret_cast<ObjectType*>(StartAddress);
        while (Count)
        {
            new(CurrentObject) ObjectType(Object);
            ++CurrentObject;
            --Count;
        }
    }
}

template<typename ObjectType, typename SizeType>
FORCEINLINE void ConstructObjectsFrom(void* RESTRICT StartAddress, SizeType Count, ObjectType&& Object) noexcept
{
    constexpr bool bObjectFitInByte = sizeof(ObjectType) == sizeof(uint8);
    if constexpr(TIsTrivial<ObjectType>::Value && bObjectFitInByte)
    {
        FMemory::Memset(StartAddress, static_cast<uint8>(::Forward<ObjectType>(Object)), sizeof(ObjectType) * Count);
    }
    else
    {
        ObjectType* CurrentObject = reinterpret_cast<ObjectType*>(StartAddress);
        while (Count)
        {
            new(CurrentObject) ObjectType(::Forward<ObjectType>(Object));
            ++CurrentObject;
            --Count;
        }
    }
}


template<typename ObjectType, typename SizeType>
FORCEINLINE void CopyConstructObjects(void* RESTRICT StartAddress, const ObjectType* RESTRICT Source, SizeType Count) noexcept
{
    if constexpr (TIsTrivial<ObjectType>::Value)
    {
        FMemory::Memcpy(StartAddress, Source, sizeof(ObjectType) * Count);
    }
    else
    {
        ObjectType* CurrentObject = reinterpret_cast<ObjectType*>(StartAddress);
        while (Count)
        {
            new(CurrentObject) ObjectType(*Source);
            ++CurrentObject;
            ++Source;
            --Count;
        }
    }
}


template<typename ObjectType>
FORCEINLINE void CopyConstructObject(void* const RESTRICT Address, const ObjectType* RESTRICT Source) noexcept
{
    if constexpr (TIsTrivial<ObjectType>::Value)
    {
        FMemory::Memcpy(Address, Source, sizeof(ObjectType));
    }
    else
    {
        new(Address) ObjectType(*Source);
    }
}


template<typename ObjectType, typename SizeType>
FORCEINLINE void CopyAssignObjects(ObjectType* RESTRICT Destination, const ObjectType* RESTRICT Source, SizeType Count) noexcept
{
    if constexpr (TIsTrivial<ObjectType>::Value)
    {
        FMemory::Memcpy(Destination, Source, sizeof(ObjectType) * Count);
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


template<typename ObjectType>
FORCEINLINE void CopyAssignObject(ObjectType* RESTRICT Destination, const ObjectType* RESTRICT Source) noexcept
{
    if constexpr (TIsTrivial<ObjectType>::Value)
    {
        FMemory::Memcpy(Destination, Source, sizeof(ObjectType));
    }
    else
    {
        *Destination = *Source;
    }
}


template<typename ObjectType, typename SizeType>
FORCEINLINE void MoveConstructObjects(void* StartAddress, const ObjectType* Source, SizeType Count) noexcept
{
    if constexpr (TIsReallocatable<ObjectType>::Value)
    {
        FMemory::Memexchange(StartAddress, Source, sizeof(ObjectType) * Count);
    }
    else
    {
        ObjectType* CurrentObject = reinterpret_cast<ObjectType*>(StartAddress);
        while (Count)
        {
            new(CurrentObject) ObjectType(::Move(*Source));
            ++CurrentObject;
            ++Source;
            --Count;
        }
    }
}


template<typename ObjectType>
FORCEINLINE void MoveConstructObject(void* Address, const ObjectType* Source) noexcept
{
    if constexpr (TIsReallocatable<ObjectType>::Value)
    {
        FMemory::Memexchange(Address, Source, sizeof(ObjectType));
    }
    else
    {
        new(Address) ObjectType(::Move(*Source));
    }
}


template<typename ObjectType, typename SizeType>
FORCEINLINE void MoveAssignObjects(ObjectType* Destination, const ObjectType* Source, SizeType Count) noexcept
{
    if constexpr(TIsReallocatable<ObjectType>::Value)
    {
        FMemory::Memexchange(Destination, Source, sizeof(ObjectType) * Count);
    }
    else
    {
        while (Count)
        {
            *Destination = ::Move(*Source);
            ++Destination;
            ++Source;
            --Count;
        }
    }
}


template<typename ObjectType>
FORCEINLINE void MoveAssignObject(ObjectType* Destination, const ObjectType* Source) noexcept
{
    if constexpr(TIsReallocatable<ObjectType>::Value)
    {
        FMemory::Memexchange(Destination, Source, sizeof(ObjectType));
    }
    else
    {
        *Destination = ::Move(*Source);
    }
}


template<typename ObjectType, typename SizeType>
FORCEINLINE void DestroyObjects(ObjectType* StartObject, SizeType Count) noexcept
{
    if constexpr (TNot<TIsTrivial<ObjectType>>::Value)
    {
        while (Count)
        {
            typedef ObjectType ObjectDestructorType;
            StartObject->~ObjectDestructorType();
            ++StartObject;
            --Count;
        }
    }
}


template<typename ObjectType>
FORCEINLINE void DestroyObject(ObjectType* Object) noexcept
{
    if constexpr (TNot<TIsTrivial<ObjectType>>::Value)
    {
        typedef ObjectType ObjectDestructorType;
        Object->~ObjectDestructorType();
    }
}


template<typename ObjectType, typename SizeType>
FORCEINLINE void RelocateObjects(void* StartAddress, ObjectType* Source, SizeType Count) noexcept
{
    static_assert(TIsReallocatable<ObjectType>::Value || TIsMoveConstructable<ObjectType>::Value || TIsCopyConstructable<ObjectType>::Value, "ObjectType cannot be relocated");

    if constexpr (TIsReallocatable<ObjectType>::Value)
    {
        FMemory::Memmove(StartAddress, Source, sizeof(ObjectType) * Count);
    }
    else
    {
        typedef ObjectType ObjectDestructorType;
        if constexpr (TIsMoveConstructable<ObjectType>::Value)
        {
            // Ensures that the function works for overlapping ranges
            ObjectType* CurrentObject = reinterpret_cast<ObjectType*>(StartAddress);
            if ((Source < CurrentObject) && (CurrentObject < Source + Count))
            {
                CurrentObject += Count;
                Source        += Count;

                while (Count)
                {
                    --Source;
                    --CurrentObject;
                    new(CurrentObject) ObjectType(::Move(*Source));
                    Source->~ObjectDestructorType();
                    --Count;
                }
            }
            else
            {
                while (Count)
                {
                    new(CurrentObject) ObjectType(::Move(*Source));
                    Source->~ObjectDestructorType();
                    ++CurrentObject;
                    ++Source;
                    --Count;
                }
            }
        }
        else
        {
            // Ensures that the function works for overlapping ranges
            ObjectType* CurrentObject = reinterpret_cast<ObjectType*>(StartAddress);
            if ((Source < CurrentObject) && (CurrentObject < Source + Count))
            {
                CurrentObject += Count;
                Source        += Count;

                while (Count)
                {
                    --CurrentObject;
                    --Source;
                    new(CurrentObject) ObjectType(*Source);
                    Source->~ObjectDestructorType();
                    --Count;
                }
            }
            else
            {
                while (Count)
                {
                    new(CurrentObject) ObjectType(*Source);
                    Source->~ObjectDestructorType();
                    ++CurrentObject;
                    ++Source;
                    --Count;
                }
            }
        }
    }
}


template<typename ObjectType, typename SizeType>
FORCEINLINE bool CompareObjects(const ObjectType* LHS, const ObjectType* RHS, SizeType Count) noexcept
{
    if constexpr (TIsTrivial<ObjectType>::Value)
    {
        return FMemory::Memcmp(LHS, RHS, sizeof(ObjectType) * Count) == 0;
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


template<typename ObjectType, typename SizeType>
FORCEINLINE void AssignObjects(ObjectType* RESTRICT Dst, const ObjectType& Element, SizeType Count) noexcept
{
    constexpr bool bObjectFitInByte = sizeof(ObjectType) == sizeof(uint8);
    if constexpr (TIsTrivial<ObjectType>::Value && bObjectFitInByte)
    {
        FMemory::Memset(Dst, static_cast<uint8>(Element), sizeof(ObjectType) * Count);
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


template<typename ObjectType, typename SizeType>
FORCEINLINE ObjectType* AssignObjectsAndReturn(ObjectType* RESTRICT Dst, const ObjectType& Element, SizeType Count) noexcept
{
    constexpr bool bObjectFitInByte = sizeof(ObjectType) == sizeof(uint8);
    if constexpr(TIsTrivial<ObjectType>::Value && bObjectFitInByte)
    {
        return reinterpret_cast<ObjectType*>(FMemory::Memset(Dst, static_cast<uint8>(Element), sizeof(ObjectType) * Count));
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
