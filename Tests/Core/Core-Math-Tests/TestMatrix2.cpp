#include "MathTest.h"

#include <Core/Math/Matrix2.h>

bool TestMatrix2()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;
    const float PI     = Math::Constants::PI;

    TEST_SECTION(TEXT("FMatrix2::Identity / constructors"));
    TEST_EXPECT(FMatrix2::Identity().IsEqual(FMatrix2(1.0f, 0.0f, 0.0f, 1.0f)));
    TEST_EXPECT(FMatrix2(5.0f).IsEqual(FMatrix2(5.0f, 0.0f, 0.0f, 5.0f)));
    TEST_EXPECT(FMatrix2(FVector2(1.0f, 0.0f), FVector2(0.0f, 1.0f)).IsEqual(FMatrix2::Identity()));

    TEST_SECTION(TEXT("FMatrix2::GetTranspose"));
    TEST_EXPECT(FMatrix2(1.0f, 2.0f, 3.0f, 4.0f).GetTranspose().IsEqual(FMatrix2(1.0f, 3.0f, 2.0f, 4.0f)));

    TEST_SECTION(TEXT("FMatrix2::GetDeterminant"));
    TEST_EXPECT(FMatrix2(1.0f, 2.0f, 3.0f, 4.0f).GetDeterminant() == -2.0f);
    TEST_EXPECT(FMatrix2::Scale(6.0f).GetDeterminant() == 36.0f);

    TEST_SECTION(TEXT("FMatrix2::GetAdjoint"));
    TEST_EXPECT(FMatrix2(1.0f, 2.0f, 3.0f, 4.0f).GetAdjoint().IsEqual(FMatrix2(4.0f, -2.0f, -3.0f, 1.0f)));

    TEST_SECTION(TEXT("FMatrix2::GetInverse"));
    {
        const FMatrix2 Mult = FMatrix2::Rotation(HalfPI) * FMatrix2::Rotation(HalfPI);
        TEST_EXPECT((Mult * Mult.GetInverse()).IsEqual(FMatrix2::Identity()));
    }

    TEST_SECTION(TEXT("FMatrix2::Scale"));
    TEST_EXPECT(FMatrix2::Scale(2.0f, 3.0f).IsEqual(FMatrix2(2.0f, 0.0f, 0.0f, 3.0f)));
    TEST_EXPECT(FMatrix2::Scale(FVector2(2.0f, 3.0f)).IsEqual(FMatrix2(2.0f, 0.0f, 0.0f, 3.0f)));

    TEST_SECTION(TEXT("FMatrix2::Rotation / operator* (matrix)"));
    TEST_EXPECT((FMatrix2::Rotation(HalfPI) * FMatrix2::Rotation(HalfPI)).IsEqual(FMatrix2::Rotation(PI)));
    {
        FMatrix2 Accum = FMatrix2::Rotation(HalfPI);
        Accum *= FMatrix2::Rotation(HalfPI);

        TEST_EXPECT(Accum.IsEqual(FMatrix2::Rotation(PI)));
    }

    TEST_SECTION(TEXT("FMatrix2::operator* (vector)"));
    TEST_EXPECT((FMatrix2::Rotation(HalfPI) * FVector2(1.0f, 0.0f)).IsEqual(FVector2(0.0f, 1.0f)));

    TEST_SECTION(TEXT("FMatrix2::operator* (scalar)"));
    TEST_EXPECT((FMatrix2(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f).IsEqual(FMatrix2(2.0f, 4.0f, 6.0f, 8.0f)));
    {
        FMatrix2 Mat(1.0f, 2.0f, 3.0f, 4.0f);
        Mat *= 2.0f;

        TEST_EXPECT(Mat.IsEqual(FMatrix2(2.0f, 4.0f, 6.0f, 8.0f)));
    }

    TEST_SECTION(TEXT("FMatrix2::operator+ (matrix/scalar)"));
    TEST_EXPECT((FMatrix2(1.0f) + FMatrix2(1.0f)).IsEqual(FMatrix2(2.0f)));
    TEST_EXPECT((FMatrix2(0.0f) + 0.5f).IsEqual(FMatrix2(0.5f, 0.5f, 0.5f, 0.5f)));
    {
        FMatrix2 Mat(1.0f);
        Mat += FMatrix2(1.0f);
        Mat += 1.0f;

        TEST_EXPECT(Mat.IsEqual(FMatrix2(3.0f, 1.0f, 1.0f, 3.0f)));
    }

    TEST_SECTION(TEXT("FMatrix2::operator- (matrix/scalar)"));
    TEST_EXPECT((FMatrix2(1.0f) - FMatrix2(1.0f)).IsEqual(FMatrix2(0.0f)));
    TEST_EXPECT((FMatrix2(1.0f) - 0.5f).IsEqual(FMatrix2(0.5f, -0.5f, -0.5f, 0.5f)));
    {
        FMatrix2 Mat(3.0f);
        Mat -= FMatrix2(1.0f);
        Mat -= 1.0f;

        TEST_EXPECT(Mat.IsEqual(FMatrix2(1.0f, -1.0f, -1.0f, 1.0f)));
    }

    TEST_SECTION(TEXT("FMatrix2::ContainsNaN / ContainsInfinity"));
    {
        const FMatrix2 NaN(1.0f, 0.0f, 0.0f, Math::Constants::NaN);
        const FMatrix2 Inf(1.0f, 0.0f, 0.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaN.ContainsNaN());
        TEST_EXPECT(Inf.ContainsInfinity());
        TEST_EXPECT(!FMatrix2(1.0f).ContainsNaN());
        TEST_EXPECT(!FMatrix2(1.0f).ContainsInfinity());
    }

    TEST_SECTION(TEXT("FMatrix2::IsEqual"));
    TEST_EXPECT(FMatrix2(1.0f, 2.0f, 3.0f, 4.0f).IsEqual(FMatrix2(1.0f, 2.0f, 3.0f, 4.0f)));
    TEST_EXPECT(!FMatrix2(1.0f, 2.0f, 3.0f, 4.0f).IsEqual(FMatrix2(1.0f, 2.0f, 3.0f, 5.0f)));

    TEST_SECTION(TEXT("FMatrix2::GetRow / GetColumn"));
    {
        const FMatrix2 Sample(1.0f, 2.0f, 3.0f, 4.0f);
        TEST_EXPECT(Sample.GetRow(0) == FVector2(1.0f, 2.0f));
        TEST_EXPECT(Sample.GetRow(1) == FVector2(3.0f, 4.0f));
        TEST_EXPECT(Sample.GetColumn(0) == FVector2(1.0f, 3.0f));
        TEST_EXPECT(Sample.GetColumn(1) == FVector2(2.0f, 4.0f));
    }

    TEST_SECTION(TEXT("FMatrix2::SetIdentity"));
    {
        FMatrix2 Sample(1.0f, 2.0f, 3.0f, 4.0f);
        Sample.SetIdentity();

        TEST_EXPECT(Sample.IsEqual(FMatrix2::Identity()));
    }

    TEST_END();
}
