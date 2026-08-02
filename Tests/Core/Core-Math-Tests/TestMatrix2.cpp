#include "MathTest.h"

#include <Core/Math/Matrix2.h>

bool TestMatrix2()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;
    const float PI     = Math::Constants::PI;

    TEST_SECTION("Matrix2::Identity / constructors");
    TEST_EXPECT(Matrix2::Identity().IsEqual(Matrix2(1.0f, 0.0f, 0.0f, 1.0f)));
    TEST_EXPECT(Matrix2(5.0f).IsEqual(Matrix2(5.0f, 0.0f, 0.0f, 5.0f)));
    TEST_EXPECT(Matrix2(Vector2(1.0f, 0.0f), Vector2(0.0f, 1.0f)).IsEqual(Matrix2::Identity()));

    TEST_SECTION("Matrix2::GetTranspose");
    TEST_EXPECT(Matrix2(1.0f, 2.0f, 3.0f, 4.0f).GetTranspose().IsEqual(Matrix2(1.0f, 3.0f, 2.0f, 4.0f)));

    TEST_SECTION("Matrix2::GetDeterminant");
    TEST_EXPECT(Matrix2(1.0f, 2.0f, 3.0f, 4.0f).GetDeterminant() == -2.0f);
    TEST_EXPECT(Matrix2::Scale(6.0f).GetDeterminant() == 36.0f);

    TEST_SECTION("Matrix2::GetAdjoint");
    TEST_EXPECT(Matrix2(1.0f, 2.0f, 3.0f, 4.0f).GetAdjoint().IsEqual(Matrix2(4.0f, -2.0f, -3.0f, 1.0f)));

    TEST_SECTION("Matrix2::GetInverse");
    {
        const Matrix2 Mult = Matrix2::Rotation(HalfPI) * Matrix2::Rotation(HalfPI);
        TEST_EXPECT((Mult * Mult.GetInverse()).IsEqual(Matrix2::Identity()));
    }

    TEST_SECTION("Matrix2::Scale");
    TEST_EXPECT(Matrix2::Scale(2.0f, 3.0f).IsEqual(Matrix2(2.0f, 0.0f, 0.0f, 3.0f)));
    TEST_EXPECT(Matrix2::Scale(Vector2(2.0f, 3.0f)).IsEqual(Matrix2(2.0f, 0.0f, 0.0f, 3.0f)));

    TEST_SECTION("Matrix2::Rotation / operator* (matrix)");
    TEST_EXPECT((Matrix2::Rotation(HalfPI) * Matrix2::Rotation(HalfPI)).IsEqual(Matrix2::Rotation(PI)));
    {
        Matrix2 Accum = Matrix2::Rotation(HalfPI);
        Accum *= Matrix2::Rotation(HalfPI);

        TEST_EXPECT(Accum.IsEqual(Matrix2::Rotation(PI)));
    }

    TEST_SECTION("Matrix2::operator* (vector)");
    TEST_EXPECT((Matrix2::Rotation(HalfPI) * Vector2(1.0f, 0.0f)).IsEqual(Vector2(0.0f, 1.0f)));

    TEST_SECTION("Matrix2::operator* (scalar)");
    TEST_EXPECT((Matrix2(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f).IsEqual(Matrix2(2.0f, 4.0f, 6.0f, 8.0f)));
    {
        Matrix2 Mat(1.0f, 2.0f, 3.0f, 4.0f);
        Mat *= 2.0f;

        TEST_EXPECT(Mat.IsEqual(Matrix2(2.0f, 4.0f, 6.0f, 8.0f)));
    }

    TEST_SECTION("Matrix2::operator+ (matrix/scalar)");
    TEST_EXPECT((Matrix2(1.0f) + Matrix2(1.0f)).IsEqual(Matrix2(2.0f)));
    TEST_EXPECT((Matrix2(0.0f) + 0.5f).IsEqual(Matrix2(0.5f, 0.5f, 0.5f, 0.5f)));
    {
        Matrix2 Mat(1.0f);
        Mat += Matrix2(1.0f);
        Mat += 1.0f;

        TEST_EXPECT(Mat.IsEqual(Matrix2(3.0f, 1.0f, 1.0f, 3.0f)));
    }

    TEST_SECTION("Matrix2::operator- (matrix/scalar)");
    TEST_EXPECT((Matrix2(1.0f) - Matrix2(1.0f)).IsEqual(Matrix2(0.0f)));
    TEST_EXPECT((Matrix2(1.0f) - 0.5f).IsEqual(Matrix2(0.5f, -0.5f, -0.5f, 0.5f)));
    {
        Matrix2 Mat(3.0f);
        Mat -= Matrix2(1.0f);
        Mat -= 1.0f;

        TEST_EXPECT(Mat.IsEqual(Matrix2(1.0f, -1.0f, -1.0f, 1.0f)));
    }

    TEST_SECTION("Matrix2::ContainsNaN / ContainsInfinity");
    {
        const Matrix2 NaN(1.0f, 0.0f, 0.0f, Math::Constants::NaN);
        const Matrix2 Inf(1.0f, 0.0f, 0.0f, Math::Constants::Infinity);
        TEST_EXPECT(NaN.ContainsNaN());
        TEST_EXPECT(Inf.ContainsInfinity());
        TEST_EXPECT(!Matrix2(1.0f).ContainsNaN());
        TEST_EXPECT(!Matrix2(1.0f).ContainsInfinity());
    }

    TEST_SECTION("Matrix2::IsEqual");
    TEST_EXPECT(Matrix2(1.0f, 2.0f, 3.0f, 4.0f).IsEqual(Matrix2(1.0f, 2.0f, 3.0f, 4.0f)));
    TEST_EXPECT(!Matrix2(1.0f, 2.0f, 3.0f, 4.0f).IsEqual(Matrix2(1.0f, 2.0f, 3.0f, 5.0f)));

    TEST_SECTION("Matrix2::GetRow / GetColumn");
    {
        const Matrix2 Sample(1.0f, 2.0f, 3.0f, 4.0f);
        TEST_EXPECT(Sample.GetRow(0) == Vector2(1.0f, 2.0f));
        TEST_EXPECT(Sample.GetRow(1) == Vector2(3.0f, 4.0f));
        TEST_EXPECT(Sample.GetColumn(0) == Vector2(1.0f, 3.0f));
        TEST_EXPECT(Sample.GetColumn(1) == Vector2(2.0f, 4.0f));
    }

    TEST_SECTION("Matrix2::SetIdentity");
    {
        Matrix2 Sample(1.0f, 2.0f, 3.0f, 4.0f);
        Sample.SetIdentity();

        TEST_EXPECT(Sample.IsEqual(Matrix2::Identity()));
    }

    TEST_END();
}
