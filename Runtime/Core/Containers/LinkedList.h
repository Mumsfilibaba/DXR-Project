#pragma once
#include "Core/Templates/Utility.h"

template<typename T>
struct TLinkedListNode
{
    TLinkedListNode() = default;

    template<typename... ArgTypes>
    FORCEINLINE TLinkedListNode(ArgTypes&&... Args)
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
    TDoubleLinkedListNode() = default;

    template<typename... ArgTypes>
    FORCEINLINE TDoubleLinkedListNode(ArgTypes&&... Args)
        : Item(Forward<ArgTypes>(Args)...)
    {
    }

    TDoubleLinkedListNode* Next{nullptr};
    TDoubleLinkedListNode* Previous{nullptr};
    T Item;
};
