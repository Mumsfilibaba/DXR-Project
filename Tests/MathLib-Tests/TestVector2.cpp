#include "MathTest.h"

#include <Core/Math/Vector2.h>

bool TestVector2()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;

    TEST_SECTION("Vector2::Vector2 (constructors)");
    TEST_EXPECT(Vector2() == Vector2(0.0f, 0.0f));
    TEST_EXPECT(Vector2(3.0f) == Vector2(3.0f, 3.0f));
    TEST_EXPECT(Vector2(1.0f, 2.0f)[0] == 1.0f);

    TEST_SECTION("Vector2::DotProduct");
    TEST_EXPECT(Vector2(1.0f, 2.0f).DotProduct(Vector2(-3.0f, -3.0f)) == -9.0f);

    TEST_SECTION("Vector2::ProjectOn");
    TEST_EXPECT(Vector2(2.0f, 5.0f).ProjectOn(Vector2(1.0f, 0.0f)).IsEqual(Vector2(2.0f, 0.0f)));

    TEST_SECTION("Vector2::GetPerpendicular");
    TEST_EXPECT(Vector2(1.0f, 2.0f).GetPerpendicular() == Vector2(-2.0f, 1.0f));

    TEST_SECTION("Vector2::GetRotated");
    TEST_EXPECT(Vector2(1.0f, 0.0f).GetRotated(HalfPI).IsEqual(Vector2(0.0f, 1.0f)));

    TEST_SECTION("Vector2::GetDistanceTo / GetDistanceSquaredTo");
    TEST_EXPECT(Math::Abs(Vector2(0.0f, 0.0f).GetDistanceTo(Vector2(3.0f, 4.0f)) - 5.0f) <= 1.0e-4f);
    TEST_EXPECT(Vector2(0.0f, 0.0f).GetDistanceSquaredTo(Vector2(3.0f, 4.0f)) == 25.0f);

    TEST_SECTION("Vector2::GetAngleBetween");
    TEST_EXPECT(Math::Abs(Vector2(1.0f, 0.0f).GetAngleBetween(Vector2(0.0f, 1.0f)) - HalfPI) <= 1.0e-4f);

    TEST_SECTION("Vector2::GetReflected");
    TEST_EXPECT(Vector2(1.0f, -1.0f).GetReflected(Vector2(0.0f, 1.0f)).IsEqual(Vector2(1.0f, 1.0f)));

    TEST_SECTION("Vector2::IsEqual");
    TEST_EXPECT(Vector2(1.0f, 2.0f).IsEqual(Vector2(1.0f, 2.0f)));
    TEST_EXPECT(!Vector2(1.0f, 2.0f).IsEqual(Vector2(1.0f, 3.0f)));

    TEST_SECTION("Vector2::Min / Max");
    TEST_EXPECT(Vector2::Min(Vector2(5.0f, -7.0f), Vector2(-3.0f)) == Vector2(-3.0f, -7.0f));
    TEST_EXPECT(Vector2::Max(Vector2(5.0f, -7.0f), Vector2(-3.0f)) == Vector2(5.0f, -3.0f));

    TEST_SECTION("Vector2::Lerp");
    TEST_EXPECT(Vector2::Lerp(Vector2(0.0f), Vector2(1.0f), 0.5f) == Vector2(0.5f, 0.5f));

    TEST_SECTION("Vector2::Clamp");
    TEST_EXPECT(Vector2::Clamp(Vector2(-3.5f, 7.5f), Vector2(-2.0f), Vector2(5.0f)) == Vector2(-2.0f, 5.0f));

    TEST_SECTION("Vector2::Saturate");
    TEST_EXPECT(Vector2::Saturate(Vector2(-5.0f, 1.5f)) == Vector2(0.0f, 1.0f));

    TEST_SECTION("Vector2::RadiansToDegrees / DegreesToRadians");
    TEST_EXPECT(Vector2::RadiansToDegrees(Vector2(Math::Constants::PI, HalfPI)).IsEqual(Vector2(180.0f, 90.0f), 1.0e-2f));
    TEST_EXPECT(Vector2::DegreesToRadians(Vector2(180.0f, 90.0f)).IsEqual(Vector2(Math::Constants::PI, HalfPI)));

    TEST_SECTION("Vector2::Normalize / GetNormalized / IsUnitVector");
    {
        Vector2 Norm(1.0f);
        Norm.Normalize();

        TEST_EXPECT(Norm.IsEqual(Vector2(0.70710678f, 0.70710678f)));
        TEST_EXPECT(Norm.IsUnitVector());
        TEST_EXPECT(Vector2(1.0f).GetNormalized().IsUnitVector());
    }

    TEST_SECTION("Vector2::ContainsNaN / ContainsInfinity");
    {
        const Vector2 NaNVector(1.0f, Math::Constants::NaN);
        const Vector2 InfVector(1.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaNVector.ContainsNaN());
        TEST_EXPECT(InfVector.ContainsInfinity());
        TEST_EXPECT(!Vector2(1.0f, 2.0f).ContainsNaN());
        TEST_EXPECT(!Vector2(1.0f, 2.0f).ContainsInfinity());
    }

    TEST_SECTION("Vector2::GetLength / GetLengthSquared");
    TEST_EXPECT(Math::Abs(Vector2(2.0f, 2.0f).GetLength() - 2.82842712f) <= 1.0e-4f);
    TEST_EXPECT(Vector2(2.0f, 2.0f).GetLengthSquared() == 8.0f);

    TEST_SECTION("Vector2::operator- (unary)");
    TEST_EXPECT(-Vector2(1.0f, 2.0f) == Vector2(-1.0f, -2.0f));

    TEST_SECTION("Vector2::operator+ / operator+=");
    TEST_EXPECT(Vector2(1.0f, 2.0f) + Vector2(3.0f, 1.0f) == Vector2(4.0f, 3.0f));
    TEST_EXPECT(Vector2(1.0f, 2.0f) + 5.0f == Vector2(6.0f, 7.0f));
    {
        Vector2 Vec(1.0f, 2.0f);
        Vec += Vector2(1.0f, 1.0f);
        Vec += 1.0f;

        TEST_EXPECT(Vec == Vector2(3.0f, 4.0f));
    }

    TEST_SECTION("Vector2::operator- / operator-=");
    TEST_EXPECT(Vector2(4.0f, 3.0f) - Vector2(3.0f, 1.0f) == Vector2(1.0f, 2.0f));
    TEST_EXPECT(Vector2(4.0f, 3.0f) - 5.0f == Vector2(-1.0f, -2.0f));
    {
        Vector2 Vec(4.0f, 3.0f);
        Vec -= Vector2(1.0f, 1.0f);
        Vec -= 1.0f;

        TEST_EXPECT(Vec == Vector2(2.0f, 1.0f));
    }

    TEST_SECTION("Vector2::operator* / operator*=");
    TEST_EXPECT(Vector2(1.0f, 2.0f) * Vector2(3.0f, 1.0f) == Vector2(3.0f, 2.0f));
    TEST_EXPECT(Vector2(1.0f, 2.0f) * 5.0f == Vector2(5.0f, 10.0f));
    TEST_EXPECT(5.0f * Vector2(1.0f, 2.0f) == Vector2(5.0f, 10.0f));
    {
        Vector2 Vec(1.0f, 2.0f);
        Vec *= Vector2(2.0f, 2.0f);
        Vec *= 2.0f;

        TEST_EXPECT(Vec == Vector2(4.0f, 8.0f));
    }

    TEST_SECTION("Vector2::operator/ / operator/=");
    TEST_EXPECT(Vector2(3.0f, 2.0f) / Vector2(3.0f, 1.0f) == Vector2(1.0f, 2.0f));
    TEST_EXPECT(Vector2(5.0f, 10.0f) / 5.0f == Vector2(1.0f, 2.0f));
    {
        Vector2 Vec(8.0f, 4.0f);
        Vec /= Vector2(2.0f, 2.0f);
        Vec /= 2.0f;

        TEST_EXPECT(Vec == Vector2(2.0f, 1.0f));
    }

    TEST_SECTION("Vector2::operator== / operator!=");
    TEST_EXPECT(Vector2(1.0f, 2.0f) == Vector2(1.0f, 2.0f));
    TEST_EXPECT(Vector2(1.0f, 2.0f) != Vector2(1.0f, 3.0f));

    TEST_SECTION("Vector2::operator[]");
    {
        Vector2 Vec(1.0f, 2.0f);
        Vec[0] = 5.0f;

        TEST_EXPECT(Vec[0] == 5.0f);
        TEST_EXPECT(Vec[1] == 2.0f);
    }

    TEST_END();
}
