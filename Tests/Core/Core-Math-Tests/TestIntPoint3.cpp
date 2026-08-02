#include "MathTest.h"

#include <Core/Math/IntVector3.h>

bool TestIntPoint3()
{
    TEST_BEGIN();

    TEST_SECTION("IntVector3::IntVector3 (constructors)");
    TEST_EXPECT(IntVector3() == IntVector3(0, 0, 0));
    TEST_EXPECT(IntVector3(-3) == IntVector3(-3, -3, -3));

    TEST_SECTION("IntVector3::Min / Max / Clamp");
    TEST_EXPECT(IntVector3::Min(IntVector3(5, -7, 2), IntVector3(-3)) == IntVector3(-3, -7, -3));
    TEST_EXPECT(IntVector3::Max(IntVector3(5, -7, 2), IntVector3(-3)) == IntVector3(5, -3, 2));
    TEST_EXPECT(IntVector3::Clamp(IntVector3(-5, 8, 1), IntVector3(-2), IntVector3(5)) == IntVector3(-2, 5, 1));

    TEST_SECTION("IntVector3::operator- (unary)");
    TEST_EXPECT(-IntVector3(1, 2, -4) == IntVector3(-1, -2, 4));

    TEST_SECTION("IntVector3::operator+ / operator+=");
    TEST_EXPECT(IntVector3(1, 2, 3) + IntVector3(3, 1, 2) == IntVector3(4, 3, 5));
    TEST_EXPECT(IntVector3(1, 2, 3) + 5 == IntVector3(6, 7, 8));
    TEST_EXPECT(5 + IntVector3(1, 2, 3) == IntVector3(6, 7, 8));
    {
        IntVector3 Vec(1, 2, 3);
        Vec += IntVector3(1, 1, 1);
        Vec += 1;

        TEST_EXPECT(Vec == IntVector3(3, 4, 5));
    }

    TEST_SECTION("IntVector3::operator- / operator-=");
    TEST_EXPECT(IntVector3(4, 3, 9) - IntVector3(3, 1, 6) == IntVector3(1, 2, 3));
    TEST_EXPECT(IntVector3(4, 3, 9) - 5 == IntVector3(-1, -2, 4));
    TEST_EXPECT(5 - IntVector3(1, 2, 3) == IntVector3(4, 3, 2));
    {
        IntVector3 Vec(4, 3, 9);
        Vec -= IntVector3(1, 1, 1);
        Vec -= 1;

        TEST_EXPECT(Vec == IntVector3(2, 1, 7));
    }

    TEST_SECTION("IntVector3::operator* / operator*=");
    TEST_EXPECT(IntVector3(1, 2, 3) * IntVector3(3, 1, 2) == IntVector3(3, 2, 6));
    TEST_EXPECT(IntVector3(1, 2, 3) * 5 == IntVector3(5, 10, 15));
    TEST_EXPECT(5 * IntVector3(1, 2, 3) == IntVector3(5, 10, 15));
    {
        IntVector3 Vec(1, 2, 3);
        Vec *= IntVector3(2, 2, 2);
        Vec *= 2;

        TEST_EXPECT(Vec == IntVector3(4, 8, 12));
    }

    TEST_SECTION("IntVector3::operator/ / operator/=");
    TEST_EXPECT(IntVector3(6, 8, 9) / IntVector3(3, 4, 3) == IntVector3(2, 2, 3));
    TEST_EXPECT(IntVector3(10, 20, 15) / 5 == IntVector3(2, 4, 3));
    TEST_EXPECT(12 / IntVector3(3, 4, 6) == IntVector3(4, 3, 2));
    {
        IntVector3 Vec(8, 4, 2);
        Vec /= IntVector3(2, 2, 2);
        Vec /= 2;

        TEST_EXPECT(Vec == IntVector3(2, 1, 0));
    }

    TEST_SECTION("IntVector3::operator== / operator!=");
    TEST_EXPECT(IntVector3(1, 2, 3) == IntVector3(1, 2, 3));
    TEST_EXPECT(IntVector3(1, 2, 3) != IntVector3(1, 2, 4));

    TEST_SECTION("IntVector3::operator[]");
    {
        IntVector3 Vec(1, 2, 3);
        Vec[2] = 9;

        TEST_EXPECT(Vec[0] == 1);
        TEST_EXPECT(Vec[2] == 9);
    }

    TEST_END();
}
