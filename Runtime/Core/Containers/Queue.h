#pragma once
#include "Core/Containers/Array.h"
#include "Core/Templates/Utility.h"
#include "Core/Threading/Atomic.h"
#include "Core/Threading/HazardPointer.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Platform/PlatformAtomic.h"

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
        TAtomicPointer<FNode*>         NextNode;
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
        : Head(nullptr)
        , Tail(nullptr)
        , NumElements(0)
    {
        // Create a dummy node to simplify edge cases.
        FNode* DummyNode = CreateDummyNode();
        Head.Store(DummyNode, EMemoryOrder::Relaxed);
        Tail.Store(DummyNode, EMemoryOrder::Relaxed);
    }

    /**
     * @brief Destructor
     */
    ~TQueue()
    {
        // Drain all nodes including the final dummy.
        FNode* Node = Tail.Load(EMemoryOrder::Relaxed);
        while (Node != nullptr)
        {
            FNode* NextNode = Node->NextNode.Load(EMemoryOrder::Relaxed);
            DeleteNode(Node);
            Node = NextNode;
        }

        Head.Store(nullptr, EMemoryOrder::Relaxed);
        Tail.Store(nullptr, EMemoryOrder::Relaxed);
        NumElements.Store(0);

        if constexpr (QueueType == EQueueType::SPMC)
        {
            // Nodes retired by consumers may still be pending reclamation, including any left
            // behind by consumer threads that have already exited.
            FHazardPointerDomain::Get().Collect();
        }
    }

    /**
     * @brief Pop the next element
     * @param OutElement Storage for the popped element
     * @return Returns true if an element was popped
     */
    bool Dequeue(ElementType& OutElement)
    {
        return DequeueInternal(&OutElement);
    }

    /**
     * @brief Pop the next element
     * @return Returns true if an element was popped
     */
    bool Dequeue()
    {
        return DequeueInternal(nullptr);
    }

    /**
     * @brief Pops all the elements in the queue and puts them into the array
     * @param OutArray Array to store all the elements in
     */
    void DequeueAll(TArray<ElementType>& OutArray)
    {
        // Bounded by the count observed on entry so a producer running alongside this cannot keep
        // the call spinning. Draining through Dequeue keeps the head private to the producer and
        // inherits whichever reclamation the queue type uses.
        int32 NumToDequeue = NumElements.Load();
        OutArray.Reserve(OutArray.Size() + NumToDequeue);

        ElementType Item;
        while (NumToDequeue > 0 && Dequeue(Item))
        {
            OutArray.Add(Move(Item));
            --NumToDequeue;
        }
    }

    /**
     * @brief Clears the queue
     */
    void Clear()
    {
        while (Dequeue())
        {
            // Dequeue until empty
        }

        NumElements.Store(0);
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
            PreviousHead = Head.Exchange(NewNode);
        }
        else
        {
            // Only the single producer ever touches the head.
            PreviousHead = Head.Load(EMemoryOrder::Relaxed);
            Head.Store(NewNode, EMemoryOrder::Relaxed);
        }

        // Publishes the item constructed in CreateNode along with the link. PreviousHead cannot
        // have been retired: a node is only retired once its link is non-null, and this store is
        // what makes it non-null.
        PreviousHead->NextNode.Store(NewNode, EMemoryOrder::Release);
        NumElements.Increment();
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
        StaticAssertPeekIsSafe();

        FNode* Next = Tail.Load(EMemoryOrder::Relaxed)->NextNode.Load(EMemoryOrder::Acquire);
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
        StaticAssertPeekIsSafe();

        FNode* Next = Tail.Load(EMemoryOrder::Relaxed)->NextNode.Load(EMemoryOrder::Acquire);
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
        StaticAssertPeekIsSafe();

        FNode* Next = Tail.Load(EMemoryOrder::Relaxed)->NextNode.Load(EMemoryOrder::Acquire);
        if (Next == nullptr)
        {
            return nullptr;
        }

        CHECK(Next->bHasItem);
        return reinterpret_cast<const ElementType*>(Next->Item.Data);
    }

private:
    static void StaticAssertPeekIsSafe()
    {
        static_assert(QueueType != EQueueType::SPMC, "Peek is unsafe on an SPMC queue, another consumer can destroy the item while it is read. Use Dequeue instead.");
    }

    FORCEINLINE bool DequeueInternal(ElementType* OutElement)
    {
        if constexpr (QueueType == EQueueType::SPMC)
        {
            return DequeueMultiConsumer(OutElement);
        }
        else
        {
            return DequeueSingleConsumer(OutElement);
        }
    }

    bool DequeueSingleConsumer(ElementType* OutElement)
    {
        FNode* PreviousTail = Tail.Load(EMemoryOrder::Relaxed);
        FNode* NextNode     = PreviousTail->NextNode.Load(EMemoryOrder::Acquire);

        // Empty queue
        if (NextNode == nullptr)
        {
            return false;
        }

        CHECK(NextNode->bHasItem);

        if (OutElement != nullptr)
        {
            *OutElement = Move(*reinterpret_cast<ElementType*>(NextNode->Item.Data));
        }

        // Destruct the item and make this node the new dummy tail.
        DestroyItemInNode(NextNode);
        Tail.Store(NextNode, EMemoryOrder::Relaxed);
        NumElements.Decrement();

        DeleteNode(PreviousTail);
        return true;
    }

    bool DequeueMultiConsumer(ElementType* OutElement)
    {
        FHazardPointerGuard Guard;
        for (;;)
        {
            FNode* PreviousTail = Tail.Load(EMemoryOrder::Acquire);
            Guard.Protect(0, PreviousTail);

            // Only safe to dereference once the hazard is published and the tail still names the
            // node, otherwise another consumer may already have retired it.
            if (Tail.Load(EMemoryOrder::Acquire) != PreviousTail)
            {
                continue;
            }

            FNode* NextNode = PreviousTail->NextNode.Load(EMemoryOrder::Acquire);
            if (NextNode == nullptr)
            {
                return false;
            }

            // Published before the claim, so whichever consumer eventually retires this node
            // cannot free it while the item is still being moved out below.
            Guard.Protect(1, NextNode);

            if (!Tail.CompareExchange(NextNode, PreviousTail))
            {
                continue;
            }

            CHECK(NextNode->bHasItem);

            if (OutElement != nullptr)
            {
                *OutElement = Move(*reinterpret_cast<ElementType*>(NextNode->Item.Data));
            }

            // Destruct the item, which this consumer now owns alone. NextNode has already become
            // the new dummy tail.
            DestroyItemInNode(NextNode);
            NumElements.Decrement();

            Guard.ClearAll();
            FHazardPointerDomain::Get().Retire(PreviousTail, &DeleteRetiredNode);
            return true;
        }
    }

    static void DeleteRetiredNode(void* Node)
    {
        FNode* NodeToDelete = static_cast<FNode*>(Node);
        CHECK(!NodeToDelete->bHasItem);
        delete NodeToDelete;
    }

    FORCEINLINE FNode* CreateDummyNode()
    {
        FNode* Result = new FNode();
        Result->NextNode.Store(nullptr, EMemoryOrder::Relaxed);
        Result->bHasItem = false;
        return Result;
    }

    template<typename... ArgTypes>
    FNode* CreateNode(ArgTypes&&... Args)
    {
        FNode* Result = new FNode();
        Result->NextNode.Store(nullptr, EMemoryOrder::Relaxed);
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
    TAtomicPointer<FNode*> Head;
    TAtomicPointer<FNode*> Tail;
    AtomicInt32            NumElements;
};
