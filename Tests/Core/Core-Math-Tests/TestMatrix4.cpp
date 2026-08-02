#include "MathTest.h"

#include <Core/Math/Matrix4.h>
#include <Core/Math/Matrix3.h>

bool TestMatrix4()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;
    const float PI     = Math::Constants::PI;

    const float Values[16] =
    {
         1.0f,  2.0f,  3.0f,  4.0f,
         5.0f,  6.0f,  7.0f,  8.0f,
         9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f,
    };
    
    const Matrix4 Sample(Values);

    TEST_SECTION("Matrix4::Identity / constructors");
    TEST_EXPECT(Matrix4::Identity() == Matrix4(1.0f));
    TEST_EXPECT(Sample.GetRow(0) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    TEST_EXPECT(Sample.GetColumn(0) == Vector4(1.0f, 5.0f, 9.0f, 13.0f));

    TEST_SECTION("Matrix4::GetTranspose");
    TEST_EXPECT(Sample.GetTranspose().GetTranspose() == Sample);
    TEST_EXPECT(Sample.GetTranspose().GetColumn(0) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));

    TEST_SECTION("Matrix4::GetDeterminant");
    TEST_EXPECT(Matrix4::Identity().GetDeterminant() == 1.0f);
    TEST_EXPECT(Matrix4::Scale(2.0f, 3.0f, 4.0f).GetDeterminant() == 24.0f);

    TEST_SECTION("Matrix4::GetInverse");
    {
        const Matrix4 Scale = Matrix4::Scale(2.0f, 3.0f, 4.0f);
        TEST_EXPECT((Scale * Scale.GetInverse()).IsEqual(Matrix4::Identity()));
    }

    TEST_SECTION("Matrix4::GetAdjugate");
    {
        const Matrix4 Scale = Matrix4::Scale(2.0f, 3.0f, 4.0f);
        TEST_EXPECT((Scale * Scale.GetAdjugate()).IsEqual(Matrix4(24.0f)));
    }

    TEST_SECTION("Matrix4::Scale");
    TEST_EXPECT(Matrix4::Scale(2.0f).IsEqual(Matrix4::Scale(2.0f, 2.0f, 2.0f)));
    TEST_EXPECT(Matrix4::Scale(Vector3(2.0f, 3.0f, 4.0f)).IsEqual(Matrix4::Scale(2.0f, 3.0f, 4.0f)));

    TEST_SECTION("Matrix4::Translation / GetTranslation");
    TEST_EXPECT(Matrix4::Translation(1.0f, 2.0f, 3.0f).GetTranslation() == Vector3(1.0f, 2.0f, 3.0f));
    TEST_EXPECT(Matrix4::Translation(Vector3(1.0f, 2.0f, 3.0f)).GetTranslation() == Vector3(1.0f, 2.0f, 3.0f));

    TEST_SECTION("Matrix4::RotationX/Y/Z");
    TEST_EXPECT((Matrix4::RotationX(HalfPI) * Matrix4::RotationX(HalfPI)).IsEqual(Matrix4::RotationX(PI)));
    TEST_EXPECT((Matrix4::RotationY(HalfPI) * Matrix4::RotationY(HalfPI)).IsEqual(Matrix4::RotationY(PI)));
    TEST_EXPECT((Matrix4::RotationZ(HalfPI) * Matrix4::RotationZ(HalfPI)).IsEqual(Matrix4::RotationZ(PI)));

    TEST_SECTION("Matrix4::RotationRollPitchYaw");
    TEST_EXPECT(Matrix4::RotationRollPitchYaw(0.0f, 0.0f, 0.0f).IsEqual(Matrix4::Identity()));
    TEST_EXPECT(Matrix4::RotationRollPitchYaw(Vector3(0.0f, 0.0f, 0.0f)).IsEqual(Matrix4::Identity()));

    TEST_SECTION("Matrix4::Transform (Vector4)");
    TEST_EXPECT(Matrix4::Identity().Transform(Vector4(1.0f, 2.0f, 3.0f, 4.0f)).IsEqual(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
    TEST_EXPECT(Matrix4::Scale(2.0f, 3.0f, 4.0f).Transform(Vector4(1.0f, 1.0f, 1.0f, 1.0f)).IsEqual(Vector4(2.0f, 3.0f, 4.0f, 1.0f)));

    TEST_SECTION("Matrix4::Transform / TransformCoord / TransformNormal (Vector3)");
    TEST_EXPECT(Matrix4::Translation(1.0f, 2.0f, 3.0f).Transform(Vector3(0.0f, 0.0f, 0.0f)).IsEqual(Vector3(1.0f, 2.0f, 3.0f)));
    TEST_EXPECT(Matrix4::Translation(1.0f, 2.0f, 3.0f).TransformCoord(Vector3(0.0f, 0.0f, 0.0f)).IsEqual(Vector3(1.0f, 2.0f, 3.0f)));
    TEST_EXPECT(Matrix4::Translation(1.0f, 2.0f, 3.0f).TransformNormal(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(1.0f, 0.0f, 0.0f)));

    TEST_SECTION("Matrix4::operator* (matrix/scalar)");
    {
        Matrix4 Accum = Matrix4::RotationZ(HalfPI);
        Accum *= Matrix4::RotationZ(HalfPI);

        TEST_EXPECT(Accum.IsEqual(Matrix4::RotationZ(PI)));
    }

    TEST_EXPECT((Matrix4(1.0f) * 2.0f).IsEqual(Matrix4(2.0f)));
    {
        Matrix4 Mat(1.0f);
        Mat *= 2.0f;

        TEST_EXPECT(Mat.IsEqual(Matrix4(2.0f)));
    }

    TEST_SECTION("Matrix4::operator+ / operator- / operator/");
    TEST_EXPECT((Matrix4(1.0f) + Matrix4(1.0f)).IsEqual(Matrix4(2.0f)));
    TEST_EXPECT((Matrix4(2.0f) - Matrix4(1.0f)).IsEqual(Matrix4(1.0f)));
    TEST_EXPECT((Matrix4(4.0f) / 2.0f).IsEqual(Matrix4(2.0f)));
    {
        Matrix4 Mat(1.0f);
        Mat += Matrix4(1.0f);
        Mat += 1.0f;
        Mat -= 1.0f;
        Mat -= Matrix4(1.0f);
        Mat /= 1.0f;

        TEST_EXPECT(Mat.IsEqual(Matrix4(1.0f)));
    }

    TEST_SECTION("Matrix4::operator== / operator!= / IsEqual");
    TEST_EXPECT(Sample == Sample);
    TEST_EXPECT(Sample != Matrix4::Identity());
    TEST_EXPECT(Sample.IsEqual(Sample));

    TEST_SECTION("Matrix4::SetIdentity / SetTranslation");
    {
        Matrix4 Mat = Sample;
        Mat.SetIdentity();

        TEST_EXPECT(Mat == Matrix4::Identity());

        Mat.SetTranslation(Vector3(1.0f, 2.0f, 3.0f));

        TEST_EXPECT(Mat.GetTranslation() == Vector3(1.0f, 2.0f, 3.0f));
    }

    TEST_SECTION("Matrix4::SetRotationAndScale / GetRotationAndScale");
    {
        Matrix4 Mat = Matrix4::Identity();
        Mat.SetRotationAndScale(Matrix3::Scale(2.0f, 3.0f, 4.0f));

        TEST_EXPECT(Mat.GetRotationAndScale().IsEqual(Matrix3::Scale(2.0f, 3.0f, 4.0f)));
    }

    TEST_SECTION("Matrix4::OrthoNormalize");
    {
        Matrix4 Mat = Matrix4::RotationZ(HalfPI);
        Mat.OrthoNormalize();

        TEST_EXPECT(Mat.IsEqual(Matrix4::RotationZ(HalfPI)));
    }

    TEST_SECTION("Matrix4::LookAt / LookTo (eye maps to origin)");
    {
        const Vector3 Eye(0.0f, 0.0f, -5.0f);
        const Vector3 At(0.0f, 0.0f, 0.0f);
        const Vector3 Up(0.0f, 1.0f, 0.0f);
        const Matrix4 View = Matrix4::LookAt(Eye, At, Up);
        TEST_EXPECT(View.TransformCoord(Eye).IsEqual(Vector3(0.0f, 0.0f, 0.0f)));

        const Matrix4 ViewTo = Matrix4::LookTo(Eye, (At - Eye).GetNormalized(), Up);
        TEST_EXPECT(ViewTo.TransformCoord(Eye).IsEqual(Vector3(0.0f, 0.0f, 0.0f)));
    }

    TEST_SECTION("Matrix4::Projections (well-formed)");
    {
        TEST_EXPECT(!Matrix4::OrthographicProjection(2.0f, 2.0f, 0.1f, 100.0f).ContainsNaN());
        TEST_EXPECT(!Matrix4::OrthographicProjection(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f).ContainsNaN());
        TEST_EXPECT(!Matrix4::PerspectiveProjection(HalfPI, 1.777f, 0.1f, 100.0f).ContainsNaN());
        TEST_EXPECT(!Matrix4::PerspectiveProjection(HalfPI, 1280.0f, 720.0f, 0.1f, 100.0f).ContainsNaN());
    }

    TEST_SECTION("Matrix4::ContainsNaN / ContainsInfinity");
    {
        Matrix4 NaN = Matrix4::Identity();
        NaN.SetTranslation(Vector3(0.0f, 0.0f, Math::Constants::NaN));
        Matrix4 Inf = Matrix4::Identity();
        Inf.SetTranslation(Vector3(0.0f, 0.0f, Math::Constants::Infinity));

        TEST_EXPECT(NaN.ContainsNaN());
        TEST_EXPECT(Inf.ContainsInfinity());
        TEST_EXPECT(!Matrix4::Identity().ContainsNaN());
        TEST_EXPECT(!Matrix4::Identity().ContainsInfinity());
    }

    TEST_END();
}
