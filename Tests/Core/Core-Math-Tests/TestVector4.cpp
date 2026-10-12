#include "MathTest.h"

#include <Core/Math/Vector4.h>

bool TestVector4()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;

    TEST_SECTION(TEXT("FVector4::FVector4 (constructors)"));
    TEST_EXPECT(FVector4() == FVector4(0.0f, 0.0f, 0.0f, 0.0f));
    TEST_EXPECT(FVector4(-3.0f) == FVector4(-3.0f, -3.0f, -3.0f, -3.0f));
    TEST_EXPECT(FVector4(FVector3(1.0f, 2.0f, 3.0f)) == FVector4(1.0f, 2.0f, 3.0f, 0.0f));
    TEST_EXPECT(FVector4(FVector3(1.0f, 2.0f, 3.0f), 4.0f) == FVector4(1.0f, 2.0f, 3.0f, 4.0f));

    TEST_SECTION(TEXT("FVector4::DotProduct"));
    TEST_EXPECT(FVector4(1.0f, 2.0f, -2.0f, 4.0f).DotProduct(FVector4(-3.0f)) == -15.0f);

    TEST_SECTION(TEXT("FVector4::CrossProduct"));
    TEST_EXPECT(FVector4(1.0f, 2.0f, -2.0f, 4.0f).CrossProduct(FVector4(5.0f, -7.0f, 2.0f, 4.0f)) == FVector4(-10.0f, -12.0f, -17.0f, 0.0f));

    TEST_SECTION(TEXT("FVector4::ProjectOn"));
    TEST_EXPECT(FVector4(4.0f, 5.0f, 3.0f, 10.0f).ProjectOn(FVector4(1.0f, 0.0f, 0.0f, 0.0f)).IsEqual(FVector4(4.0f, 0.0f, 0.0f, 0.0f)));

    TEST_SECTION(TEXT("FVector4::GetReflected"));
    TEST_EXPECT(FVector4(1.0f, 2.0f, -2.0f, 4.0f).GetReflected(FVector4(0.0f, 1.0f, 0.0f, 0.0f)).IsEqual(FVector4(1.0f, -2.0f, -2.0f, 4.0f)));

    TEST_SECTION(TEXT("FVector4::GetDistanceTo / GetDistanceSquaredTo"));
    TEST_EXPECT(Math::Abs(FVector4(0.0f).GetDistanceTo(FVector4(2.0f, 3.0f, 6.0f, 0.0f)) - 7.0f) <= 1.0e-4f);
    TEST_EXPECT(FVector4(0.0f).GetDistanceSquaredTo(FVector4(2.0f, 3.0f, 6.0f, 0.0f)) == 49.0f);

    TEST_SECTION(TEXT("FVector4::GetAngleBetween"));
    TEST_EXPECT(Math::Abs(FVector4(1.0f, 0.0f, 0.0f, 0.0f).GetAngleBetween(FVector4(0.0f, 1.0f, 0.0f, 0.0f)) - HalfPI) <= 1.0e-4f);

    TEST_SECTION(TEXT("FVector4::IsEqual"));
    TEST_EXPECT(FVector4(1.0f, 2.0f, 3.0f, 4.0f).IsEqual(FVector4(1.0f, 2.0f, 3.0f, 4.0f)));
    TEST_EXPECT(!FVector4(1.0f, 2.0f, 3.0f, 4.0f).IsEqual(FVector4(1.0f, 2.0f, 3.0f, 5.0f)));

    TEST_SECTION(TEXT("FVector4::Min / Max"));
    TEST_EXPECT(FVector4::Min(FVector4(5.0f, -7.0f, 2.0f, 4.0f), FVector4(-3.0f)) == FVector4(-3.0f, -7.0f, -3.0f, -3.0f));
    TEST_EXPECT(FVector4::Max(FVector4(5.0f, -7.0f, 2.0f, 4.0f), FVector4(-3.0f)) == FVector4(5.0f, -3.0f, 2.0f, 4.0f));

    TEST_SECTION(TEXT("FVector4::Lerp"));
    TEST_EXPECT(FVector4::Lerp(FVector4(0.0f), FVector4(1.0f), 0.5f) == FVector4(0.5f));

    TEST_SECTION(TEXT("FVector4::Clamp"));
    TEST_EXPECT(FVector4::Clamp(FVector4(-3.5f, 7.5f, 1.0f, 2.0f), FVector4(-2.0f), FVector4(5.0f)) == FVector4(-2.0f, 5.0f, 1.0f, 2.0f));

    TEST_SECTION(TEXT("FVector4::Saturate"));
    TEST_EXPECT(FVector4::Saturate(FVector4(-5.0f, 1.5f, 0.25f, -5.7f)) == FVector4(0.0f, 1.0f, 0.25f, 0.0f));

    TEST_SECTION(TEXT("FVector4::RadiansToDegrees / DegreesToRadians"));
    TEST_EXPECT(FVector4::RadiansToDegrees(FVector4(Math::Constants::PI, HalfPI, 0.0f, 0.0f)).IsEqual(FVector4(180.0f, 90.0f, 0.0f, 0.0f), 1.0e-2f));
    TEST_EXPECT(FVector4::DegreesToRadians(FVector4(180.0f, 90.0f, 0.0f, 0.0f)).IsEqual(FVector4(Math::Constants::PI, HalfPI, 0.0f, 0.0f)));

    TEST_SECTION(TEXT("FVector4::Normalize / GetNormalized / IsUnitVector"));
    {
        FVector4 Norm(1.0f);
        const FVector4 Normalized = Norm.GetNormalized();
        Norm.Normalize();

        TEST_EXPECT(Norm.IsEqual(FVector4(0.5f)));
        TEST_EXPECT(Normalized.IsEqual(FVector4(0.5f)));
        TEST_EXPECT(Norm.IsUnitVector());
    }

    TEST_SECTION(TEXT("FVector4::NormalizeXYZ / GetNormalizedXYZ"));
    {
        FVector4 Vec(0.0f, 3.0f, 4.0f, 9.0f);
        const FVector4 NormalizedXYZ = Vec.GetNormalizedXYZ();
        Vec.NormalizeXYZ();

        TEST_EXPECT(Vec.IsEqual(FVector4(0.0f, 0.6f, 0.8f, 0.0f)));
        TEST_EXPECT(NormalizedXYZ.IsEqual(FVector4(0.0f, 0.6f, 0.8f, 0.0f)));
    }

    TEST_SECTION(TEXT("FVector4::ContainsNaN / ContainsInfinity"));
    {
        const FVector4 NaNVector(1.0f, 0.0f, 0.0f, Math::Constants::NaN);
        const FVector4 InfVector(1.0f, 0.0f, 0.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaNVector.ContainsNaN());
        TEST_EXPECT(InfVector.ContainsInfinity());
        TEST_EXPECT(!FVector4(1.0f, 2.0f, 3.0f, 4.0f).ContainsNaN());
        TEST_EXPECT(!FVector4(1.0f, 2.0f, 3.0f, 4.0f).ContainsInfinity());
    }

    TEST_SECTION(TEXT("FVector4::GetLength / GetLengthSquared"));
    TEST_EXPECT(Math::Abs(FVector4(2.0f).GetLength() - 4.0f) <= 1.0e-4f);
    TEST_EXPECT(FVector4(2.0f).GetLengthSquared() == 16.0f);

    TEST_SECTION(TEXT("FVector4::operator- (unary)"));
    TEST_EXPECT(-FVector4(1.0f, 2.0f, -2.0f, 4.0f) == FVector4(-1.0f, -2.0f, 2.0f, -4.0f));

    TEST_SECTION(TEXT("FVector4::operator+ / operator+="));
    TEST_EXPECT(FVector4(1.0f, 2.0f, 3.0f, 4.0f) + FVector4(3.0f, 1.0f, -1.0f, 2.0f) == FVector4(4.0f, 3.0f, 2.0f, 6.0f));
    TEST_EXPECT(FVector4(1.0f, 2.0f, 3.0f, 4.0f) + 5.0f == FVector4(6.0f, 7.0f, 8.0f, 9.0f));
    {
        FVector4 Vec(1.0f, 2.0f, 3.0f, 4.0f);
        Vec += FVector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vec += 1.0f;

        TEST_EXPECT(Vec == FVector4(3.0f, 4.0f, 5.0f, 6.0f));
    }

    TEST_SECTION(TEXT("FVector4::operator- / operator-="));
    TEST_EXPECT(FVector4(4.0f, 3.0f, 7.0f, 1.0f) - FVector4(3.0f, 1.0f, 8.0f, 3.0f) == FVector4(1.0f, 2.0f, -1.0f, -2.0f));
    TEST_EXPECT(FVector4(4.0f, 3.0f, 7.0f, 1.0f) - 5.0f == FVector4(-1.0f, -2.0f, 2.0f, -4.0f));
    {
        FVector4 Vec(4.0f, 3.0f, 7.0f, 1.0f);
        Vec -= FVector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vec -= 1.0f;

        TEST_EXPECT(Vec == FVector4(2.0f, 1.0f, 5.0f, -1.0f));
    }

    TEST_SECTION(TEXT("FVector4::operator* / operator*="));
    TEST_EXPECT(FVector4(1.0f, 2.0f, -1.0f, -2.0f) * FVector4(3.0f, 1.0f, 2.0f, -1.0f) == FVector4(3.0f, 2.0f, -2.0f, 2.0f));
    TEST_EXPECT(FVector4(1.0f, 2.0f, -1.0f, -2.0f) * 5.0f == FVector4(5.0f, 10.0f, -5.0f, -10.0f));
    TEST_EXPECT(5.0f * FVector4(1.0f, 2.0f, -1.0f, -2.0f) == FVector4(5.0f, 10.0f, -5.0f, -10.0f));
    {
        FVector4 Vec(1.0f, 2.0f, 3.0f, 4.0f);
        Vec *= FVector4(2.0f, 2.0f, 2.0f, 2.0f);
        Vec *= 2.0f;

        TEST_EXPECT(Vec == FVector4(4.0f, 8.0f, 12.0f, 16.0f));
    }

    TEST_SECTION(TEXT("FVector4::operator/ / operator/="));
    TEST_EXPECT(FVector4(3.0f, 2.0f, -2.0f, 2.0f) / FVector4(3.0f, 1.0f, 2.0f, 2.0f) == FVector4(1.0f, 2.0f, -1.0f, 1.0f));
    TEST_EXPECT(FVector4(5.0f, 10.0f, -5.0f, -10.0f) / 5.0f == FVector4(1.0f, 2.0f, -1.0f, -2.0f));
    {
        FVector4 Vec(8.0f, 4.0f, 2.0f, 16.0f);
        Vec /= FVector4(2.0f, 2.0f, 2.0f, 2.0f);
        Vec /= 2.0f;

        TEST_EXPECT(Vec == FVector4(2.0f, 1.0f, 0.5f, 4.0f));
    }

    TEST_SECTION(TEXT("FVector4::operator== / operator!="));
    TEST_EXPECT(FVector4(1.0f, 2.0f, 3.0f, 4.0f) == FVector4(1.0f, 2.0f, 3.0f, 4.0f));
    TEST_EXPECT(FVector4(1.0f, 2.0f, 3.0f, 4.0f) != FVector4(1.0f, 2.0f, 3.0f, 5.0f));

    TEST_SECTION(TEXT("FVector4::operator[]"));
    {
        FVector4 Vec(1.0f, 2.0f, 3.0f, 4.0f);
        Vec[3] = -2.0f;

        TEST_EXPECT(Vec[0] == 1.0f);
        TEST_EXPECT(Vec[3] == -2.0f);
    }

    TEST_END();
}
