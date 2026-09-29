#include "RingBuffer_Test.h"

#if RUN_RINGBUFFER_TEST
#include "TestUtils.h"

#include <Core/Containers/RingBuffer.h>
#include <Core/Containers/String.h>

bool RingBuffer_Test()
{
    TEST_BEGIN();

    TRingBuffer<int32> Buffer(3);

    TEST_SECTION("Until it is full a ring buffer is an ordinary queue, oldest first");
    Buffer.Emplace(1);
    Buffer.Emplace(2);
    TEST_EXPECT_EQ(Buffer.Size(), 2);
    TEST_EXPECT(!Buffer.IsFull());
    TEST_EXPECT_EQ(Buffer[0], 1);
    TEST_EXPECT_EQ(Buffer[1], 2);

    TEST_SECTION("Once full, every append drops the oldest element");
    Buffer.Emplace(3);
    Buffer.Emplace(4);
    Buffer.Emplace(5);
    TEST_EXPECT(Buffer.IsFull());
    TEST_EXPECT_EQ(Buffer.Size(), 3);
    TEST_EXPECT_EQ(Buffer[0], 3);
    TEST_EXPECT_EQ(Buffer[1], 4);
    TEST_EXPECT_EQ(Buffer[2], 5);

    TEST_SECTION("Growing keeps every element in order and makes room for more");
    Buffer.SetCapacity(5);
    Buffer.Emplace(6);
    TEST_EXPECT_EQ(Buffer.Size(), 4);
    TEST_EXPECT_EQ(Buffer[0], 3);
    TEST_EXPECT_EQ(Buffer[3], 6);

    TEST_SECTION("Shrinking keeps the newest elements");
    Buffer.SetCapacity(2);
    TEST_EXPECT_EQ(Buffer.Size(), 2);
    TEST_EXPECT_EQ(Buffer[0], 5);
    TEST_EXPECT_EQ(Buffer[1], 6);

    TEST_SECTION("Clearing empties it, and it fills again from the start");
    Buffer.Clear();
    TEST_EXPECT(Buffer.IsEmpty());
    Buffer.Emplace(7);
    TEST_EXPECT_EQ(Buffer[0], 7);

    TEST_SECTION("Non-trivial elements are overwritten in place rather than leaked");
    TRingBuffer<String> Strings(2);
    Strings.Emplace("Alpha");
    Strings.Emplace("Bravo");
    Strings.Emplace("Charlie");
    TEST_EXPECT(Strings[0] == "Bravo");
    TEST_EXPECT(Strings[1] == "Charlie");

    TEST_END();
}

#endif
