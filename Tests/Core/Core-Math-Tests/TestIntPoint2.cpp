#include "MathTest.h"

#include <Core/Math/IntVector2.h>

bool TestIntPoint2()
{
    TEST_BEGIN();

    TEST_SECTION("IntVector2::IntVector2 (constructors)");
    TEST_EXPECT(FIntVector2() == FIntVector2(0, 0));
    TEST_EXPECT(FIntVector2(-3) == FIntVector2(-3, -3));

    TEST_SECTION("IntVector2::Min / Max / Clamp");
    TEST_EXPECT(FIntVector2::Min(FIntVector2(5, -7), FIntVector2(-3)) == FIntVector2(-3, -7));
    TEST_EXPECT(FIntVector2::Max(FIntVector2(5, -7), FIntVector2(-3)) == FIntVector2(5, -3));
    TEST_EXPECT(FIntVector2::Clamp(FIntVector2(-5, 8), FIntVector2(-2), FIntVector2(5)) == FIntVector2(-2, 5));

    TEST_SECTION("IntVector2::operator- (unary)");
    TEST_EXPECT(-FIntVector2(1, 2) == FIntVector2(-1, -2));

    TEST_SECTION("IntVector2::operator+ / operator+=");
    TEST_EXPECT(FIntVector2(1, 2) + FIntVector2(3, 1) == FIntVector2(4, 3));
    TEST_EXPECT(FIntVector2(1, 2) + 5 == FIntVector2(6, 7));
    TEST_EXPECT(5 + FIntVector2(1, 2) == FIntVector2(6, 7));
    {
        FIntVector2 Vec(1, 2);
        Vec += FIntVector2(1, 1);
        Vec += 1;

        TEST_EXPECT(Vec == FIntVector2(3, 4));
    }

    TEST_SECTION("IntVector2::operator- / operator-=");
    TEST_EXPECT(FIntVector2(4, 3) - FIntVector2(3, 1) == FIntVector2(1, 2));
    TEST_EXPECT(FIntVector2(4, 3) - 5 == FIntVector2(-1, -2));
    TEST_EXPECT(5 - FIntVector2(1, 2) == FIntVector2(4, 3));
    {
        FIntVector2 Vec(4, 3);
        Vec -= FIntVector2(1, 1);
        Vec -= 1;

        TEST_EXPECT(Vec == FIntVector2(2, 1));
    }

    TEST_SECTION("IntVector2::operator* / operator*=");
    TEST_EXPECT(FIntVector2(1, 2) * FIntVector2(3, 1) == FIntVector2(3, 2));
    TEST_EXPECT(FIntVector2(1, 2) * 5 == FIntVector2(5, 10));
    TEST_EXPECT(5 * FIntVector2(1, 2) == FIntVector2(5, 10));
    {
        FIntVector2 Vec(1, 2);
        Vec *= FIntVector2(2, 2);
        Vec *= 2;

        TEST_EXPECT(Vec == FIntVector2(4, 8));
    }

    TEST_SECTION("IntVector2::operator/ / operator/=");
    TEST_EXPECT(FIntVector2(6, 8) / FIntVector2(3, 4) == FIntVector2(2, 2));
    TEST_EXPECT(FIntVector2(10, 20) / 5 == FIntVector2(2, 4));
    TEST_EXPECT(12 / FIntVector2(3, 4) == FIntVector2(4, 3));
    {
        FIntVector2 Vec(8, 4);
        Vec /= FIntVector2(2, 2);
        Vec /= 2;

        TEST_EXPECT(Vec == FIntVector2(2, 1));
    }

    TEST_SECTION("IntVector2::operator== / operator!=");
    TEST_EXPECT(FIntVector2(1, 2) == FIntVector2(1, 2));
    TEST_EXPECT(FIntVector2(1, 2) != FIntVector2(1, 3));

    TEST_SECTION("IntVector2::operator[]");
    {
        FIntVector2 Vec(1, 2);
        Vec[0] = 5;

        TEST_EXPECT(Vec[0] == 5);
        TEST_EXPECT(Vec[1] == 2);
    }

    TEST_END();
}
