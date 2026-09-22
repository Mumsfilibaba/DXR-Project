#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/Utility/NonCopyable.h"

struct AlgorithmPrivate : public FNonConstructible
{
    template<typename PointerType>
    struct TElementFromPointer
    {
        using Type = typename TRemoveCV<typename TRemovePointer<typename TRemoveReference<PointerType>::Type>::Type>::Type;
    };

    template<typename ElementType>
    struct TLess
    {
        FORCEINLINE bool operator()(const ElementType& Left, const ElementType& Right) const
        {
            return Left < Right;
        }
    };

    NODISCARD static FORCEINLINE int32 FloorLog2(int32 Value)
    {
        int32 Log = 0;
        while (Value >= 2)
        {
            Value >>= 1;
            ++Log;
        }

        return Log;
    }

    template<typename ElementType, typename PredicateType>
    static FORCEINLINE void Sort3(ElementType& A, ElementType& B, ElementType& C, PredicateType&& Predicate)
    {
        if (Predicate(B, A))
        {
            ::Swap(A, B);
        }

        if (Predicate(C, B))
        {
            ::Swap(B, C);
            if (Predicate(B, A))
            {
                ::Swap(A, B);
            }
        }
    }

    template<typename ElementType>
    struct TScratchArray
    {
        ElementType* Data  = nullptr;
        int32        Count = 0;

        TScratchArray() = default;

        explicit TScratchArray(int32 InCount)
        {
            Reset(InCount);
        }

        ~TScratchArray()
        {
            Memory::Free(Data);
        }

        void Reset(int32 InCount)
        {
            Memory::Free(Data);

            Data  = nullptr;
            Count = InCount;

            if (InCount > 0)
            {
                Data = static_cast<ElementType*>(Memory::Malloc(static_cast<uint64>(InCount) * sizeof(ElementType)));
            }
        }

        TScratchArray(const TScratchArray&)            = delete;
        TScratchArray& operator=(const TScratchArray&) = delete;
    };

    template<typename KeyType>
    NODISCARD static FORCEINLINE auto ToUnsignedSortKey(KeyType Key)
    {
        if constexpr (sizeof(KeyType) <= 4)
        {
            uint32 Bits = static_cast<uint32>(Key);
            if constexpr (TIsSigned<KeyType>::Value)
            {
                Bits ^= 0x80000000u;
            }

            return Bits;
        }
        else
        {
            uint64 Bits = static_cast<uint64>(Key);
            if constexpr (TIsSigned<KeyType>::Value)
            {
                Bits ^= 0x8000000000000000ull;
            }

            return Bits;
        }
    }
};
