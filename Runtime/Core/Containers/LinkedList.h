#pragma once
#include "Core/Templates/Utility.h"

template<typename T>
struct TLinkedListNode
{
    TLinkedListNode() noexcept = default;

    template<typename... ArgTypes>
    FORCEINLINE TLinkedListNode(ArgTypes&&... Args) noexcept
        : Next(nullptr)
        , Item(Forward<ArgTypes>(Args)...)
    {
    }

    TLinkedListNode* Next{nullptr};
    T Item;
};

template<typename T>
struct TDoubleLinkedListNode
{
    TDoubleLinkedListNode() noexcept = default;

    template<typename... ArgTypes>
    FORCEINLINE TDoubleLinkedListNode(ArgTypes&&... Args) noexcept
        : Item(Forward<ArgTypes>(Args)...)
    {
    }

    TDoubleLinkedListNode* Next{nullptr};
    TDoubleLinkedListNode* Previous{nullptr};
    T Item;
};
