#include "MathTest.h"

#include <Core/Math/IntVector2.h>

bool TestIntPoint2()
{
    TEST_BEGIN();

    TEST_SECTION("IntVector2::IntVector2 (constructors)");
    TEST_EXPECT(IntVector2() == IntVector2(0, 0));
    TEST_EXPECT(IntVector2(-3) == IntVector2(-3, -3));

    TEST_SECTION("IntVector2::Min / Max / Clamp");
    TEST_EXPECT(IntVector2::Min(IntVector2(5, -7), IntVector2(-3)) == IntVector2(-3, -7));
    TEST_EXPECT(IntVector2::Max(IntVector2(5, -7), IntVector2(-3)) == IntVector2(5, -3));
    TEST_EXPECT(IntVector2::Clamp(IntVector2(-5, 8), IntVector2(-2), IntVector2(5)) == IntVector2(-2, 5));

    TEST_SECTION("IntVector2::operator- (unary)");
    TEST_EXPECT(-IntVector2(1, 2) == IntVector2(-1, -2));

    TEST_SECTION("IntVector2::operator+ / operator+=");
    TEST_EXPECT(IntVector2(1, 2) + IntVector2(3, 1) == IntVector2(4, 3));
    TEST_EXPECT(IntVector2(1, 2) + 5 == IntVector2(6, 7));
    TEST_EXPECT(5 + IntVector2(1, 2) == IntVector2(6, 7));
    {
        IntVector2 Vec(1, 2);
        Vec += IntVector2(1, 1);
        Vec += 1;

        TEST_EXPECT(Vec == IntVector2(3, 4));
    }

    TEST_SECTION("IntVector2::operator- / operator-=");
    TEST_EXPECT(IntVector2(4, 3) - IntVector2(3, 1) == IntVector2(1, 2));
    TEST_EXPECT(IntVector2(4, 3) - 5 == IntVector2(-1, -2));
    TEST_EXPECT(5 - IntVector2(1, 2) == IntVector2(4, 3));
    {
        IntVector2 Vec(4, 3);
        Vec -= IntVector2(1, 1);
        Vec -= 1;

        TEST_EXPECT(Vec == IntVector2(2, 1));
    }

    TEST_SECTION("IntVector2::operator* / operator*=");
    TEST_EXPECT(IntVector2(1, 2) * IntVector2(3, 1) == IntVector2(3, 2));
    TEST_EXPECT(IntVector2(1, 2) * 5 == IntVector2(5, 10));
    TEST_EXPECT(5 * IntVector2(1, 2) == IntVector2(5, 10));
    {
        IntVector2 Vec(1, 2);
        Vec *= IntVector2(2, 2);
        Vec *= 2;

        TEST_EXPECT(Vec == IntVector2(4, 8));
    }

    TEST_SECTION("IntVector2::operator/ / operator/=");
    TEST_EXPECT(IntVector2(6, 8) / IntVector2(3, 4) == IntVector2(2, 2));
    TEST_EXPECT(IntVector2(10, 20) / 5 == IntVector2(2, 4));
    TEST_EXPECT(12 / IntVector2(3, 4) == IntVector2(4, 3));
    {
        IntVector2 Vec(8, 4);
        Vec /= IntVector2(2, 2);
        Vec /= 2;

        TEST_EXPECT(Vec == IntVector2(2, 1));
    }

    TEST_SECTION("IntVector2::operator== / operator!=");
    TEST_EXPECT(IntVector2(1, 2) == IntVector2(1, 2));
    TEST_EXPECT(IntVector2(1, 2) != IntVector2(1, 3));

    TEST_SECTION("IntVector2::operator[]");
    {
        IntVector2 Vec(1, 2);
        Vec[0] = 5;

        TEST_EXPECT(Vec[0] == 5);
        TEST_EXPECT(Vec[1] == 2);
    }

    TEST_END();
}
