#include "MathTest.h"

#include <Core/Math/IntVector2.h>

bool TestInt16Vector2()
{
    TEST_BEGIN();

    const int16 Five = 5;
    const int16 One  = 1;
    const int16 Two  = 2;

    TEST_SECTION(TEXT("FInt16Vector2::FInt16Vector2 (constructors)"));
    TEST_EXPECT(FInt16Vector2() == FInt16Vector2(0, 0));
    TEST_EXPECT(FInt16Vector2(int16(-3)) == FInt16Vector2(-3, -3));

    TEST_SECTION(TEXT("FInt16Vector2::Min / Max / Clamp"));
    TEST_EXPECT(FInt16Vector2::Min(FInt16Vector2(5, -7), FInt16Vector2(int16(-3))) == FInt16Vector2(-3, -7));
    TEST_EXPECT(FInt16Vector2::Max(FInt16Vector2(5, -7), FInt16Vector2(int16(-3))) == FInt16Vector2(5, -3));
    TEST_EXPECT(FInt16Vector2::Clamp(FInt16Vector2(-5, 8), FInt16Vector2(int16(-2)), FInt16Vector2(int16(5))) == FInt16Vector2(-2, 5));

    TEST_SECTION(TEXT("FInt16Vector2::operator- (unary)"));
    TEST_EXPECT(-FInt16Vector2(1, 2) == FInt16Vector2(-1, -2));

    TEST_SECTION(TEXT("FInt16Vector2::operator+ / operator+="));
    TEST_EXPECT(FInt16Vector2(1, 2) + FInt16Vector2(3, 1) == FInt16Vector2(4, 3));
    TEST_EXPECT(FInt16Vector2(1, 2) + Five == FInt16Vector2(6, 7));
    TEST_EXPECT(Five + FInt16Vector2(1, 2) == FInt16Vector2(6, 7));
    {
        FInt16Vector2 Vec(1, 2);
        Vec += FInt16Vector2(1, 1);
        Vec += One;

        TEST_EXPECT(Vec == FInt16Vector2(3, 4));
    }

    TEST_SECTION(TEXT("FInt16Vector2::operator- / operator-="));
    TEST_EXPECT(FInt16Vector2(4, 3) - FInt16Vector2(3, 1) == FInt16Vector2(1, 2));
    TEST_EXPECT(FInt16Vector2(4, 3) - Five == FInt16Vector2(-1, -2));
    TEST_EXPECT(Five - FInt16Vector2(1, 2) == FInt16Vector2(4, 3));
    {
        FInt16Vector2 Vec(4, 3);
        Vec -= FInt16Vector2(1, 1);
        Vec -= One;

        TEST_EXPECT(Vec == FInt16Vector2(2, 1));
    }

    TEST_SECTION(TEXT("FInt16Vector2::operator* / operator*="));
    TEST_EXPECT(FInt16Vector2(1, 2) * FInt16Vector2(3, 1) == FInt16Vector2(3, 2));
    TEST_EXPECT(FInt16Vector2(1, 2) * Five == FInt16Vector2(5, 10));
    TEST_EXPECT(Five * FInt16Vector2(1, 2) == FInt16Vector2(5, 10));
    {
        FInt16Vector2 Vec(1, 2);
        Vec *= FInt16Vector2(2, 2);
        Vec *= Two;

        TEST_EXPECT(Vec == FInt16Vector2(4, 8));
    }

    TEST_SECTION(TEXT("FInt16Vector2::operator/ / operator/="));
    TEST_EXPECT(FInt16Vector2(6, 8) / FInt16Vector2(3, 4) == FInt16Vector2(2, 2));
    TEST_EXPECT(FInt16Vector2(10, 20) / Five == FInt16Vector2(2, 4));
    TEST_EXPECT(int16(12) / FInt16Vector2(3, 4) == FInt16Vector2(4, 3));
    {
        FInt16Vector2 Vec(8, 4);
        Vec /= FInt16Vector2(2, 2);
        Vec /= Two;

        TEST_EXPECT(Vec == FInt16Vector2(2, 1));
    }

    TEST_SECTION(TEXT("FInt16Vector2::operator== / operator!="));
    TEST_EXPECT(FInt16Vector2(1, 2) == FInt16Vector2(1, 2));
    TEST_EXPECT(FInt16Vector2(1, 2) != FInt16Vector2(1, 3));

    TEST_SECTION(TEXT("FInt16Vector2::operator[]"));
    {
        FInt16Vector2 Vec(1, 2);
        Vec[0] = 5;

        TEST_EXPECT(Vec[0] == 5);
        TEST_EXPECT(Vec[1] == 2);
    }

    TEST_END();
}
