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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Linked list which can go in forward

template<typename T>
struct TLinkedListNode
{
    FORCEINLINE TLinkedListNode() noexcept
        : Next(nullptr)
        , Item()
    {
    }

    template<typename... ArgTypes>
    FORCEINLINE TLinkedListNode(ArgTypes&&... Args) noexcept
        : Next(nullptr)
        , Item(Forward<ArgTypes>(Args)...)
    {
    }

    TLinkedListNode* Next;
    T Item;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Double-Linked list which can go in forward and backwards

template<typename T>
struct TDoubleLinkedListNode
{
    FORCEINLINE TDoubleLinkedListNode() noexcept
        : Next(nullptr)
        , Previous(nullptr)
        , Item()
    {
    }

    template<typename... ArgTypes>
    FORCEINLINE TDoubleLinkedListNode(ArgTypes&&... Args) noexcept
        : Next(nullptr)
        , Item(Forward<ArgTypes>(Args)...)
    {
    }

    TDoubleLinkedListNode* Next;
    TDoubleLinkedListNode* Previous;
    T Item;
};

#endif