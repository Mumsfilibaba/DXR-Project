#pragma once

#if 1
#include <list>
#include <forward_list>

template<typename T>
using TLinkedList = std::forward_list<T>;

template<typename T>
using TDoubleLinkedList = std::list<T>;

#else
#include "Core/Templates/Move.h"

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
        : Next(nullptr)
        , Item(Forward<ArgTypes>(Args)...)
    {
    }

    TDoubleLinkedListNode* Next{nullptr};
    TDoubleLinkedListNode* Previous{nullptr};
    T Item;
};

#endif