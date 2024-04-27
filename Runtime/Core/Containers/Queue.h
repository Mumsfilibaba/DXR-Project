#pragma once
#include "Array.h"
#include "Core/Templates/Utility.h"
#include "Core/Threading/AtomicInt.h"
#include "Core/Platform/PlatformMisc.h"
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
    TQueue()
    {
        // Create a Node here to more easily handle edge-cases
        Head = new FNode();
        Tail = Head;
    }

    /**
     * @brief - Destructor
     */
    ~TQueue()
    {
        while (Tail != nullptr)
        {
            FNode* Node = Tail;
            Tail = Tail->NextNode;
            DeleteNode(Node);
        }
    }

    /**
     * @brief            - Pop the next element
     * @param OutElement - Storage for the popped element
     * @return           - Returns true if an element was popped
     */
    bool Dequeue(ElementType& OutElement)
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

        DeleteNode(PreviousTail);
        NumElements--;
        return true;
    }

    /**
     * @brief  - Pop the next element
     * @return - Returns true if an element was popped
     */
    bool Dequeue()
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

        DeleteNode(PreviousTail);
        NumElements--;
        return true;
    }

    /**
     * @brief          - Pops all the elements in the queue and puts them into the array
     * @param OutArray - Array to store all the elements in
     */
    void DequeueAll(TArray<ElementType>& OutArray)
    {
        FNode* TailToDequeue;
        if constexpr (QueueType != EQueueType::SPSC)
        {
            FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Head), Tail);
            TailToDequeue = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Tail->NextNode), nullptr));
        }
        else
        {
            Head           = Tail;
            TailToDequeue  = Tail->NextNode;
            Tail->NextNode = nullptr;
        }
        
        // Count all the nodes
        int32 LocalNumElements = NumElements.Load();
        NumElements = 0;
        OutArray.Reserve(LocalNumElements);
        
        // Insert all the elements in the array
        FNode* CurrentTail  = TailToDequeue;
        FNode* PreviousTail = nullptr;
        while (CurrentTail)
        {
            OutArray.Add(::Move(*reinterpret_cast<ElementType*>(CurrentTail->Item.Data)));
            PreviousTail = CurrentTail;
            CurrentTail  = CurrentTail->NextNode;
            DeleteNode(PreviousTail);
        }
    }
    
    /**
     * @brief - Clears the queue
     */
    void Clear()
    {
        while (Dequeue());
        NumElements = 0;
    }

    /**
     * @brief      - Add an element to the back of the queue
     * @param Item - The new element to push
     * @return     - Returns true if the element was successfully pushed
     */
    bool Enqueue(const ElementType& Item)
    {
        return Emplace(Item);
    }

    /**
     * @brief      - Add an element to the back of the queue
     * @param Item - The new element to push
     * @return     - Returns true if the element was successfully pushed
     */
    bool Enqueue(ElementType&& Item)
    {
        return Emplace(::Forward<ElementType>(Item));
    }

    /**
     * @brief      - Add an element to the back of the queue
     * @param Args - Arguments used to construct the new element in-place
     * @return     - Returns true if the element was successfully pushed
     */
    template<typename... ArgTypes>
    bool Emplace(ArgTypes&&... Args) noexcept
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

        NumElements++;
        return true;
    }

    /**
     * @return - Returns true if the queue is empty
     */
    bool IsEmpty() const
    {
        return NumElements.Load() == 0;;
    }
    
    /**
     * @return - Returns the number of elements in the queue
     */
    int32 Size() const
    {
        return NumElements.Load();
    }

    /**
     * @brief         - Peek at the first element of the queue without popping from the queue
     * @param OutItem - Storage for the item to peek
     * @return        - Returns true if the element was successfully stored
     */
    bool Peek(ElementType& OutItem) const
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
    ElementType* Peek()
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
    const ElementType* Peek() const
    {
        if (Tail->NextNode == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<const ElementType*>(&Tail->NextNode->Item);
    }

private:
    template<typename... ArgTypes>
    FNode* CreateNode(ArgTypes&&... Args)
    {
        // Construct the new Item
        FNode* Result = new FNode();
        new(reinterpret_cast<void*>(Result->Item.Data)) ElementType(::Forward<ArgTypes>(Args)...);
        return Result;
    }

    void DeleteNode(FNode* Node)
    {
        // Call the destructor of the node since these elements are "constructed"
        typedef ElementType ElementDestructType;
        reinterpret_cast<ElementDestructType*>(Node->Item.Data)->~ElementDestructType();
        delete Node;
    }

    FNode* volatile Head{nullptr};
    FNode* volatile Tail{nullptr};
    FAtomicInt32    NumElements{0};
};
