#include "MathTest.h"

#include <Core/Math/Vector4.h>

bool TestVector4()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;

    TEST_SECTION("Vector4::Vector4 (constructors)");
    TEST_EXPECT(Vector4() == Vector4(0.0f, 0.0f, 0.0f, 0.0f));
    TEST_EXPECT(Vector4(-3.0f) == Vector4(-3.0f, -3.0f, -3.0f, -3.0f));
    TEST_EXPECT(Vector4(Vector3(1.0f, 2.0f, 3.0f)) == Vector4(1.0f, 2.0f, 3.0f, 0.0f));
    TEST_EXPECT(Vector4(Vector3(1.0f, 2.0f, 3.0f), 4.0f) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));

    TEST_SECTION("Vector4::DotProduct");
    TEST_EXPECT(Vector4(1.0f, 2.0f, -2.0f, 4.0f).DotProduct(Vector4(-3.0f)) == -15.0f);

    TEST_SECTION("Vector4::CrossProduct");
    TEST_EXPECT(Vector4(1.0f, 2.0f, -2.0f, 4.0f).CrossProduct(Vector4(5.0f, -7.0f, 2.0f, 4.0f)) == Vector4(-10.0f, -12.0f, -17.0f, 0.0f));

    TEST_SECTION("Vector4::ProjectOn");
    TEST_EXPECT(Vector4(4.0f, 5.0f, 3.0f, 10.0f).ProjectOn(Vector4(1.0f, 0.0f, 0.0f, 0.0f)).IsEqual(Vector4(4.0f, 0.0f, 0.0f, 0.0f)));

    TEST_SECTION("Vector4::GetReflected");
    TEST_EXPECT(Vector4(1.0f, 2.0f, -2.0f, 4.0f).GetReflected(Vector4(0.0f, 1.0f, 0.0f, 0.0f)).IsEqual(Vector4(1.0f, -2.0f, -2.0f, 4.0f)));

    TEST_SECTION("Vector4::GetDistanceTo / GetDistanceSquaredTo");
    TEST_EXPECT(Math::Abs(Vector4(0.0f).GetDistanceTo(Vector4(2.0f, 3.0f, 6.0f, 0.0f)) - 7.0f) <= 1.0e-4f);
    TEST_EXPECT(Vector4(0.0f).GetDistanceSquaredTo(Vector4(2.0f, 3.0f, 6.0f, 0.0f)) == 49.0f);

    TEST_SECTION("Vector4::GetAngleBetween");
    TEST_EXPECT(Math::Abs(Vector4(1.0f, 0.0f, 0.0f, 0.0f).GetAngleBetween(Vector4(0.0f, 1.0f, 0.0f, 0.0f)) - HalfPI) <= 1.0e-4f);

    TEST_SECTION("Vector4::IsEqual");
    TEST_EXPECT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsEqual(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
    TEST_EXPECT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsEqual(Vector4(1.0f, 2.0f, 3.0f, 5.0f)));

    TEST_SECTION("Vector4::Min / Max");
    TEST_EXPECT(Vector4::Min(Vector4(5.0f, -7.0f, 2.0f, 4.0f), Vector4(-3.0f)) == Vector4(-3.0f, -7.0f, -3.0f, -3.0f));
    TEST_EXPECT(Vector4::Max(Vector4(5.0f, -7.0f, 2.0f, 4.0f), Vector4(-3.0f)) == Vector4(5.0f, -3.0f, 2.0f, 4.0f));

    TEST_SECTION("Vector4::Lerp");
    TEST_EXPECT(Vector4::Lerp(Vector4(0.0f), Vector4(1.0f), 0.5f) == Vector4(0.5f));

    TEST_SECTION("Vector4::Clamp");
    TEST_EXPECT(Vector4::Clamp(Vector4(-3.5f, 7.5f, 1.0f, 2.0f), Vector4(-2.0f), Vector4(5.0f)) == Vector4(-2.0f, 5.0f, 1.0f, 2.0f));

    TEST_SECTION("Vector4::Saturate");
    TEST_EXPECT(Vector4::Saturate(Vector4(-5.0f, 1.5f, 0.25f, -5.7f)) == Vector4(0.0f, 1.0f, 0.25f, 0.0f));

    TEST_SECTION("Vector4::RadiansToDegrees / DegreesToRadians");
    TEST_EXPECT(Vector4::RadiansToDegrees(Vector4(Math::Constants::PI, HalfPI, 0.0f, 0.0f)).IsEqual(Vector4(180.0f, 90.0f, 0.0f, 0.0f), 1.0e-2f));
    TEST_EXPECT(Vector4::DegreesToRadians(Vector4(180.0f, 90.0f, 0.0f, 0.0f)).IsEqual(Vector4(Math::Constants::PI, HalfPI, 0.0f, 0.0f)));

    TEST_SECTION("Vector4::Normalize / GetNormalized / IsUnitVector");
    {
        Vector4 Norm(1.0f);
        const Vector4 Normalized = Norm.GetNormalized();
        Norm.Normalize();

        TEST_EXPECT(Norm.IsEqual(Vector4(0.5f)));
        TEST_EXPECT(Normalized.IsEqual(Vector4(0.5f)));
        TEST_EXPECT(Norm.IsUnitVector());
    }

    TEST_SECTION("Vector4::NormalizeXYZ / GetNormalizedXYZ");
    {
        Vector4 Vec(0.0f, 3.0f, 4.0f, 9.0f);
        const Vector4 NormalizedXYZ = Vec.GetNormalizedXYZ();
        Vec.NormalizeXYZ();

        TEST_EXPECT(Vec.IsEqual(Vector4(0.0f, 0.6f, 0.8f, 0.0f)));
        TEST_EXPECT(NormalizedXYZ.IsEqual(Vector4(0.0f, 0.6f, 0.8f, 0.0f)));
    }

    TEST_SECTION("Vector4::ContainsNaN / ContainsInfinity");
    {
        const Vector4 NaNVector(1.0f, 0.0f, 0.0f, Math::Constants::NaN);
        const Vector4 InfVector(1.0f, 0.0f, 0.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaNVector.ContainsNaN());
        TEST_EXPECT(InfVector.ContainsInfinity());
        TEST_EXPECT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).ContainsNaN());
        TEST_EXPECT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).ContainsInfinity());
    }

    TEST_SECTION("Vector4::GetLength / GetLengthSquared");
    TEST_EXPECT(Math::Abs(Vector4(2.0f).GetLength() - 4.0f) <= 1.0e-4f);
    TEST_EXPECT(Vector4(2.0f).GetLengthSquared() == 16.0f);

    TEST_SECTION("Vector4::operator- (unary)");
    TEST_EXPECT(-Vector4(1.0f, 2.0f, -2.0f, 4.0f) == Vector4(-1.0f, -2.0f, 2.0f, -4.0f));

    TEST_SECTION("Vector4::operator+ / operator+=");
    TEST_EXPECT(Vector4(1.0f, 2.0f, 3.0f, 4.0f) + Vector4(3.0f, 1.0f, -1.0f, 2.0f) == Vector4(4.0f, 3.0f, 2.0f, 6.0f));
    TEST_EXPECT(Vector4(1.0f, 2.0f, 3.0f, 4.0f) + 5.0f == Vector4(6.0f, 7.0f, 8.0f, 9.0f));
    {
        Vector4 Vec(1.0f, 2.0f, 3.0f, 4.0f);
        Vec += Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vec += 1.0f;

        TEST_EXPECT(Vec == Vector4(3.0f, 4.0f, 5.0f, 6.0f));
    }

    TEST_SECTION("Vector4::operator- / operator-=");
    TEST_EXPECT(Vector4(4.0f, 3.0f, 7.0f, 1.0f) - Vector4(3.0f, 1.0f, 8.0f, 3.0f) == Vector4(1.0f, 2.0f, -1.0f, -2.0f));
    TEST_EXPECT(Vector4(4.0f, 3.0f, 7.0f, 1.0f) - 5.0f == Vector4(-1.0f, -2.0f, 2.0f, -4.0f));
    {
        Vector4 Vec(4.0f, 3.0f, 7.0f, 1.0f);
        Vec -= Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vec -= 1.0f;

        TEST_EXPECT(Vec == Vector4(2.0f, 1.0f, 5.0f, -1.0f));
    }

    TEST_SECTION("Vector4::operator* / operator*=");
    TEST_EXPECT(Vector4(1.0f, 2.0f, -1.0f, -2.0f) * Vector4(3.0f, 1.0f, 2.0f, -1.0f) == Vector4(3.0f, 2.0f, -2.0f, 2.0f));
    TEST_EXPECT(Vector4(1.0f, 2.0f, -1.0f, -2.0f) * 5.0f == Vector4(5.0f, 10.0f, -5.0f, -10.0f));
    TEST_EXPECT(5.0f * Vector4(1.0f, 2.0f, -1.0f, -2.0f) == Vector4(5.0f, 10.0f, -5.0f, -10.0f));
    {
        Vector4 Vec(1.0f, 2.0f, 3.0f, 4.0f);
        Vec *= Vector4(2.0f, 2.0f, 2.0f, 2.0f);
        Vec *= 2.0f;

        TEST_EXPECT(Vec == Vector4(4.0f, 8.0f, 12.0f, 16.0f));
    }

    TEST_SECTION("Vector4::operator/ / operator/=");
    TEST_EXPECT(Vector4(3.0f, 2.0f, -2.0f, 2.0f) / Vector4(3.0f, 1.0f, 2.0f, 2.0f) == Vector4(1.0f, 2.0f, -1.0f, 1.0f));
    TEST_EXPECT(Vector4(5.0f, 10.0f, -5.0f, -10.0f) / 5.0f == Vector4(1.0f, 2.0f, -1.0f, -2.0f));
    {
        Vector4 Vec(8.0f, 4.0f, 2.0f, 16.0f);
        Vec /= Vector4(2.0f, 2.0f, 2.0f, 2.0f);
        Vec /= 2.0f;

        TEST_EXPECT(Vec == Vector4(2.0f, 1.0f, 0.5f, 4.0f));
    }

    TEST_SECTION("Vector4::operator== / operator!=");
    TEST_EXPECT(Vector4(1.0f, 2.0f, 3.0f, 4.0f) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    TEST_EXPECT(Vector4(1.0f, 2.0f, 3.0f, 4.0f) != Vector4(1.0f, 2.0f, 3.0f, 5.0f));

    TEST_SECTION("Vector4::operator[]");
    {
        Vector4 Vec(1.0f, 2.0f, 3.0f, 4.0f);
        Vec[3] = -2.0f;

        TEST_EXPECT(Vec[0] == 1.0f);
        TEST_EXPECT(Vec[3] == -2.0f);
    }

    TEST_END();
}
