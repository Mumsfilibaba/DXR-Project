#include "MathTest.h"

#include <Core/Math/Matrix3.h>

bool TestMatrix3()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;
    const float PI     = Math::Constants::PI;

    const FMatrix3 Sample(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);

    TEST_SECTION("Matrix3::Identity / constructors");
    TEST_EXPECT(FMatrix3::Identity() == FMatrix3(1.0f));
    TEST_EXPECT(FMatrix3(FVector3(1.0f, 0.0f, 0.0f), FVector3(0.0f, 1.0f, 0.0f), FVector3(0.0f, 0.0f, 1.0f)) == FMatrix3::Identity());

    TEST_SECTION("Matrix3::GetTranspose");
    TEST_EXPECT(Sample.GetTranspose() == FMatrix3(1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f));
    TEST_EXPECT(Sample.GetTranspose().GetTranspose() == Sample);

    TEST_SECTION("Matrix3::GetDeterminant");
    TEST_EXPECT(FMatrix3::Identity().GetDeterminant() == 1.0f);
    TEST_EXPECT(FMatrix3::Scale(2.0f, 3.0f, 4.0f).GetDeterminant() == 24.0f);

    TEST_SECTION("Matrix3::GetInverse");
    {
        const FMatrix3 Scale = FMatrix3::Scale(2.0f, 3.0f, 4.0f);
        TEST_EXPECT((Scale * Scale.GetInverse()).IsEqual(FMatrix3::Identity()));
    }

    TEST_SECTION("Matrix3::GetAdjugate");
    TEST_EXPECT(FMatrix3::Scale(2.0f, 3.0f, 4.0f).GetAdjugate().IsEqual(FMatrix3::Scale(12.0f, 8.0f, 6.0f)));

    TEST_SECTION("Matrix3::Scale");
    TEST_EXPECT(FMatrix3::Scale(2.0f).IsEqual(FMatrix3::Scale(2.0f, 2.0f, 2.0f)));
    TEST_EXPECT(FMatrix3::Scale(FVector3(2.0f, 3.0f, 4.0f)).IsEqual(FMatrix3::Scale(2.0f, 3.0f, 4.0f)));

    TEST_SECTION("Matrix3::RotationX/Y/Z");
    TEST_EXPECT((FMatrix3::RotationX(HalfPI) * FMatrix3::RotationX(HalfPI)).IsEqual(FMatrix3::RotationX(PI)));
    TEST_EXPECT((FMatrix3::RotationY(HalfPI) * FMatrix3::RotationY(HalfPI)).IsEqual(FMatrix3::RotationY(PI)));
    TEST_EXPECT((FMatrix3::RotationZ(HalfPI) * FMatrix3::RotationZ(HalfPI)).IsEqual(FMatrix3::RotationZ(PI)));

    TEST_SECTION("Matrix3::RotationRollPitchYaw");
    TEST_EXPECT(FMatrix3::RotationRollPitchYaw(0.0f, 0.0f, 0.0f).IsEqual(FMatrix3::Identity()));

    TEST_SECTION("Matrix3::operator* (vector)");
    TEST_EXPECT((FMatrix3::Identity() * FVector3(1.0f, 2.0f, 3.0f)).IsEqual(FVector3(1.0f, 2.0f, 3.0f)));
    TEST_EXPECT((FMatrix3::Scale(2.0f, 3.0f, 4.0f) * FVector3(1.0f, 1.0f, 1.0f)).IsEqual(FVector3(2.0f, 3.0f, 4.0f)));

    TEST_SECTION("Matrix3::operator* (matrix)");
    {
        FMatrix3 Accum = FMatrix3::RotationZ(HalfPI);
        Accum *= FMatrix3::RotationZ(HalfPI);

        TEST_EXPECT(Accum.IsEqual(FMatrix3::RotationZ(PI)));
    }

    TEST_SECTION("Matrix3::operator* (scalar)");
    TEST_EXPECT((FMatrix3(1.0f) * 2.0f).IsEqual(FMatrix3(2.0f)));
    {
        FMatrix3 Mat(1.0f);
        Mat *= 2.0f;

        TEST_EXPECT(Mat.IsEqual(FMatrix3(2.0f)));
    }

    TEST_SECTION("Matrix3::operator+ / operator-");
    TEST_EXPECT((FMatrix3(1.0f) + FMatrix3(1.0f)).IsEqual(FMatrix3(2.0f)));
    TEST_EXPECT((FMatrix3(2.0f) - FMatrix3(1.0f)).IsEqual(FMatrix3(1.0f)));
    {
        FMatrix3 Mat(1.0f);
        Mat += FMatrix3(1.0f);
        Mat += 1.0f;
        Mat -= 1.0f;
        Mat -= FMatrix3(1.0f);

        TEST_EXPECT(Mat.IsEqual(FMatrix3(1.0f)));
    }

    TEST_SECTION("Matrix3::operator/ (scalar)");
    TEST_EXPECT((FMatrix3(2.0f) / 2.0f).IsEqual(FMatrix3(1.0f)));
    {
        FMatrix3 Mat(4.0f);
        Mat /= 2.0f;

        TEST_EXPECT(Mat.IsEqual(FMatrix3(2.0f)));
    }

    TEST_SECTION("Matrix3::operator== / operator!= / IsEqual");
    TEST_EXPECT(Sample == Sample);
    TEST_EXPECT(Sample != FMatrix3::Identity());
    TEST_EXPECT(Sample.IsEqual(Sample));

    TEST_SECTION("Matrix3::GetRow / GetColumn");
    TEST_EXPECT(Sample.GetRow(0) == FVector3(1.0f, 2.0f, 3.0f));
    TEST_EXPECT(Sample.GetColumn(0) == FVector3(1.0f, 4.0f, 7.0f));

    TEST_SECTION("Matrix3::SetIdentity");
    {
        FMatrix3 ToReset = Sample;
        ToReset.SetIdentity();

        TEST_EXPECT(ToReset == FMatrix3::Identity());
    }

    TEST_SECTION("Matrix3::ContainsNaN / ContainsInfinity");
    {
        const FMatrix3 NaN(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, Math::Constants::NaN);
        const FMatrix3 Inf(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaN.ContainsNaN());
        TEST_EXPECT(Inf.ContainsInfinity());
        TEST_EXPECT(!FMatrix3::Identity().ContainsNaN());
        TEST_EXPECT(!FMatrix3::Identity().ContainsInfinity());
    }

    TEST_END();
}
