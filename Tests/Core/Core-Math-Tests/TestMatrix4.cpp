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
    
    const FMatrix4 Sample(Values);

    TEST_SECTION(TEXT("FMatrix4::Identity / constructors"));
    TEST_EXPECT(FMatrix4::Identity() == FMatrix4(1.0f));
    TEST_EXPECT(Sample.GetRow(0) == FVector4(1.0f, 2.0f, 3.0f, 4.0f));
    TEST_EXPECT(Sample.GetColumn(0) == FVector4(1.0f, 5.0f, 9.0f, 13.0f));

    TEST_SECTION(TEXT("FMatrix4::GetTranspose"));
    TEST_EXPECT(Sample.GetTranspose().GetTranspose() == Sample);
    TEST_EXPECT(Sample.GetTranspose().GetColumn(0) == FVector4(1.0f, 2.0f, 3.0f, 4.0f));

    TEST_SECTION(TEXT("FMatrix4::GetDeterminant"));
    TEST_EXPECT(FMatrix4::Identity().GetDeterminant() == 1.0f);
    TEST_EXPECT(FMatrix4::Scale(2.0f, 3.0f, 4.0f).GetDeterminant() == 24.0f);

    TEST_SECTION(TEXT("FMatrix4::GetInverse"));
    {
        const FMatrix4 Scale = FMatrix4::Scale(2.0f, 3.0f, 4.0f);
        TEST_EXPECT((Scale * Scale.GetInverse()).IsEqual(FMatrix4::Identity()));
    }

    TEST_SECTION(TEXT("FMatrix4::GetAdjugate"));
    {
        const FMatrix4 Scale = FMatrix4::Scale(2.0f, 3.0f, 4.0f);
        TEST_EXPECT((Scale * Scale.GetAdjugate()).IsEqual(FMatrix4(24.0f)));
    }

    TEST_SECTION(TEXT("FMatrix4::Scale"));
    TEST_EXPECT(FMatrix4::Scale(2.0f).IsEqual(FMatrix4::Scale(2.0f, 2.0f, 2.0f)));
    TEST_EXPECT(FMatrix4::Scale(FVector3(2.0f, 3.0f, 4.0f)).IsEqual(FMatrix4::Scale(2.0f, 3.0f, 4.0f)));

    TEST_SECTION(TEXT("FMatrix4::Translation / GetTranslation"));
    TEST_EXPECT(FMatrix4::Translation(1.0f, 2.0f, 3.0f).GetTranslation() == FVector3(1.0f, 2.0f, 3.0f));
    TEST_EXPECT(FMatrix4::Translation(FVector3(1.0f, 2.0f, 3.0f)).GetTranslation() == FVector3(1.0f, 2.0f, 3.0f));

    TEST_SECTION(TEXT("FMatrix4::RotationX/Y/Z"));
    TEST_EXPECT((FMatrix4::RotationX(HalfPI) * FMatrix4::RotationX(HalfPI)).IsEqual(FMatrix4::RotationX(PI)));
    TEST_EXPECT((FMatrix4::RotationY(HalfPI) * FMatrix4::RotationY(HalfPI)).IsEqual(FMatrix4::RotationY(PI)));
    TEST_EXPECT((FMatrix4::RotationZ(HalfPI) * FMatrix4::RotationZ(HalfPI)).IsEqual(FMatrix4::RotationZ(PI)));

    TEST_SECTION(TEXT("FMatrix4::RotationRollPitchYaw"));
    TEST_EXPECT(FMatrix4::RotationRollPitchYaw(0.0f, 0.0f, 0.0f).IsEqual(FMatrix4::Identity()));
    TEST_EXPECT(FMatrix4::RotationRollPitchYaw(FVector3(0.0f, 0.0f, 0.0f)).IsEqual(FMatrix4::Identity()));

    TEST_SECTION(TEXT("FMatrix4::Transform (FVector4)"));
    TEST_EXPECT(FMatrix4::Identity().Transform(FVector4(1.0f, 2.0f, 3.0f, 4.0f)).IsEqual(FVector4(1.0f, 2.0f, 3.0f, 4.0f)));
    TEST_EXPECT(FMatrix4::Scale(2.0f, 3.0f, 4.0f).Transform(FVector4(1.0f, 1.0f, 1.0f, 1.0f)).IsEqual(FVector4(2.0f, 3.0f, 4.0f, 1.0f)));

    TEST_SECTION(TEXT("FMatrix4::Transform / TransformCoord / TransformNormal (FVector3)"));
    TEST_EXPECT(FMatrix4::Translation(1.0f, 2.0f, 3.0f).Transform(FVector3(0.0f, 0.0f, 0.0f)).IsEqual(FVector3(1.0f, 2.0f, 3.0f)));
    TEST_EXPECT(FMatrix4::Translation(1.0f, 2.0f, 3.0f).TransformCoord(FVector3(0.0f, 0.0f, 0.0f)).IsEqual(FVector3(1.0f, 2.0f, 3.0f)));
    TEST_EXPECT(FMatrix4::Translation(1.0f, 2.0f, 3.0f).TransformNormal(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(1.0f, 0.0f, 0.0f)));

    TEST_SECTION(TEXT("FMatrix4::operator* (matrix/scalar)"));
    {
        FMatrix4 Accum = FMatrix4::RotationZ(HalfPI);
        Accum *= FMatrix4::RotationZ(HalfPI);

        TEST_EXPECT(Accum.IsEqual(FMatrix4::RotationZ(PI)));
    }

    TEST_EXPECT((FMatrix4(1.0f) * 2.0f).IsEqual(FMatrix4(2.0f)));
    {
        FMatrix4 Mat(1.0f);
        Mat *= 2.0f;

        TEST_EXPECT(Mat.IsEqual(FMatrix4(2.0f)));
    }

    TEST_SECTION(TEXT("FMatrix4::operator+ / operator- / operator/"));
    TEST_EXPECT((FMatrix4(1.0f) + FMatrix4(1.0f)).IsEqual(FMatrix4(2.0f)));
    TEST_EXPECT((FMatrix4(2.0f) - FMatrix4(1.0f)).IsEqual(FMatrix4(1.0f)));
    TEST_EXPECT((FMatrix4(4.0f) / 2.0f).IsEqual(FMatrix4(2.0f)));
    {
        FMatrix4 Mat(1.0f);
        Mat += FMatrix4(1.0f);
        Mat += 1.0f;
        Mat -= 1.0f;
        Mat -= FMatrix4(1.0f);
        Mat /= 1.0f;

        TEST_EXPECT(Mat.IsEqual(FMatrix4(1.0f)));
    }

    TEST_SECTION(TEXT("FMatrix4::operator== / operator!= / IsEqual"));
    TEST_EXPECT(Sample == Sample);
    TEST_EXPECT(Sample != FMatrix4::Identity());
    TEST_EXPECT(Sample.IsEqual(Sample));

    TEST_SECTION(TEXT("FMatrix4::SetIdentity / SetTranslation"));
    {
        FMatrix4 Mat = Sample;
        Mat.SetIdentity();

        TEST_EXPECT(Mat == FMatrix4::Identity());

        Mat.SetTranslation(FVector3(1.0f, 2.0f, 3.0f));

        TEST_EXPECT(Mat.GetTranslation() == FVector3(1.0f, 2.0f, 3.0f));
    }

    TEST_SECTION(TEXT("FMatrix4::SetRotationAndScale / GetRotationAndScale"));
    {
        FMatrix4 Mat = FMatrix4::Identity();
        Mat.SetRotationAndScale(FMatrix3::Scale(2.0f, 3.0f, 4.0f));

        TEST_EXPECT(Mat.GetRotationAndScale().IsEqual(FMatrix3::Scale(2.0f, 3.0f, 4.0f)));
    }

    TEST_SECTION(TEXT("FMatrix4::OrthoNormalize"));
    {
        FMatrix4 Mat = FMatrix4::RotationZ(HalfPI);
        Mat.OrthoNormalize();

        TEST_EXPECT(Mat.IsEqual(FMatrix4::RotationZ(HalfPI)));
    }

    TEST_SECTION(TEXT("FMatrix4::LookAt / LookTo (eye maps to origin)"));
    {
        const FVector3 Eye(0.0f, 0.0f, -5.0f);
        const FVector3 At(0.0f, 0.0f, 0.0f);
        const FVector3 Up(0.0f, 1.0f, 0.0f);
        const FMatrix4 View = FMatrix4::LookAt(Eye, At, Up);
        TEST_EXPECT(View.TransformCoord(Eye).IsEqual(FVector3(0.0f, 0.0f, 0.0f)));

        const FMatrix4 ViewTo = FMatrix4::LookTo(Eye, (At - Eye).GetNormalized(), Up);
        TEST_EXPECT(ViewTo.TransformCoord(Eye).IsEqual(FVector3(0.0f, 0.0f, 0.0f)));
    }

    TEST_SECTION(TEXT("FMatrix4::Projections (well-formed)"));
    {
        TEST_EXPECT(!FMatrix4::OrthographicProjection(2.0f, 2.0f, 0.1f, 100.0f).ContainsNaN());
        TEST_EXPECT(!FMatrix4::OrthographicProjection(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f).ContainsNaN());
        TEST_EXPECT(!FMatrix4::PerspectiveProjection(HalfPI, 1.777f, 0.1f, 100.0f).ContainsNaN());
        TEST_EXPECT(!FMatrix4::PerspectiveProjection(HalfPI, 1280.0f, 720.0f, 0.1f, 100.0f).ContainsNaN());
    }

    TEST_SECTION(TEXT("FMatrix4::ContainsNaN / ContainsInfinity"));
    {
        FMatrix4 NaN = FMatrix4::Identity();
        NaN.SetTranslation(FVector3(0.0f, 0.0f, Math::Constants::NaN));
        FMatrix4 Inf = FMatrix4::Identity();
        Inf.SetTranslation(FVector3(0.0f, 0.0f, Math::Constants::Infinity));

        TEST_EXPECT(NaN.ContainsNaN());
        TEST_EXPECT(Inf.ContainsInfinity());
        TEST_EXPECT(!FMatrix4::Identity().ContainsNaN());
        TEST_EXPECT(!FMatrix4::Identity().ContainsInfinity());
    }

    TEST_END();
}
