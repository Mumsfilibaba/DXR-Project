#include "MathTest.h"

#include <Core/Math/Quaternion.h>

bool TestQuaternion()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;
    const float PI     = Math::Constants::PI;

    const FQuaternion Rotation   = FQuaternion::FromAxisAngle(FVector3(0.0f, 0.0f, 1.0f), HalfPI);
    const FQuaternion Rotation180 = FQuaternion::FromAxisAngle(FVector3(0.0f, 0.0f, 1.0f), PI);

    TEST_SECTION(TEXT("FQuaternion::FQuaternion (default/identity)"));
    FQuaternion DefaultQuaternion;
    TEST_EXPECT(DefaultQuaternion.IsEqual(FQuaternion(0.0f, 0.0f, 0.0f, 1.0f)));
    TEST_EXPECT(FQuaternion::Identity.IsEqual(FQuaternion(0.0f, 0.0f, 0.0f, 1.0f)));

    TEST_SECTION(TEXT("FQuaternion::FQuaternion (FVector3, W)"));
    TEST_EXPECT(FQuaternion(FVector3(1.0f, 2.0f, 3.0f), 4.0f).IsEqual(FQuaternion(1.0f, 2.0f, 3.0f, 4.0f)));

    TEST_SECTION(TEXT("FQuaternion::GetLengthSquared / GetLength"));
    {
        const FQuaternion NonUnit(1.0f, 2.0f, 3.0f, 4.0f);
        TEST_EXPECT(Math::Abs(NonUnit.GetLengthSquared() - 30.0f) <= 1.0e-3f);
        TEST_EXPECT(Math::Abs(NonUnit.GetLength() - 5.47722557f) <= 1.0e-3f);
    }

    TEST_SECTION(TEXT("FQuaternion::Normalize / GetNormalized / IsUnit"));
    {
        const FQuaternion NonUnit(1.0f, 2.0f, 3.0f, 4.0f);
        TEST_EXPECT(!NonUnit.IsUnit());
        TEST_EXPECT(NonUnit.GetNormalized().IsUnit());

        FQuaternion Normalized = NonUnit;
        Normalized.Normalize();

        TEST_EXPECT(Normalized.IsUnit());
    }

    TEST_SECTION(TEXT("FQuaternion::IsEqual"));
    TEST_EXPECT(Rotation.IsEqual(Rotation));
    TEST_EXPECT(!Rotation.IsEqual(Rotation180));

    TEST_SECTION(TEXT("FQuaternion::DotProduct"));
    TEST_EXPECT(Math::Abs(FQuaternion::Identity.DotProduct(FQuaternion::Identity) - 1.0f) <= 1.0e-4f);
    TEST_EXPECT(Math::Abs(FQuaternion(1.0f, 2.0f, 3.0f, 4.0f).DotProduct(FQuaternion(1.0f, 2.0f, 3.0f, 4.0f)) - 30.0f) <= 1.0e-3f);

    TEST_SECTION(TEXT("FQuaternion::GetConjugated / GetInversed"));
    TEST_EXPECT(FQuaternion(1.0f, 2.0f, 3.0f, 4.0f).GetConjugated().IsEqual(FQuaternion(-1.0f, -2.0f, -3.0f, 4.0f)));
    TEST_EXPECT(Rotation.GetConjugated().IsEqual(Rotation.GetInversed()));
    TEST_EXPECT((Rotation * Rotation.GetInversed()).IsEqual(FQuaternion::Identity));

    TEST_SECTION(TEXT("FQuaternion::RotateVector"));
    TEST_EXPECT(Rotation.RotateVector(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(0.0f, 1.0f, 0.0f)));
    TEST_EXPECT(Rotation180.RotateVector(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(-1.0f, 0.0f, 0.0f)));

    TEST_SECTION(TEXT("FQuaternion::ToMatrix3 / FromRotationMatrix"));
    {
        const FQuaternion FromMatrix = FQuaternion::FromRotationMatrix(Rotation.ToMatrix3());
        TEST_EXPECT(FromMatrix.RotateVector(FVector3(1.0f, 2.0f, 3.0f)).IsEqual(Rotation.RotateVector(FVector3(1.0f, 2.0f, 3.0f))));
    }

    TEST_SECTION(TEXT("FQuaternion::ToMatrix4"));
    {
        const FMatrix3 M3 = Rotation.ToMatrix3();
        const FMatrix4 M4 = Rotation.ToMatrix4();
        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                TEST_EXPECT(Math::Abs(M4.M[Row][Col] - M3.M[Row][Col]) <= 1.0e-4f);
            }
        }

        TEST_EXPECT(Math::Abs(M4.M[3][3] - 1.0f) <= 1.0e-4f);
        TEST_EXPECT(Math::Abs(M4.M[3][0]) <= 1.0e-4f);
        TEST_EXPECT(Math::Abs(M4.M[0][3]) <= 1.0e-4f);
    }

    TEST_SECTION(TEXT("FQuaternion::FromEuler / ToEuler"));
    {
        const FVector3 Euler(0.3f, 0.5f, 0.2f);
        const FQuaternion FromVec   = FQuaternion::FromEuler(Euler);
        const FQuaternion FromFloat = FQuaternion::FromEuler(Euler.X, Euler.Y, Euler.Z);
        TEST_EXPECT(FromVec.IsEqual(FromFloat));
        TEST_EXPECT(FromVec.ToEuler().IsEqual(Euler));
    }

    TEST_SECTION(TEXT("FQuaternion::FromAxisAngle"));
    TEST_EXPECT(Rotation.IsEqual(FQuaternion(0.0f, 0.0f, 0.70710678f, 0.70710678f)));

    TEST_SECTION(TEXT("FQuaternion::Slerp"));
    TEST_EXPECT(FQuaternion::Slerp(FQuaternion::Identity, Rotation, 0.0f).RotateVector(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(1.0f, 0.0f, 0.0f)));
    TEST_EXPECT(FQuaternion::Slerp(FQuaternion::Identity, Rotation, 1.0f).RotateVector(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(0.0f, 1.0f, 0.0f)));
    TEST_EXPECT(FQuaternion::Slerp(FQuaternion::Identity, Rotation, 0.5f).RotateVector(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(0.70710678f, 0.70710678f, 0.0f)));

    TEST_SECTION(TEXT("FQuaternion::Nlerp"));
    {
        TEST_EXPECT(FQuaternion::Nlerp(FQuaternion::Identity, Rotation, 0.0f).RotateVector(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(1.0f, 0.0f, 0.0f)));
        TEST_EXPECT(FQuaternion::Nlerp(FQuaternion::Identity, Rotation, 1.0f).RotateVector(FVector3(1.0f, 0.0f, 0.0f)).IsEqual(FVector3(0.0f, 1.0f, 0.0f)));
        TEST_EXPECT(FQuaternion::Nlerp(FQuaternion::Identity, Rotation, 0.5f).IsUnit());
    }

    TEST_SECTION(TEXT("FQuaternion::operator* (quaternion)"));
    TEST_EXPECT((Rotation * FQuaternion::Identity).IsEqual(Rotation));
    {
        FQuaternion Accum = Rotation;
        Accum *= Rotation;

        TEST_EXPECT(Accum.IsEqual(Rotation180));
    }

    TEST_SECTION(TEXT("FQuaternion::operator* (scalar)"));
    TEST_EXPECT((FQuaternion(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f).IsEqual(FQuaternion(2.0f, 4.0f, 6.0f, 8.0f)));
    {
        FQuaternion Scaled(1.0f, 2.0f, 3.0f, 4.0f);
        Scaled *= 2.0f;

        TEST_EXPECT(Scaled.IsEqual(FQuaternion(2.0f, 4.0f, 6.0f, 8.0f)));
    }

    TEST_SECTION(TEXT("FQuaternion::operator-"));
    TEST_EXPECT((-FQuaternion(1.0f, 2.0f, 3.0f, 4.0f)).IsEqual(FQuaternion(-1.0f, -2.0f, -3.0f, -4.0f)));

    TEST_SECTION(TEXT("FQuaternion::operator[]"));
    {
        const FQuaternion Quat(1.0f, 2.0f, 3.0f, 4.0f);
        TEST_EXPECT(Quat[0] == 1.0f);
        TEST_EXPECT(Quat[1] == 2.0f);
        TEST_EXPECT(Quat[2] == 3.0f);
        TEST_EXPECT(Quat[3] == 4.0f);
    }

    TEST_END();
}
