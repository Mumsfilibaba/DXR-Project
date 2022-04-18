#pragma once
#include "Core/Containers/HashTable.h"
#include "Core/Utilities/HashUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStateHasher

template<typename StateDescType>
struct TStateHasher
{
    typename ReturnType = size_t;

    ReturnType operator()(const StateDescType& State) const noexcept
    {
        return static_cast<ReturnType>(State.GetHash());
    }
};

template<typename StateType, typename StateDescType>
using TStateHashTable = THashTable<StateDescType, StateType, TStateHasher<StateDescType>>;