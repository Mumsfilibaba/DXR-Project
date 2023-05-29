#pragma once
#include "Core/Templates/Utility.h"
#include "Core/Platform/PlatformInterlocked.h"

enum class EQueueType
{
    // Single Producer, Multiple Consumers
    SPMC,
    // Multiple Producers, Single Consumer
    MPSC,
    // Single Producer, Single Consumer
    SPSC,
};

template<typename ElementType, EQueueType QueueType = EQueueType::SPSC>
class TQueue
{
private:
    struct FNode
    {
        FNode* volatile                NextNode;
        TTypeAlignedBytes<ElementType> Item;
    };

public:
    TQueue(const TQueue&)            = delete;
    TQueue& operator=(const TQueue&) = delete;

    /**
     * @brief - Constructor
     */
    FORCEINLINE TQueue()
    {
        // Create a Node here to more easily handle edge-cases
        Head = new FNode();
        Tail = Head;
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TQueue()
    {
        while (Tail != nullptr)
        {
            FNode* Node = Tail;
            Tail = Tail->NextNode;

            // Call the destructor of the node since these elements are "constructed"
            typedef ElementType ElementDestructType;
            reinterpret_cast<ElementDestructType*>(Node->Item.Data)->~ElementDestructType();

            delete Node;
        }
    }

    /**
     * @brief            - Pop the next element
     * @param OutElement - Storage for the popped element
     * @return           - Returns true if an element was popped
     */
    FORCEINLINE bool Dequeue(ElementType& OutElement)
    {
        FNode* NextNode;
        if constexpr(QueueType == EQueueType::SPMC)
        {
            NextNode = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Tail->NextNode), nullptr));
        }
        else
        {
            NextNode = Tail->NextNode;
        }

        // We are empty
        if (NextNode == nullptr)
        {
            return false;
        }

        // Move the item and "reset" it
        OutElement = ::Move(*reinterpret_cast<ElementType*>(NextNode->Item.Data));
        
        // Set the next node
        FNode* PreviousTail;
        if constexpr (QueueType == EQueueType::SPMC)
        {
            PreviousTail = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Tail), NextNode));
        }
        else
        {
            PreviousTail = Tail;
            Tail = NextNode;
        }

        delete PreviousTail;
        return true;
    }

    /**
     * @brief  - Pop the next element
     * @return - Returns true if an element was popped
     */
    FORCEINLINE bool Dequeue()
    {
        FNode* NextNode;
        if constexpr(QueueType == EQueueType::SPMC)
        {
            NextNode = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Tail->NextNode), nullptr));
        }
        else
        {
            NextNode = Tail->NextNode;
        }

        // We are empty
        if (NextNode == nullptr)
        {
            return false;
        }
        
        FNode* PreviousTail;
        if constexpr (QueueType == EQueueType::SPMC)
        {
            PreviousTail = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Tail), NextNode));
        }
        else
        {
            PreviousTail = Tail;
            Tail = NextNode;
        }

        delete PreviousTail;
        return true;
    }

    /**
     * @brief - Clears the queue
     */
    FORCEINLINE void Clear()
    {
        while (Dequeue());
    }

    /**
     * @brief      - Add an element to the back of the queue
     * @param Item - The new element to push
     * @return     - Returns true if the element was successfully pushed
     */
    FORCEINLINE bool Enqueue(const ElementType& Item)
    {
        return Emplace(Item);
    }

    /**
     * @brief      - Add an element to the back of the queue
     * @param Item - The new element to push
     * @return     - Returns true if the element was successfully pushed
     */
    FORCEINLINE bool Enqueue(ElementType&& Item)
    {
        return Emplace(::Forward<ElementType>(Item));
    }

    /**
     * @brief      - Add an element to the back of the queue
     * @param Args - Arguments used to construct the new element in-place
     * @return     - Returns true if the element was successfully pushed
     */
    template<typename... ArgTypes>
    FORCEINLINE bool Emplace(ArgTypes&&... Args) noexcept
    {
        FNode* NewNode = CreateNode(::Forward<ArgTypes>(Args)...);
        if (NewNode == nullptr)
        {
            return false;
        }

        FNode* PreviousHead;
        if constexpr (QueueType == EQueueType::MPSC)
        {
            PreviousHead = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Head), NewNode));
            FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&PreviousHead->NextNode), NewNode);
        }
        else
        {
            PreviousHead = Head;
            Head = NewNode;
            PreviousHead->NextNode = NewNode;
        }

        return true;
    }

    /**
     * @return - Returns true if the queue is empty
     */
    FORCEINLINE bool IsEmpty() const
    {
        return (Tail->NextNode == nullptr);
    }

    /**
     * @brief         - Peek at the first element of the queue without popping from the queue
     * @param OutItem - Storage for the item to peek
     * @return        - Returns true if the element was successfully stored
     */
    FORCEINLINE bool Peek(ElementType& OutItem) const
    {
        if (Tail->NextNode == nullptr)
        {
            return false;
        }

        OutItem = *reinterpret_cast<ElementType*>(&Tail->NextNode->Item);
        return true;
    }

    /**
     * @return - Returns a pointer to the first element in the queue, nullptr if the queue is empty
     */
    FORCEINLINE ElementType* Peek()
    {
        if (Tail->NextNode == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<ElementType*>(&Tail->NextNode->Item);
    }

    /**
    * @return - Returns a pointer to the first element in the queue, nullptr if the queue is empty
    */
    FORCEINLINE const ElementType* Peek() const
    {
        if (Tail->NextNode == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<const ElementType*>(&Tail->NextNode->Item);
    }

private:
    template<typename... ArgTypes>
    FORCEINLINE FNode* CreateNode(ArgTypes&&... Args)
    {
        // Construct the new Item
        FNode* Result = new FNode();
        new(reinterpret_cast<void*>(Result->Item.Data)) ElementType(::Forward<ArgTypes>(Args)...);
        return Result;
    }

    FNode* volatile Head{nullptr};
    FNode* volatile Tail{nullptr};
};