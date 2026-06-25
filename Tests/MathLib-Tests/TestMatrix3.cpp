#include "MathTest.h"

#include <Core/Math/Matrix3.h>

bool TestMatrix3()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;
    const float PI     = Math::Constants::PI;

    const Matrix3 Sample(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);

    TEST_SECTION("Matrix3::Identity / constructors");
    TEST_EXPECT(Matrix3::Identity() == Matrix3(1.0f));
    TEST_EXPECT(Matrix3(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f)) == Matrix3::Identity());

    TEST_SECTION("Matrix3::GetTranspose");
    TEST_EXPECT(Sample.GetTranspose() == Matrix3(1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f));
    TEST_EXPECT(Sample.GetTranspose().GetTranspose() == Sample);

    TEST_SECTION("Matrix3::GetDeterminant");
    TEST_EXPECT(Matrix3::Identity().GetDeterminant() == 1.0f);
    TEST_EXPECT(Matrix3::Scale(2.0f, 3.0f, 4.0f).GetDeterminant() == 24.0f);

    TEST_SECTION("Matrix3::GetInverse");
    {
        const Matrix3 Scale = Matrix3::Scale(2.0f, 3.0f, 4.0f);
        TEST_EXPECT((Scale * Scale.GetInverse()).IsEqual(Matrix3::Identity()));
    }

    TEST_SECTION("Matrix3::GetAdjugate");
    TEST_EXPECT(Matrix3::Scale(2.0f, 3.0f, 4.0f).GetAdjugate().IsEqual(Matrix3::Scale(12.0f, 8.0f, 6.0f)));

    TEST_SECTION("Matrix3::Scale");
    TEST_EXPECT(Matrix3::Scale(2.0f).IsEqual(Matrix3::Scale(2.0f, 2.0f, 2.0f)));
    TEST_EXPECT(Matrix3::Scale(Vector3(2.0f, 3.0f, 4.0f)).IsEqual(Matrix3::Scale(2.0f, 3.0f, 4.0f)));

    TEST_SECTION("Matrix3::RotationX/Y/Z");
    TEST_EXPECT((Matrix3::RotationX(HalfPI) * Matrix3::RotationX(HalfPI)).IsEqual(Matrix3::RotationX(PI)));
    TEST_EXPECT((Matrix3::RotationY(HalfPI) * Matrix3::RotationY(HalfPI)).IsEqual(Matrix3::RotationY(PI)));
    TEST_EXPECT((Matrix3::RotationZ(HalfPI) * Matrix3::RotationZ(HalfPI)).IsEqual(Matrix3::RotationZ(PI)));

    TEST_SECTION("Matrix3::RotationRollPitchYaw");
    TEST_EXPECT(Matrix3::RotationRollPitchYaw(0.0f, 0.0f, 0.0f).IsEqual(Matrix3::Identity()));

    TEST_SECTION("Matrix3::operator* (vector)");
    TEST_EXPECT((Matrix3::Identity() * Vector3(1.0f, 2.0f, 3.0f)).IsEqual(Vector3(1.0f, 2.0f, 3.0f)));
    TEST_EXPECT((Matrix3::Scale(2.0f, 3.0f, 4.0f) * Vector3(1.0f, 1.0f, 1.0f)).IsEqual(Vector3(2.0f, 3.0f, 4.0f)));

    TEST_SECTION("Matrix3::operator* (matrix)");
    {
        Matrix3 Accum = Matrix3::RotationZ(HalfPI);
        Accum *= Matrix3::RotationZ(HalfPI);

        TEST_EXPECT(Accum.IsEqual(Matrix3::RotationZ(PI)));
    }

    TEST_SECTION("Matrix3::operator* (scalar)");
    TEST_EXPECT((Matrix3(1.0f) * 2.0f).IsEqual(Matrix3(2.0f)));
    {
        Matrix3 Mat(1.0f);
        Mat *= 2.0f;

        TEST_EXPECT(Mat.IsEqual(Matrix3(2.0f)));
    }

    TEST_SECTION("Matrix3::operator+ / operator-");
    TEST_EXPECT((Matrix3(1.0f) + Matrix3(1.0f)).IsEqual(Matrix3(2.0f)));
    TEST_EXPECT((Matrix3(2.0f) - Matrix3(1.0f)).IsEqual(Matrix3(1.0f)));
    {
        Matrix3 Mat(1.0f);
        Mat += Matrix3(1.0f);
        Mat += 1.0f;
        Mat -= 1.0f;
        Mat -= Matrix3(1.0f);

        TEST_EXPECT(Mat.IsEqual(Matrix3(1.0f)));
    }

    TEST_SECTION("Matrix3::operator/ (scalar)");
    TEST_EXPECT((Matrix3(2.0f) / 2.0f).IsEqual(Matrix3(1.0f)));
    {
        Matrix3 Mat(4.0f);
        Mat /= 2.0f;

        TEST_EXPECT(Mat.IsEqual(Matrix3(2.0f)));
    }

    TEST_SECTION("Matrix3::operator== / operator!= / IsEqual");
    TEST_EXPECT(Sample == Sample);
    TEST_EXPECT(Sample != Matrix3::Identity());
    TEST_EXPECT(Sample.IsEqual(Sample));

    TEST_SECTION("Matrix3::GetRow / GetColumn");
    TEST_EXPECT(Sample.GetRow(0) == Vector3(1.0f, 2.0f, 3.0f));
    TEST_EXPECT(Sample.GetColumn(0) == Vector3(1.0f, 4.0f, 7.0f));

    TEST_SECTION("Matrix3::SetIdentity");
    {
        Matrix3 ToReset = Sample;
        ToReset.SetIdentity();

        TEST_EXPECT(ToReset == Matrix3::Identity());
    }

    TEST_SECTION("Matrix3::ContainsNaN / ContainsInfinity");
    {
        const Matrix3 NaN(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, Math::Constants::NaN);
        const Matrix3 Inf(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaN.ContainsNaN());
        TEST_EXPECT(Inf.ContainsInfinity());
        TEST_EXPECT(!Matrix3::Identity().ContainsNaN());
        TEST_EXPECT(!Matrix3::Identity().ContainsInfinity());
    }

    TEST_END();
}
