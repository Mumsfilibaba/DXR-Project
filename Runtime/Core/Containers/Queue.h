#pragma once
#include "Core/CoreTypes.h"
#include "Core/Platform/PlatformInterlocked.h"
#include "Core/Templates/Move.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EQueueType 

enum class EQueueType
{
    Lockfree,
    Standard,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TQueue 

template<typename T, EQueueType QueueMode = EQueueType::Standard>
class TQueue
{
public:
    using ElementType = T;

private:
    struct FNode
    {
        FNode()
            : NextNode(nullptr)
        { }

        explicit FNode(const ElementType& InItem)
            : NextNode(nullptr)
            , Item(InItem)
        { }

        explicit FNode(ElementType&& InItem)
            : NextNode(nullptr)
            , Item(::Move(InItem))
        { }

        FNode* volatile NextNode;
        ElementType     Item;
    };

public:

    TQueue(const TQueue&)            = delete;
    TQueue& operator=(const TQueue&) = delete;

    FORCEINLINE TQueue()
        : Head(nullptr)
        , Tail(nullptr)
    {
        Head = new FNode();
        Tail = Head;
    }

    FORCEINLINE ~TQueue()
    {
        while (Tail != nullptr)
        {
            FNode* Node = Tail;
            Tail = Tail->NextNode;
            delete Node;
        }
    }

    FORCEINLINE bool Pop(ElementType& OutElement)
    {
        FNode* Popped = Tail->NextNode;
        if (Popped == nullptr)
        {
            return false;
        }

        OutElement = ::Move(Popped->Item);

        FNode* PreviousTail = Tail;
        Tail       = Popped;
        Tail->Item = ElementType();
        delete PreviousTail;

        return true;
    }

    FORCEINLINE bool Pop()
    {
        FNode* Popped = Tail->NextNode;
        if (Popped == nullptr)
        {
            return false;
        }

        FNode* OldTail = Tail;
        Tail       = Popped;
        Tail->Item = ElementType();
        delete OldTail;

        return true;
    }

    FORCEINLINE void Clear()
    {
        while (Pop());
    }

    FORCEINLINE bool Push(const ElementType& Item)
    {
        return Emplace(Item);
    }

    FORCEINLINE bool Push(ElementType&& Item)
    {
        return Emplace(Item);
    }

    template<typename... ArgTypes>
    FORCEINLINE bool Emplace(ArgTypes&&... Args) noexcept
    {
        FNode* NewNode = new FNode(::Forward<ArgTypes>(Args)...);
        if (NewNode == nullptr)
        {
            return false;
        }

        FNode* PreviousHead;
        if constexpr (QueueMode == EQueueType::Lockfree)
        {
            PreviousHead = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void**>(&Head), NewNode));
            FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void**>(&PreviousHead->NextNode), NewNode);
        }
        else
        {
            PreviousHead = Head;
            Head         = NewNode;
            PreviousHead->NextNode = NewNode;
        }

        return true;
    }

    FORCEINLINE bool IsEmpty() const
    {
        return (Tail->NextNode == nullptr);
    }

    FORCEINLINE bool Peek(ElementType& OutItem) const
    {
        if (Tail->NextNode == nullptr)
        {
            return false;
        }

        OutItem = Tail->NextNode->Item;
        return true;
    }

    FORCEINLINE ElementType* Peek()
    {
        if (Tail->NextNode == nullptr)
        {
            return nullptr;
        }

        return AddressOf(Tail->NextNode->Item);
    }

    FORCEINLINE const ElementType* Peek() const
    {
        if (Tail->NextNode == nullptr)
        {
            return nullptr;
        }

        return AddressOf(Tail->NextNode->Item);
    }

private:
    FNode* volatile Head;
    FNode*          Tail;
};
