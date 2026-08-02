#include "MathTest.h"

#include <Core/Math/IntVector3.h>

bool TestInt16Vector3()
{
    TEST_BEGIN();

    const int16 Five = 5;
    const int16 One  = 1;
    const int16 Two  = 2;

    TEST_SECTION("Int16Vector3::Int16Vector3 (constructors)");
    TEST_EXPECT(Int16Vector3() == Int16Vector3(0, 0, 0));
    TEST_EXPECT(Int16Vector3(int16(-3)) == Int16Vector3(-3, -3, -3));

    TEST_SECTION("Int16Vector3::Min / Max / Clamp");
    TEST_EXPECT(Int16Vector3::Min(Int16Vector3(5, -7, 2), Int16Vector3(int16(-3))) == Int16Vector3(-3, -7, -3));
    TEST_EXPECT(Int16Vector3::Max(Int16Vector3(5, -7, 2), Int16Vector3(int16(-3))) == Int16Vector3(5, -3, 2));
    TEST_EXPECT(Int16Vector3::Clamp(Int16Vector3(-5, 8, 1), Int16Vector3(int16(-2)), Int16Vector3(int16(5))) == Int16Vector3(-2, 5, 1));

    TEST_SECTION("Int16Vector3::operator- (unary)");
    TEST_EXPECT(-Int16Vector3(1, 2, -4) == Int16Vector3(-1, -2, 4));

    TEST_SECTION("Int16Vector3::operator+ / operator+=");
    TEST_EXPECT(Int16Vector3(1, 2, 3) + Int16Vector3(3, 1, 2) == Int16Vector3(4, 3, 5));
    TEST_EXPECT(Int16Vector3(1, 2, 3) + Five == Int16Vector3(6, 7, 8));
    TEST_EXPECT(Five + Int16Vector3(1, 2, 3) == Int16Vector3(6, 7, 8));
    {
        Int16Vector3 Vec(1, 2, 3);
        Vec += Int16Vector3(1, 1, 1);
        Vec += One;

        TEST_EXPECT(Vec == Int16Vector3(3, 4, 5));
    }

    TEST_SECTION("Int16Vector3::operator- / operator-=");
    TEST_EXPECT(Int16Vector3(4, 3, 9) - Int16Vector3(3, 1, 6) == Int16Vector3(1, 2, 3));
    TEST_EXPECT(Int16Vector3(4, 3, 9) - Five == Int16Vector3(-1, -2, 4));
    TEST_EXPECT(Five - Int16Vector3(1, 2, 3) == Int16Vector3(4, 3, 2));
    {
        Int16Vector3 Vec(4, 3, 9);
        Vec -= Int16Vector3(1, 1, 1);
        Vec -= One;

        TEST_EXPECT(Vec == Int16Vector3(2, 1, 7));
    }

    TEST_SECTION("Int16Vector3::operator* / operator*=");
    TEST_EXPECT(Int16Vector3(1, 2, 3) * Int16Vector3(3, 1, 2) == Int16Vector3(3, 2, 6));
    TEST_EXPECT(Int16Vector3(1, 2, 3) * Five == Int16Vector3(5, 10, 15));
    TEST_EXPECT(Five * Int16Vector3(1, 2, 3) == Int16Vector3(5, 10, 15));
    {
        Int16Vector3 Vec(1, 2, 3);
        Vec *= Int16Vector3(2, 2, 2);
        Vec *= Two;

        TEST_EXPECT(Vec == Int16Vector3(4, 8, 12));
    }

    TEST_SECTION("Int16Vector3::operator/ / operator/=");
    TEST_EXPECT(Int16Vector3(6, 8, 9) / Int16Vector3(3, 4, 3) == Int16Vector3(2, 2, 3));
    TEST_EXPECT(Int16Vector3(10, 20, 15) / Five == Int16Vector3(2, 4, 3));
    TEST_EXPECT(int16(12) / Int16Vector3(3, 4, 6) == Int16Vector3(4, 3, 2));
    {
        Int16Vector3 Vec(8, 4, 2);
        Vec /= Int16Vector3(2, 2, 2);
        Vec /= Two;

        TEST_EXPECT(Vec == Int16Vector3(2, 1, 0));
    }

    TEST_SECTION("Int16Vector3::operator== / operator!=");
    TEST_EXPECT(Int16Vector3(1, 2, 3) == Int16Vector3(1, 2, 3));
    TEST_EXPECT(Int16Vector3(1, 2, 3) != Int16Vector3(1, 2, 4));

    TEST_SECTION("Int16Vector3::operator[]");
    {
        Int16Vector3 Vec(1, 2, 3);
        Vec[2] = 9;

        TEST_EXPECT(Vec[0] == 1);
        TEST_EXPECT(Vec[2] == 9);
    }

    TEST_END();
}
