#include "MathTest.h"

#include <Core/Math/Vector2.h>

bool TestVector2()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;

    TEST_SECTION("Vector2::Vector2 (constructors)");
    TEST_EXPECT(FVector2() == FVector2(0.0f, 0.0f));
    TEST_EXPECT(FVector2(3.0f) == FVector2(3.0f, 3.0f));
    TEST_EXPECT(FVector2(1.0f, 2.0f)[0] == 1.0f);

    TEST_SECTION("Vector2::DotProduct");
    TEST_EXPECT(FVector2(1.0f, 2.0f).DotProduct(FVector2(-3.0f, -3.0f)) == -9.0f);

    TEST_SECTION("Vector2::ProjectOn");
    TEST_EXPECT(FVector2(2.0f, 5.0f).ProjectOn(FVector2(1.0f, 0.0f)).IsEqual(FVector2(2.0f, 0.0f)));

    TEST_SECTION("Vector2::GetPerpendicular");
    TEST_EXPECT(FVector2(1.0f, 2.0f).GetPerpendicular() == FVector2(-2.0f, 1.0f));

    TEST_SECTION("Vector2::GetRotated");
    TEST_EXPECT(FVector2(1.0f, 0.0f).GetRotated(HalfPI).IsEqual(FVector2(0.0f, 1.0f)));

    TEST_SECTION("Vector2::GetDistanceTo / GetDistanceSquaredTo");
    TEST_EXPECT(Math::Abs(FVector2(0.0f, 0.0f).GetDistanceTo(FVector2(3.0f, 4.0f)) - 5.0f) <= 1.0e-4f);
    TEST_EXPECT(FVector2(0.0f, 0.0f).GetDistanceSquaredTo(FVector2(3.0f, 4.0f)) == 25.0f);

    TEST_SECTION("Vector2::GetAngleBetween");
    TEST_EXPECT(Math::Abs(FVector2(1.0f, 0.0f).GetAngleBetween(FVector2(0.0f, 1.0f)) - HalfPI) <= 1.0e-4f);

    TEST_SECTION("Vector2::GetReflected");
    TEST_EXPECT(FVector2(1.0f, -1.0f).GetReflected(FVector2(0.0f, 1.0f)).IsEqual(FVector2(1.0f, 1.0f)));

    TEST_SECTION("Vector2::IsEqual");
    TEST_EXPECT(FVector2(1.0f, 2.0f).IsEqual(FVector2(1.0f, 2.0f)));
    TEST_EXPECT(!FVector2(1.0f, 2.0f).IsEqual(FVector2(1.0f, 3.0f)));

    TEST_SECTION("Vector2::Min / Max");
    TEST_EXPECT(FVector2::Min(FVector2(5.0f, -7.0f), FVector2(-3.0f)) == FVector2(-3.0f, -7.0f));
    TEST_EXPECT(FVector2::Max(FVector2(5.0f, -7.0f), FVector2(-3.0f)) == FVector2(5.0f, -3.0f));

    TEST_SECTION("Vector2::Lerp");
    TEST_EXPECT(FVector2::Lerp(FVector2(0.0f), FVector2(1.0f), 0.5f) == FVector2(0.5f, 0.5f));

    TEST_SECTION("Vector2::Clamp");
    TEST_EXPECT(FVector2::Clamp(FVector2(-3.5f, 7.5f), FVector2(-2.0f), FVector2(5.0f)) == FVector2(-2.0f, 5.0f));

    TEST_SECTION("Vector2::Saturate");
    TEST_EXPECT(FVector2::Saturate(FVector2(-5.0f, 1.5f)) == FVector2(0.0f, 1.0f));

    TEST_SECTION("Vector2::RadiansToDegrees / DegreesToRadians");
    TEST_EXPECT(FVector2::RadiansToDegrees(FVector2(Math::Constants::PI, HalfPI)).IsEqual(FVector2(180.0f, 90.0f), 1.0e-2f));
    TEST_EXPECT(FVector2::DegreesToRadians(FVector2(180.0f, 90.0f)).IsEqual(FVector2(Math::Constants::PI, HalfPI)));

    TEST_SECTION("Vector2::Normalize / GetNormalized / IsUnitVector");
    {
        FVector2 Norm(1.0f);
        Norm.Normalize();

        TEST_EXPECT(Norm.IsEqual(FVector2(0.70710678f, 0.70710678f)));
        TEST_EXPECT(Norm.IsUnitVector());
        TEST_EXPECT(FVector2(1.0f).GetNormalized().IsUnitVector());
    }

    TEST_SECTION("Vector2::ContainsNaN / ContainsInfinity");
    {
        const FVector2 NaNVector(1.0f, Math::Constants::NaN);
        const FVector2 InfVector(1.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaNVector.ContainsNaN());
        TEST_EXPECT(InfVector.ContainsInfinity());
        TEST_EXPECT(!FVector2(1.0f, 2.0f).ContainsNaN());
        TEST_EXPECT(!FVector2(1.0f, 2.0f).ContainsInfinity());
    }

    TEST_SECTION("Vector2::GetLength / GetLengthSquared");
    TEST_EXPECT(Math::Abs(FVector2(2.0f, 2.0f).GetLength() - 2.82842712f) <= 1.0e-4f);
    TEST_EXPECT(FVector2(2.0f, 2.0f).GetLengthSquared() == 8.0f);

    TEST_SECTION("Vector2::operator- (unary)");
    TEST_EXPECT(-FVector2(1.0f, 2.0f) == FVector2(-1.0f, -2.0f));

    TEST_SECTION("Vector2::operator+ / operator+=");
    TEST_EXPECT(FVector2(1.0f, 2.0f) + FVector2(3.0f, 1.0f) == FVector2(4.0f, 3.0f));
    TEST_EXPECT(FVector2(1.0f, 2.0f) + 5.0f == FVector2(6.0f, 7.0f));
    {
        FVector2 Vec(1.0f, 2.0f);
        Vec += FVector2(1.0f, 1.0f);
        Vec += 1.0f;

        TEST_EXPECT(Vec == FVector2(3.0f, 4.0f));
    }

    TEST_SECTION("Vector2::operator- / operator-=");
    TEST_EXPECT(FVector2(4.0f, 3.0f) - FVector2(3.0f, 1.0f) == FVector2(1.0f, 2.0f));
    TEST_EXPECT(FVector2(4.0f, 3.0f) - 5.0f == FVector2(-1.0f, -2.0f));
    {
        FVector2 Vec(4.0f, 3.0f);
        Vec -= FVector2(1.0f, 1.0f);
        Vec -= 1.0f;

        TEST_EXPECT(Vec == FVector2(2.0f, 1.0f));
    }

    TEST_SECTION("Vector2::operator* / operator*=");
    TEST_EXPECT(FVector2(1.0f, 2.0f) * FVector2(3.0f, 1.0f) == FVector2(3.0f, 2.0f));
    TEST_EXPECT(FVector2(1.0f, 2.0f) * 5.0f == FVector2(5.0f, 10.0f));
    TEST_EXPECT(5.0f * FVector2(1.0f, 2.0f) == FVector2(5.0f, 10.0f));
    {
        FVector2 Vec(1.0f, 2.0f);
        Vec *= FVector2(2.0f, 2.0f);
        Vec *= 2.0f;

        TEST_EXPECT(Vec == FVector2(4.0f, 8.0f));
    }

    TEST_SECTION("Vector2::operator/ / operator/=");
    TEST_EXPECT(FVector2(3.0f, 2.0f) / FVector2(3.0f, 1.0f) == FVector2(1.0f, 2.0f));
    TEST_EXPECT(FVector2(5.0f, 10.0f) / 5.0f == FVector2(1.0f, 2.0f));
    {
        FVector2 Vec(8.0f, 4.0f);
        Vec /= FVector2(2.0f, 2.0f);
        Vec /= 2.0f;

        TEST_EXPECT(Vec == FVector2(2.0f, 1.0f));
    }

    TEST_SECTION("Vector2::operator== / operator!=");
    TEST_EXPECT(FVector2(1.0f, 2.0f) == FVector2(1.0f, 2.0f));
    TEST_EXPECT(FVector2(1.0f, 2.0f) != FVector2(1.0f, 3.0f));

    TEST_SECTION("Vector2::operator[]");
    {
        FVector2 Vec(1.0f, 2.0f);
        Vec[0] = 5.0f;

        TEST_EXPECT(Vec[0] == 5.0f);
        TEST_EXPECT(Vec[1] == 2.0f);
    }

    TEST_END();
}
