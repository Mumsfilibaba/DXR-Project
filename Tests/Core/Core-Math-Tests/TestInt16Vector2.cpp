#include "MathTest.h"

#include <Core/Math/IntVector2.h>

bool TestInt16Vector2()
{
    TEST_BEGIN();

    const int16 Five = 5;
    const int16 One  = 1;
    const int16 Two  = 2;

    TEST_SECTION("Int16Vector2::Int16Vector2 (constructors)");
    TEST_EXPECT(Int16Vector2() == Int16Vector2(0, 0));
    TEST_EXPECT(Int16Vector2(int16(-3)) == Int16Vector2(-3, -3));

    TEST_SECTION("Int16Vector2::Min / Max / Clamp");
    TEST_EXPECT(Int16Vector2::Min(Int16Vector2(5, -7), Int16Vector2(int16(-3))) == Int16Vector2(-3, -7));
    TEST_EXPECT(Int16Vector2::Max(Int16Vector2(5, -7), Int16Vector2(int16(-3))) == Int16Vector2(5, -3));
    TEST_EXPECT(Int16Vector2::Clamp(Int16Vector2(-5, 8), Int16Vector2(int16(-2)), Int16Vector2(int16(5))) == Int16Vector2(-2, 5));

    TEST_SECTION("Int16Vector2::operator- (unary)");
    TEST_EXPECT(-Int16Vector2(1, 2) == Int16Vector2(-1, -2));

    TEST_SECTION("Int16Vector2::operator+ / operator+=");
    TEST_EXPECT(Int16Vector2(1, 2) + Int16Vector2(3, 1) == Int16Vector2(4, 3));
    TEST_EXPECT(Int16Vector2(1, 2) + Five == Int16Vector2(6, 7));
    TEST_EXPECT(Five + Int16Vector2(1, 2) == Int16Vector2(6, 7));
    {
        Int16Vector2 Vec(1, 2);
        Vec += Int16Vector2(1, 1);
        Vec += One;

        TEST_EXPECT(Vec == Int16Vector2(3, 4));
    }

    TEST_SECTION("Int16Vector2::operator- / operator-=");
    TEST_EXPECT(Int16Vector2(4, 3) - Int16Vector2(3, 1) == Int16Vector2(1, 2));
    TEST_EXPECT(Int16Vector2(4, 3) - Five == Int16Vector2(-1, -2));
    TEST_EXPECT(Five - Int16Vector2(1, 2) == Int16Vector2(4, 3));
    {
        Int16Vector2 Vec(4, 3);
        Vec -= Int16Vector2(1, 1);
        Vec -= One;

        TEST_EXPECT(Vec == Int16Vector2(2, 1));
    }

    TEST_SECTION("Int16Vector2::operator* / operator*=");
    TEST_EXPECT(Int16Vector2(1, 2) * Int16Vector2(3, 1) == Int16Vector2(3, 2));
    TEST_EXPECT(Int16Vector2(1, 2) * Five == Int16Vector2(5, 10));
    TEST_EXPECT(Five * Int16Vector2(1, 2) == Int16Vector2(5, 10));
    {
        Int16Vector2 Vec(1, 2);
        Vec *= Int16Vector2(2, 2);
        Vec *= Two;

        TEST_EXPECT(Vec == Int16Vector2(4, 8));
    }

    TEST_SECTION("Int16Vector2::operator/ / operator/=");
    TEST_EXPECT(Int16Vector2(6, 8) / Int16Vector2(3, 4) == Int16Vector2(2, 2));
    TEST_EXPECT(Int16Vector2(10, 20) / Five == Int16Vector2(2, 4));
    TEST_EXPECT(int16(12) / Int16Vector2(3, 4) == Int16Vector2(4, 3));
    {
        Int16Vector2 Vec(8, 4);
        Vec /= Int16Vector2(2, 2);
        Vec /= Two;

        TEST_EXPECT(Vec == Int16Vector2(2, 1));
    }

    TEST_SECTION("Int16Vector2::operator== / operator!=");
    TEST_EXPECT(Int16Vector2(1, 2) == Int16Vector2(1, 2));
    TEST_EXPECT(Int16Vector2(1, 2) != Int16Vector2(1, 3));

    TEST_SECTION("Int16Vector2::operator[]");
    {
        Int16Vector2 Vec(1, 2);
        Vec[0] = 5;

        TEST_EXPECT(Vec[0] == 5);
        TEST_EXPECT(Vec[1] == 2);
    }

    TEST_END();
}
