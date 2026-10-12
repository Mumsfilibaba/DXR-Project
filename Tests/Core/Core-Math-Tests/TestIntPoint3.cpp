#include "MathTest.h"

#include <Core/Math/IntVector3.h>

bool TestIntPoint3()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("FIntVector3::FIntVector3 (constructors)"));
    TEST_EXPECT(FIntVector3() == FIntVector3(0, 0, 0));
    TEST_EXPECT(FIntVector3(-3) == FIntVector3(-3, -3, -3));

    TEST_SECTION(TEXT("FIntVector3::Min / Max / Clamp"));
    TEST_EXPECT(FIntVector3::Min(FIntVector3(5, -7, 2), FIntVector3(-3)) == FIntVector3(-3, -7, -3));
    TEST_EXPECT(FIntVector3::Max(FIntVector3(5, -7, 2), FIntVector3(-3)) == FIntVector3(5, -3, 2));
    TEST_EXPECT(FIntVector3::Clamp(FIntVector3(-5, 8, 1), FIntVector3(-2), FIntVector3(5)) == FIntVector3(-2, 5, 1));

    TEST_SECTION(TEXT("FIntVector3::operator- (unary)"));
    TEST_EXPECT(-FIntVector3(1, 2, -4) == FIntVector3(-1, -2, 4));

    TEST_SECTION(TEXT("FIntVector3::operator+ / operator+="));
    TEST_EXPECT(FIntVector3(1, 2, 3) + FIntVector3(3, 1, 2) == FIntVector3(4, 3, 5));
    TEST_EXPECT(FIntVector3(1, 2, 3) + 5 == FIntVector3(6, 7, 8));
    TEST_EXPECT(5 + FIntVector3(1, 2, 3) == FIntVector3(6, 7, 8));
    {
        FIntVector3 Vec(1, 2, 3);
        Vec += FIntVector3(1, 1, 1);
        Vec += 1;

        TEST_EXPECT(Vec == FIntVector3(3, 4, 5));
    }

    TEST_SECTION(TEXT("FIntVector3::operator- / operator-="));
    TEST_EXPECT(FIntVector3(4, 3, 9) - FIntVector3(3, 1, 6) == FIntVector3(1, 2, 3));
    TEST_EXPECT(FIntVector3(4, 3, 9) - 5 == FIntVector3(-1, -2, 4));
    TEST_EXPECT(5 - FIntVector3(1, 2, 3) == FIntVector3(4, 3, 2));
    {
        FIntVector3 Vec(4, 3, 9);
        Vec -= FIntVector3(1, 1, 1);
        Vec -= 1;

        TEST_EXPECT(Vec == FIntVector3(2, 1, 7));
    }

    TEST_SECTION(TEXT("FIntVector3::operator* / operator*="));
    TEST_EXPECT(FIntVector3(1, 2, 3) * FIntVector3(3, 1, 2) == FIntVector3(3, 2, 6));
    TEST_EXPECT(FIntVector3(1, 2, 3) * 5 == FIntVector3(5, 10, 15));
    TEST_EXPECT(5 * FIntVector3(1, 2, 3) == FIntVector3(5, 10, 15));
    {
        FIntVector3 Vec(1, 2, 3);
        Vec *= FIntVector3(2, 2, 2);
        Vec *= 2;

        TEST_EXPECT(Vec == FIntVector3(4, 8, 12));
    }

    TEST_SECTION(TEXT("FIntVector3::operator/ / operator/="));
    TEST_EXPECT(FIntVector3(6, 8, 9) / FIntVector3(3, 4, 3) == FIntVector3(2, 2, 3));
    TEST_EXPECT(FIntVector3(10, 20, 15) / 5 == FIntVector3(2, 4, 3));
    TEST_EXPECT(12 / FIntVector3(3, 4, 6) == FIntVector3(4, 3, 2));
    {
        FIntVector3 Vec(8, 4, 2);
        Vec /= FIntVector3(2, 2, 2);
        Vec /= 2;

        TEST_EXPECT(Vec == FIntVector3(2, 1, 0));
    }

    TEST_SECTION(TEXT("FIntVector3::operator== / operator!="));
    TEST_EXPECT(FIntVector3(1, 2, 3) == FIntVector3(1, 2, 3));
    TEST_EXPECT(FIntVector3(1, 2, 3) != FIntVector3(1, 2, 4));

    TEST_SECTION(TEXT("FIntVector3::operator[]"));
    {
        FIntVector3 Vec(1, 2, 3);
        Vec[2] = 9;

        TEST_EXPECT(Vec[0] == 1);
        TEST_EXPECT(Vec[2] == 9);
    }

    TEST_END();
}
