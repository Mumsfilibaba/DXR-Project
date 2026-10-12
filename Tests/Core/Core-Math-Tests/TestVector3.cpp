#include "MathTest.h"

#include <Core/Math/Vector3.h>

bool TestVector3()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;

    TEST_SECTION(TEXT("FVector3::FVector3 (constructors)"));
    TEST_EXPECT(FVector3() == FVector3(0.0f, 0.0f, 0.0f));
    TEST_EXPECT(FVector3(-3.0f) == FVector3(-3.0f, -3.0f, -3.0f));

    TEST_SECTION(TEXT("FVector3 axis constants"));
    TEST_EXPECT(FVector3::Up.IsUnitVector());
    TEST_EXPECT(FVector3::Forward.IsUnitVector());
    TEST_EXPECT(FVector3::Right.IsUnitVector());

    TEST_SECTION(TEXT("FVector3::DotProduct"));
    TEST_EXPECT(FVector3(1.0f, 2.0f, -2.0f).DotProduct(FVector3(-3.0f)) == -3.0f);

    TEST_SECTION(TEXT("FVector3::CrossProduct"));
    TEST_EXPECT(FVector3(1.0f, 2.0f, -2.0f).CrossProduct(FVector3(5.0f, -7.0f, 2.0f)) == FVector3(-10.0f, -12.0f, -17.0f));

    TEST_SECTION(TEXT("FVector3::ProjectOn"));
    TEST_EXPECT(FVector3(4.0f, 5.0f, 3.0f).ProjectOn(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(4.0f, 0.0f, 0.0f)));

    TEST_SECTION(TEXT("FVector3::GetOrthonormalTo"));
    TEST_EXPECT(FVector3(1.0f, 1.0f, 0.0f).GetOrthonormalTo(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(0.0f, 1.0f, 0.0f)));
    TEST_EXPECT(FVector3(1.0f, 1.0f, 0.0f).GetOrthonormalTo(FVector3(3.0f, 0.0f, 0.0f)).IsEqual(FVector3(0.0f, 1.0f, 0.0f)));
    TEST_EXPECT(FVector3(2.0f, 0.0f, 0.0f).GetOrthonormalTo(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(0.0f, 0.0f, 0.0f)));

    TEST_SECTION(TEXT("FVector3::GetReflected"));
    TEST_EXPECT(FVector3(1.0f, 2.0f, -2.0f).GetReflected(FVector3(0.0f, 1.0f, 0.0f)).IsEqual(FVector3(1.0f, -2.0f, -2.0f)));

    TEST_SECTION(TEXT("FVector3::GetRotated"));
    TEST_EXPECT(FVector3(1.0f, 0.0f, 0.0f).GetRotated(FVector3(0.0f, 0.0f, 1.0f), HalfPI).IsEqual(FVector3(0.0f, 1.0f, 0.0f)));

    TEST_SECTION(TEXT("FVector3::GetDistanceTo / GetDistanceSquaredTo"));
    TEST_EXPECT(Math::Abs(FVector3(0.0f).GetDistanceTo(FVector3(2.0f, 3.0f, 6.0f)) - 7.0f) <= 1.0e-4f);
    TEST_EXPECT(FVector3(0.0f).GetDistanceSquaredTo(FVector3(2.0f, 3.0f, 6.0f)) == 49.0f);

    TEST_SECTION(TEXT("FVector3::GetAngleBetween"));
    TEST_EXPECT(Math::Abs(FVector3(1.0f, 0.0f, 0.0f).GetAngleBetween(FVector3(0.0f, 1.0f, 0.0f)) - HalfPI) <= 1.0e-4f);

    TEST_SECTION(TEXT("FVector3::IsEqual"));
    TEST_EXPECT(FVector3(1.0f, 2.0f, 3.0f).IsEqual(FVector3(1.0f, 2.0f, 3.0f)));
    TEST_EXPECT(!FVector3(1.0f, 2.0f, 3.0f).IsEqual(FVector3(1.0f, 2.0f, 4.0f)));

    TEST_SECTION(TEXT("FVector3::Min / Max"));
    TEST_EXPECT(FVector3::Min(FVector3(5.0f, -7.0f, 2.0f), FVector3(-3.0f)) == FVector3(-3.0f, -7.0f, -3.0f));
    TEST_EXPECT(FVector3::Max(FVector3(5.0f, -7.0f, 2.0f), FVector3(-3.0f)) == FVector3(5.0f, -3.0f, 2.0f));

    TEST_SECTION(TEXT("FVector3::Lerp"));
    TEST_EXPECT(FVector3::Lerp(FVector3(0.0f), FVector3(1.0f), 0.5f) == FVector3(0.5f));

    TEST_SECTION(TEXT("FVector3::Clamp"));
    TEST_EXPECT(FVector3::Clamp(FVector3(-3.5f, 7.5f, 1.0f), FVector3(-2.0f), FVector3(5.0f)) == FVector3(-2.0f, 5.0f, 1.0f));

    TEST_SECTION(TEXT("FVector3::Saturate"));
    TEST_EXPECT(FVector3::Saturate(FVector3(-5.0f, 1.5f, 0.25f)) == FVector3(0.0f, 1.0f, 0.25f));

    TEST_SECTION(TEXT("FVector3::RadiansToDegrees / DegreesToRadians"));
    TEST_EXPECT(FVector3::RadiansToDegrees(FVector3(Math::Constants::PI, HalfPI, 0.0f)).IsEqual(FVector3(180.0f, 90.0f, 0.0f), 1.0e-2f));
    TEST_EXPECT(FVector3::DegreesToRadians(FVector3(180.0f, 90.0f, 0.0f)).IsEqual(FVector3(Math::Constants::PI, HalfPI, 0.0f)));

    TEST_SECTION(TEXT("FVector3::Normalize / GetNormalized / IsUnitVector"));
    {
        FVector3 Norm(1.0f);
        Norm.Normalize();

        TEST_EXPECT(Norm.IsEqual(FVector3(0.57735026f)));
        TEST_EXPECT(Norm.IsUnitVector());
        TEST_EXPECT(FVector3(1.0f).GetNormalized().IsUnitVector());
    }

    TEST_SECTION(TEXT("FVector3::ContainsNaN / ContainsInfinity"));
    {
        const FVector3 NaNVector(1.0f, 0.0f, Math::Constants::NaN);
        const FVector3 InfVector(1.0f, 0.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaNVector.ContainsNaN());
        TEST_EXPECT(InfVector.ContainsInfinity());
        TEST_EXPECT(!FVector3(1.0f, 2.0f, 3.0f).ContainsNaN());
        TEST_EXPECT(!FVector3(1.0f, 2.0f, 3.0f).ContainsInfinity());
    }

    TEST_SECTION(TEXT("FVector3::GetLength / GetLengthSquared"));
    TEST_EXPECT(Math::Abs(FVector3(2.0f).GetLength() - 3.46410161f) <= 1.0e-4f);
    TEST_EXPECT(FVector3(2.0f).GetLengthSquared() == 12.0f);

    TEST_SECTION(TEXT("FVector3::operator- (unary)"));
    TEST_EXPECT(-FVector3(1.0f, 2.0f, -2.0f) == FVector3(-1.0f, -2.0f, 2.0f));

    TEST_SECTION(TEXT("FVector3::operator+ / operator+="));
    TEST_EXPECT(FVector3(1.0f, 2.0f, 3.0f) + FVector3(3.0f, 1.0f, -1.0f) == FVector3(4.0f, 3.0f, 2.0f));
    TEST_EXPECT(FVector3(1.0f, 2.0f, 3.0f) + 5.0f == FVector3(6.0f, 7.0f, 8.0f));
    {
        FVector3 Vec(1.0f, 2.0f, 3.0f);
        Vec += FVector3(1.0f, 1.0f, 1.0f);
        Vec += 1.0f;

        TEST_EXPECT(Vec == FVector3(3.0f, 4.0f, 5.0f));
    }

    TEST_SECTION(TEXT("FVector3::operator- / operator-="));
    TEST_EXPECT(FVector3(4.0f, 3.0f, 7.0f) - FVector3(3.0f, 1.0f, 8.0f) == FVector3(1.0f, 2.0f, -1.0f));
    TEST_EXPECT(FVector3(4.0f, 3.0f, 7.0f) - 5.0f == FVector3(-1.0f, -2.0f, 2.0f));
    {
        FVector3 Vec(4.0f, 3.0f, 7.0f);
        Vec -= FVector3(1.0f, 1.0f, 1.0f);
        Vec -= 1.0f;

        TEST_EXPECT(Vec == FVector3(2.0f, 1.0f, 5.0f));
    }

    TEST_SECTION(TEXT("FVector3::operator* / operator*="));
    TEST_EXPECT(FVector3(1.0f, 2.0f, -1.0f) * FVector3(3.0f, 1.0f, 2.0f) == FVector3(3.0f, 2.0f, -2.0f));
    TEST_EXPECT(FVector3(1.0f, 2.0f, -1.0f) * 5.0f == FVector3(5.0f, 10.0f, -5.0f));
    TEST_EXPECT(5.0f * FVector3(1.0f, 2.0f, -1.0f) == FVector3(5.0f, 10.0f, -5.0f));
    {
        FVector3 Vec(1.0f, 2.0f, 3.0f);
        Vec *= FVector3(2.0f, 2.0f, 2.0f);
        Vec *= 2.0f;

        TEST_EXPECT(Vec == FVector3(4.0f, 8.0f, 12.0f));
    }

    TEST_SECTION(TEXT("FVector3::operator/ / operator/="));
    TEST_EXPECT(FVector3(3.0f, 2.0f, -2.0f) / FVector3(3.0f, 1.0f, 2.0f) == FVector3(1.0f, 2.0f, -1.0f));
    TEST_EXPECT(FVector3(5.0f, 10.0f, -5.0f) / 5.0f == FVector3(1.0f, 2.0f, -1.0f));
    {
        FVector3 Vec(8.0f, 4.0f, 2.0f);
        Vec /= FVector3(2.0f, 2.0f, 2.0f);
        Vec /= 2.0f;

        TEST_EXPECT(Vec == FVector3(2.0f, 1.0f, 0.5f));
    }

    TEST_SECTION(TEXT("FVector3::operator== / operator!="));
    TEST_EXPECT(FVector3(1.0f, 2.0f, 3.0f) == FVector3(1.0f, 2.0f, 3.0f));
    TEST_EXPECT(FVector3(1.0f, 2.0f, 3.0f) != FVector3(1.0f, 2.0f, 4.0f));

    TEST_SECTION(TEXT("FVector3::operator[]"));
    {
        FVector3 Vec(1.0f, 2.0f, 3.0f);
        Vec[2] = -1.0f;

        TEST_EXPECT(Vec[0] == 1.0f);
        TEST_EXPECT(Vec[2] == -1.0f);
    }

    TEST_END();
}
