#pragma once
#include "Core/Containers/HashTable.h"
#include "Core/Utilities/HashUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TRHIStructHasher

template<typename StructureType>
struct TRHIStructHasher
{
    using ReturnType = size_t;

    ReturnType operator()(const StructureType& State) const noexcept
    {
        return static_cast<ReturnType>(State.GetHash());
    }
};

template<typename T, typename StructureType>
using TRHIStructHashTable = THashTable<StructureType, T, TRHIStructHasher<StructureType>>;
