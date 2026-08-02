#include "LinkedList_Test.h"

#if RUN_TLINKEDLIST_TEST
#include "TestUtils.h"

#include <Core/Containers/LinkedList.h>

bool TLinkedList_Test()
{
    TEST_BEGIN();

    TEST_SECTION("TLinkedListNode construction / linking / traversal");
    {
        TLinkedListNode<int32> First(1);
        TLinkedListNode<int32> Second(2);
        TLinkedListNode<int32> Third(3);
        
        First.Next  = &Second;
        Second.Next = &Third;
        Third.Next  = nullptr;

        TEST_EXPECT(First.Item == 1);

        int32 Sum   = 0;
        int32 Count = 0;

        for (TLinkedListNode<int32>* It = &First; It != nullptr; It = It->Next)
        {
            Sum += It->Item;
            ++Count;
        }

        TEST_EXPECT_EQ(Count, 3);
        TEST_EXPECT_EQ(Sum, 6);
    }

    TEST_SECTION("TDoubleLinkedListNode forward / backward links");
    {
        TDoubleLinkedListNode<int32> First(10);
        TDoubleLinkedListNode<int32> Second(20);
        
        First.Next      = &Second;
        Second.Previous = &First;

        TEST_EXPECT(First.Next->Item == 20);
        TEST_EXPECT(Second.Previous->Item == 10);
        TEST_EXPECT(First.Previous == nullptr);
        TEST_EXPECT(Second.Next == nullptr);
    }

    TEST_SECTION("Heap-allocated node list with FInstanced (single destruction, no leaks)");
    {
        FInstanced::Reset();
        STRESS_SWEEP(TargetSize, Seed, Stress::DefaultSeedCount)
        {
            (void)Seed;

            // Build a list by prepending nodes; the head ends up at the last-inserted id.
            TLinkedListNode<FInstanced>* Head = nullptr;
            for (int32 Step = 0; Step < TargetSize; ++Step)
            {
                TLinkedListNode<FInstanced>* Node = new TLinkedListNode<FInstanced>(FInstanced(Step));
                Node->Next = Head;
                Head       = Node;
            }

            TEST_EXPECT(FInstanced::LiveCount() == TargetSize);

            int32 Expected = TargetSize - 1;
            bool  bOrdered = true;
            for (TLinkedListNode<FInstanced>* It = Head; It != nullptr; It = It->Next)
            {
                bOrdered = bOrdered && (It->Item.GetId() == Expected) && It->Item.IsPayloadValid();
                --Expected;
            }

            TEST_EXPECT(bOrdered);

            // Tear the list down; every node payload must be destroyed exactly once.
            while (Head != nullptr)
            {
                TLinkedListNode<FInstanced>* Next = Head->Next;
                delete Head;
                Head = Next;
            }

            TEST_EXPECT(FInstanced::LiveCount() == 0);
        }

        TEST_EXPECT(FInstanced::LiveCount() == 0);
    }

    TEST_END();
}
#endif
