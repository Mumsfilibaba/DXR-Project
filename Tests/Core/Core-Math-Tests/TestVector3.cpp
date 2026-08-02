#include "MathTest.h"

#include <Core/Math/Vector3.h>

bool TestVector3()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;

    TEST_SECTION("Vector3::Vector3 (constructors)");
    TEST_EXPECT(Vector3() == Vector3(0.0f, 0.0f, 0.0f));
    TEST_EXPECT(Vector3(-3.0f) == Vector3(-3.0f, -3.0f, -3.0f));

    TEST_SECTION("Vector3 axis constants");
    TEST_EXPECT(Vector3::Up.IsUnitVector());
    TEST_EXPECT(Vector3::Forward.IsUnitVector());
    TEST_EXPECT(Vector3::Right.IsUnitVector());

    TEST_SECTION("Vector3::DotProduct");
    TEST_EXPECT(Vector3(1.0f, 2.0f, -2.0f).DotProduct(Vector3(-3.0f)) == -3.0f);

    TEST_SECTION("Vector3::CrossProduct");
    TEST_EXPECT(Vector3(1.0f, 2.0f, -2.0f).CrossProduct(Vector3(5.0f, -7.0f, 2.0f)) == Vector3(-10.0f, -12.0f, -17.0f));

    TEST_SECTION("Vector3::ProjectOn");
    TEST_EXPECT(Vector3(4.0f, 5.0f, 3.0f).ProjectOn(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(4.0f, 0.0f, 0.0f)));

    TEST_SECTION("Vector3::GetReflected");
    TEST_EXPECT(Vector3(1.0f, 2.0f, -2.0f).GetReflected(Vector3(0.0f, 1.0f, 0.0f)).IsEqual(Vector3(1.0f, -2.0f, -2.0f)));

    TEST_SECTION("Vector3::GetRotated");
    TEST_EXPECT(Vector3(1.0f, 0.0f, 0.0f).GetRotated(Vector3(0.0f, 0.0f, 1.0f), HalfPI).IsEqual(Vector3(0.0f, 1.0f, 0.0f)));

    TEST_SECTION("Vector3::GetDistanceTo / GetDistanceSquaredTo");
    TEST_EXPECT(Math::Abs(Vector3(0.0f).GetDistanceTo(Vector3(2.0f, 3.0f, 6.0f)) - 7.0f) <= 1.0e-4f);
    TEST_EXPECT(Vector3(0.0f).GetDistanceSquaredTo(Vector3(2.0f, 3.0f, 6.0f)) == 49.0f);

    TEST_SECTION("Vector3::GetAngleBetween");
    TEST_EXPECT(Math::Abs(Vector3(1.0f, 0.0f, 0.0f).GetAngleBetween(Vector3(0.0f, 1.0f, 0.0f)) - HalfPI) <= 1.0e-4f);

    TEST_SECTION("Vector3::IsEqual");
    TEST_EXPECT(Vector3(1.0f, 2.0f, 3.0f).IsEqual(Vector3(1.0f, 2.0f, 3.0f)));
    TEST_EXPECT(!Vector3(1.0f, 2.0f, 3.0f).IsEqual(Vector3(1.0f, 2.0f, 4.0f)));

    TEST_SECTION("Vector3::Min / Max");
    TEST_EXPECT(Vector3::Min(Vector3(5.0f, -7.0f, 2.0f), Vector3(-3.0f)) == Vector3(-3.0f, -7.0f, -3.0f));
    TEST_EXPECT(Vector3::Max(Vector3(5.0f, -7.0f, 2.0f), Vector3(-3.0f)) == Vector3(5.0f, -3.0f, 2.0f));

    TEST_SECTION("Vector3::Lerp");
    TEST_EXPECT(Vector3::Lerp(Vector3(0.0f), Vector3(1.0f), 0.5f) == Vector3(0.5f));

    TEST_SECTION("Vector3::Clamp");
    TEST_EXPECT(Vector3::Clamp(Vector3(-3.5f, 7.5f, 1.0f), Vector3(-2.0f), Vector3(5.0f)) == Vector3(-2.0f, 5.0f, 1.0f));

    TEST_SECTION("Vector3::Saturate");
    TEST_EXPECT(Vector3::Saturate(Vector3(-5.0f, 1.5f, 0.25f)) == Vector3(0.0f, 1.0f, 0.25f));

    TEST_SECTION("Vector3::RadiansToDegrees / DegreesToRadians");
    TEST_EXPECT(Vector3::RadiansToDegrees(Vector3(Math::Constants::PI, HalfPI, 0.0f)).IsEqual(Vector3(180.0f, 90.0f, 0.0f), 1.0e-2f));
    TEST_EXPECT(Vector3::DegreesToRadians(Vector3(180.0f, 90.0f, 0.0f)).IsEqual(Vector3(Math::Constants::PI, HalfPI, 0.0f)));

    TEST_SECTION("Vector3::Normalize / GetNormalized / IsUnitVector");
    {
        Vector3 Norm(1.0f);
        Norm.Normalize();

        TEST_EXPECT(Norm.IsEqual(Vector3(0.57735026f)));
        TEST_EXPECT(Norm.IsUnitVector());
        TEST_EXPECT(Vector3(1.0f).GetNormalized().IsUnitVector());
    }

    TEST_SECTION("Vector3::ContainsNaN / ContainsInfinity");
    {
        const Vector3 NaNVector(1.0f, 0.0f, Math::Constants::NaN);
        const Vector3 InfVector(1.0f, 0.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaNVector.ContainsNaN());
        TEST_EXPECT(InfVector.ContainsInfinity());
        TEST_EXPECT(!Vector3(1.0f, 2.0f, 3.0f).ContainsNaN());
        TEST_EXPECT(!Vector3(1.0f, 2.0f, 3.0f).ContainsInfinity());
    }

    TEST_SECTION("Vector3::GetLength / GetLengthSquared");
    TEST_EXPECT(Math::Abs(Vector3(2.0f).GetLength() - 3.46410161f) <= 1.0e-4f);
    TEST_EXPECT(Vector3(2.0f).GetLengthSquared() == 12.0f);

    TEST_SECTION("Vector3::operator- (unary)");
    TEST_EXPECT(-Vector3(1.0f, 2.0f, -2.0f) == Vector3(-1.0f, -2.0f, 2.0f));

    TEST_SECTION("Vector3::operator+ / operator+=");
    TEST_EXPECT(Vector3(1.0f, 2.0f, 3.0f) + Vector3(3.0f, 1.0f, -1.0f) == Vector3(4.0f, 3.0f, 2.0f));
    TEST_EXPECT(Vector3(1.0f, 2.0f, 3.0f) + 5.0f == Vector3(6.0f, 7.0f, 8.0f));
    {
        Vector3 Vec(1.0f, 2.0f, 3.0f);
        Vec += Vector3(1.0f, 1.0f, 1.0f);
        Vec += 1.0f;

        TEST_EXPECT(Vec == Vector3(3.0f, 4.0f, 5.0f));
    }

    TEST_SECTION("Vector3::operator- / operator-=");
    TEST_EXPECT(Vector3(4.0f, 3.0f, 7.0f) - Vector3(3.0f, 1.0f, 8.0f) == Vector3(1.0f, 2.0f, -1.0f));
    TEST_EXPECT(Vector3(4.0f, 3.0f, 7.0f) - 5.0f == Vector3(-1.0f, -2.0f, 2.0f));
    {
        Vector3 Vec(4.0f, 3.0f, 7.0f);
        Vec -= Vector3(1.0f, 1.0f, 1.0f);
        Vec -= 1.0f;

        TEST_EXPECT(Vec == Vector3(2.0f, 1.0f, 5.0f));
    }

    TEST_SECTION("Vector3::operator* / operator*=");
    TEST_EXPECT(Vector3(1.0f, 2.0f, -1.0f) * Vector3(3.0f, 1.0f, 2.0f) == Vector3(3.0f, 2.0f, -2.0f));
    TEST_EXPECT(Vector3(1.0f, 2.0f, -1.0f) * 5.0f == Vector3(5.0f, 10.0f, -5.0f));
    TEST_EXPECT(5.0f * Vector3(1.0f, 2.0f, -1.0f) == Vector3(5.0f, 10.0f, -5.0f));
    {
        Vector3 Vec(1.0f, 2.0f, 3.0f);
        Vec *= Vector3(2.0f, 2.0f, 2.0f);
        Vec *= 2.0f;

        TEST_EXPECT(Vec == Vector3(4.0f, 8.0f, 12.0f));
    }

    TEST_SECTION("Vector3::operator/ / operator/=");
    TEST_EXPECT(Vector3(3.0f, 2.0f, -2.0f) / Vector3(3.0f, 1.0f, 2.0f) == Vector3(1.0f, 2.0f, -1.0f));
    TEST_EXPECT(Vector3(5.0f, 10.0f, -5.0f) / 5.0f == Vector3(1.0f, 2.0f, -1.0f));
    {
        Vector3 Vec(8.0f, 4.0f, 2.0f);
        Vec /= Vector3(2.0f, 2.0f, 2.0f);
        Vec /= 2.0f;

        TEST_EXPECT(Vec == Vector3(2.0f, 1.0f, 0.5f));
    }

    TEST_SECTION("Vector3::operator== / operator!=");
    TEST_EXPECT(Vector3(1.0f, 2.0f, 3.0f) == Vector3(1.0f, 2.0f, 3.0f));
    TEST_EXPECT(Vector3(1.0f, 2.0f, 3.0f) != Vector3(1.0f, 2.0f, 4.0f));

    TEST_SECTION("Vector3::operator[]");
    {
        Vector3 Vec(1.0f, 2.0f, 3.0f);
        Vec[2] = -1.0f;

        TEST_EXPECT(Vec[0] == 1.0f);
        TEST_EXPECT(Vec[2] == -1.0f);
    }

    TEST_END();
}
