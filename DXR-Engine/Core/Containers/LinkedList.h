#pragma once
#include "Core/Templates/Move.h"

template<typename T>
struct TLinkedListNode
{
    FORCEINLINE TLinkedListNode() noexcept
        : Next( nullptr )
        , Item()
    {
    }

    template<typename... ArgTypes>
    FORCEINLINE TLinkedListNode( ArgTypes&&... Args ) noexcept
        : Next( nullptr )
        , Item( Forward<ArgTypes>( Args )... )
    {
    }

    TLinkedListNode* Next;
    T Item;
};

template<typename T>
struct TDoubleLinkedListNode
{
    FORCEINLINE TDoubleLinkedListNode() noexcept
        : Next( nullptr )
        , Previous( nullptr )
        , Item()
    {
    }

    template<typename... ArgTypes>
    FORCEINLINE TDoubleLinkedListNode( ArgTypes&&... Args ) noexcept
        : Next( nullptr )
        , Item( Forward<ArgTypes>( Args )... )
    {
    }

    TDoubleLinkedListNode* Next;
    TDoubleLinkedListNode* Previous;
    T Item;
};
