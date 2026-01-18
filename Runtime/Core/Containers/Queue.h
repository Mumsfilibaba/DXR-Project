#pragma once
#include "Core/Containers/Array.h"
#include "Core/Templates/Utility.h"
#include "Core/Threading/Atomic.h"
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
        bool                           bHasItem;
    };

public:
    TQueue(const TQueue&) = delete;
    TQueue& operator=(const TQueue&) = delete;

    /**
     * @brief Constructor
     */
    TQueue()
    {
        // Create a dummy node to simplify edge cases.
        Head = CreateDummyNode();
        Tail = Head;
        NumElements = 0;
    }

    /**
     * @brief Destructor
     */
    ~TQueue()
    {
        // Drain all nodes including the final dummy.
        while (Tail != nullptr)
        {
            FNode* Node = Tail;
            Tail = Tail->NextNode;
            DeleteNode(Node);
        }

        Head = nullptr;
        NumElements = 0;
    }

    /**
     * @brief Pop the next element
     * @param OutElement Storage for the popped element
     * @return Returns true if an element was popped
     */
    bool Dequeue(ElementType& OutElement)
    {
        FNode* NextNode;
        if constexpr (QueueType == EQueueType::SPMC)
        {
            NextNode = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Tail->NextNode), nullptr));
        }
        else
        {
            NextNode = Tail->NextNode;
        }

        // Empty queue
        if (NextNode == nullptr)
        {
            return false;
        }

        CHECK(NextNode->bHasItem);

        // Move out the item
        OutElement = Move(*reinterpret_cast<ElementType*>(NextNode->Item.Data));

        // Destruct the item and make this node the new dummy tail.
        DestroyItemInNode(NextNode);

        // Advance tail
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
     * @brief Pop the next element
     * @return Returns true if an element was popped
     */
    bool Dequeue()
    {
        FNode* NextNode;
        if constexpr (QueueType == EQueueType::SPMC)
        {
            NextNode = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Tail->NextNode), nullptr));
        }
        else
        {
            NextNode = Tail->NextNode;
        }

        // Empty queue
        if (NextNode == nullptr)
        {
            return false;
        }

        CHECK(NextNode->bHasItem);

        // Destruct the item and make this node the new dummy tail.
        DestroyItemInNode(NextNode);

        // Advance tail
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
     * @brief Pops all the elements in the queue and puts them into the array
     * @param OutArray Array to store all the elements in
     */
    void DequeueAll(TArray<ElementType>& OutArray)
    {
        FNode* TailToDequeue;
        if constexpr (QueueType != EQueueType::SPSC)
        {
            // Detach producer head from consumer tail (keep dummy tail).
            FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Head), Tail);
            TailToDequeue = reinterpret_cast<FNode*>(FPlatformInterlocked::InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&Tail->NextNode), nullptr));
        }
        else
        {
            Head           = Tail;
            TailToDequeue  = Tail->NextNode;
            Tail->NextNode = nullptr;
        }

        // Snapshot element count
        int32 LocalNumElements = NumElements.Load();
        NumElements = 0;
        OutArray.Reserve(LocalNumElements);

        // Drain detached list
        FNode* Current = TailToDequeue;
        while (Current)
        {
            CHECK(Current->bHasItem);
            OutArray.Add(Move(*reinterpret_cast<ElementType*>(Current->Item.Data)));
            FNode* Next = Current->NextNode;
            DeleteNode(Current);
            Current = Next;
        }
    }

    /**
     * @brief Clears the queue
     */
    void Clear()
    {
        while (Dequeue())
        {
        }

        NumElements = 0;
    }

    /**
     * @brief Add an element to the back of the queue
     * @param Item The new element to push
     * @return Returns true if the element was successfully pushed
     */
    bool Enqueue(const ElementType& Item)
    {
        return Emplace(Item);
    }

    /**
     * @brief Add an element to the back of the queue
     * @param Item The new element to push
     * @return Returns true if the element was successfully pushed
     */
    bool Enqueue(ElementType&& Item)
    {
        return Emplace(Forward<ElementType>(Item));
    }

    /**
     * @brief Add an element to the back of the queue
     * @param Args Arguments used to construct the new element in-place
     * @return Returns true if the element was successfully pushed
     */
    template<typename... ArgTypes>
    bool Emplace(ArgTypes&&... Args)
    {
        FNode* NewNode = CreateNode(Forward<ArgTypes>(Args)...);
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
     * @return Returns true if the queue is empty
     */
    bool IsEmpty() const
    {
        return NumElements.Load() == 0;
    }

    /**
     * @return Returns the number of elements in the queue
     */
    int32 Size() const
    {
        return NumElements.Load();
    }

    /**
     * @brief Peek at the first element of the queue without popping from the queue
     * @param OutItem Storage for the item to peek
     * @return Returns true if the element was successfully stored
     */
    bool Peek(ElementType& OutItem) const
    {
        FNode* Next = Tail->NextNode;
        if (Next == nullptr)
        {
            return false;
        }

        CHECK(Next->bHasItem);
        OutItem = *reinterpret_cast<const ElementType*>(Next->Item.Data);
        return true;
    }

    /**
     * @return Returns a pointer to the first element in the queue, nullptr if the queue is empty
     */
    ElementType* Peek()
    {
        FNode* Next = Tail->NextNode;
        if (Next == nullptr)
        {
            return nullptr;
        }

        CHECK(Next->bHasItem);
        return reinterpret_cast<ElementType*>(Next->Item.Data);
    }

    /**
     * @return Returns a pointer to the first element in the queue, nullptr if the queue is empty
     */
    const ElementType* Peek() const
    {
        FNode* Next = Tail->NextNode;
        if (Next == nullptr)
        {
            return nullptr;
        }

        CHECK(Next->bHasItem);
        return reinterpret_cast<const ElementType*>(Next->Item.Data);
    }

private:

    FORCEINLINE FNode* CreateDummyNode()
    {
        FNode* Result = new FNode();
        Result->NextNode = nullptr;
        Result->bHasItem = false;
        return Result;
    }

    template<typename... ArgTypes>
    FNode* CreateNode(ArgTypes&&... Args)
    {
        FNode* Result = new FNode();
        Result->NextNode = nullptr;
        Result->bHasItem = true;
        new(reinterpret_cast<void*>(Result->Item.Data)) ElementType(Forward<ArgTypes>(Args)...);
        return Result;
    }

    FORCEINLINE void DestroyItemInNode(FNode* Node)
    {
        if (Node->bHasItem)
        {
            typedef ElementType ElementDestructType;
            reinterpret_cast<ElementDestructType*>(Node->Item.Data)->~ElementDestructType();
            Node->bHasItem = false;
        }
    }

    FORCEINLINE void DeleteNode(FNode* Node)
    {
        DestroyItemInNode(Node);
        delete Node;
    }

private:
    FNode* volatile Head{nullptr};
    FNode* volatile Tail{nullptr};
    FAtomicInt32    NumElements;
};
