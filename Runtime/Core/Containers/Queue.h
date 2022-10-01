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

    /**
     * @breif: Constructor
     */
    FORCEINLINE TQueue()
        : Head(nullptr)
        , Tail(nullptr)
    {
        Head = new FNode();
        Tail = Head;
    }

    /**
     * @breif: Destructor
     */
    FORCEINLINE ~TQueue()
    {
        while (Tail != nullptr)
        {
            FNode* Node = Tail;
            Tail = Tail->NextNode;
            delete Node;
        }
    }

    /**
     * @breif: Pop the next element
     * 
     * @param OutElement: Storage for the popped element
     * @return: Returns true if an element was popped
     */
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

    /**
     * @breif: Pop the next element
     *
     * @return: Returns true if an element was popped
     */
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

    /**
     * @breif: Clears the queue
     */
    FORCEINLINE void Clear()
    {
        while (Pop());
    }

    /**
     * @breif: Push an element to the back of the queue 
     * 
     * @param Item: The new element to push
     * @return: Returns true if the element was successfully pushed
     */
    FORCEINLINE bool Push(const ElementType& Item)
    {
        return Emplace(Item);
    }

    /**
     * @breif: Push an element to the back of the queue 
     * 
     * @param Item: The new element to push
     * @return: Returns true if the element was successfully pushed
     */
    FORCEINLINE bool Push(ElementType&& Item)
    {
        return Emplace(::Forward<ElementType>(Item));
    }

    /**
     * @breif: Push an element to the back of the queue
     *
     * @param Args: Arguments used to construct the new element in-place
     * @return: Returns true if the element was successfully pushed
     */
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

    /** 
     * @return: Returns true if the queue is empty
     */
    FORCEINLINE bool IsEmpty() const
    {
        return (Tail->NextNode == nullptr);
    }

    /**
     * @breif: Peek at the first element of the queue without popping from the queue
     *
     * @param OutItem: Storage for the item to peek
     * @return: Returns true if the element was successfully stored
     */
    FORCEINLINE bool Peek(ElementType& OutItem) const
    {
        if (Tail->NextNode == nullptr)
        {
            return false;
        }

        OutItem = Tail->NextNode->Item;
        return true;
    }

    /**
     * @return: Returns a pointer to the first element in the queue, nullptr if the queue is empty
     */
    FORCEINLINE ElementType* Peek()
    {
        if (Tail->NextNode == nullptr)
        {
            return nullptr;
        }

        return ::AddressOf(Tail->NextNode->Item);
    }

    /**
    * @return: Returns a pointer to the first element in the queue, nullptr if the queue is empty
    */
    FORCEINLINE const ElementType* Peek() const
    {
        if (Tail->NextNode == nullptr)
        {
            return nullptr;
        }

        return ::AddressOf(Tail->NextNode->Item);
    }

private:
    FNode* volatile Head;
    FNode*          Tail;
};
