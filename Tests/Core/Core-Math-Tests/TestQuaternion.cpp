#include "MathTest.h"

#include <Core/Math/Quaternion.h>

bool TestQuaternion()
{
    TEST_BEGIN();

    const float HalfPI = Math::Constants::HalfPI;
    const float PI     = Math::Constants::PI;

    const Quaternion Rotation   = Quaternion::FromAxisAngle(Vector3(0.0f, 0.0f, 1.0f), HalfPI);
    const Quaternion Rotation180 = Quaternion::FromAxisAngle(Vector3(0.0f, 0.0f, 1.0f), PI);

    TEST_SECTION("Quaternion::Quaternion (default/identity)");
    Quaternion DefaultQuaternion;
    TEST_EXPECT(DefaultQuaternion.IsEqual(Quaternion(0.0f, 0.0f, 0.0f, 1.0f)));
    TEST_EXPECT(Quaternion::Identity.IsEqual(Quaternion(0.0f, 0.0f, 0.0f, 1.0f)));

    TEST_SECTION("Quaternion::Quaternion (Vector3, W)");
    TEST_EXPECT(Quaternion(Vector3(1.0f, 2.0f, 3.0f), 4.0f).IsEqual(Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));

    TEST_SECTION("Quaternion::GetLengthSquared / GetLength");
    {
        const Quaternion NonUnit(1.0f, 2.0f, 3.0f, 4.0f);
        TEST_EXPECT(Math::Abs(NonUnit.GetLengthSquared() - 30.0f) <= 1.0e-3f);
        TEST_EXPECT(Math::Abs(NonUnit.GetLength() - 5.47722557f) <= 1.0e-3f);
    }

    TEST_SECTION("Quaternion::Normalize / GetNormalized / IsUnit");
    {
        const Quaternion NonUnit(1.0f, 2.0f, 3.0f, 4.0f);
        TEST_EXPECT(!NonUnit.IsUnit());
        TEST_EXPECT(NonUnit.GetNormalized().IsUnit());

        Quaternion Normalized = NonUnit;
        Normalized.Normalize();

        TEST_EXPECT(Normalized.IsUnit());
    }

    TEST_SECTION("Quaternion::IsEqual");
    TEST_EXPECT(Rotation.IsEqual(Rotation));
    TEST_EXPECT(!Rotation.IsEqual(Rotation180));

    TEST_SECTION("Quaternion::DotProduct");
    TEST_EXPECT(Math::Abs(Quaternion::Identity.DotProduct(Quaternion::Identity) - 1.0f) <= 1.0e-4f);
    TEST_EXPECT(Math::Abs(Quaternion(1.0f, 2.0f, 3.0f, 4.0f).DotProduct(Quaternion(1.0f, 2.0f, 3.0f, 4.0f)) - 30.0f) <= 1.0e-3f);

    TEST_SECTION("Quaternion::GetConjugated / GetInversed");
    TEST_EXPECT(Quaternion(1.0f, 2.0f, 3.0f, 4.0f).GetConjugated().IsEqual(Quaternion(-1.0f, -2.0f, -3.0f, 4.0f)));
    TEST_EXPECT(Rotation.GetConjugated().IsEqual(Rotation.GetInversed()));
    TEST_EXPECT((Rotation * Rotation.GetInversed()).IsEqual(Quaternion::Identity));

    TEST_SECTION("Quaternion::RotateVector");
    TEST_EXPECT(Rotation.RotateVector(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(0.0f, 1.0f, 0.0f)));
    TEST_EXPECT(Rotation180.RotateVector(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(-1.0f, 0.0f, 0.0f)));

    TEST_SECTION("Quaternion::ToMatrix3 / FromRotationMatrix");
    {
        const Quaternion FromMatrix = Quaternion::FromRotationMatrix(Rotation.ToMatrix3());
        TEST_EXPECT(FromMatrix.RotateVector(Vector3(1.0f, 2.0f, 3.0f)).IsEqual(Rotation.RotateVector(Vector3(1.0f, 2.0f, 3.0f))));
    }

    TEST_SECTION("Quaternion::ToMatrix4");
    {
        const Matrix3 M3 = Rotation.ToMatrix3();
        const Matrix4 M4 = Rotation.ToMatrix4();
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

    TEST_SECTION("Quaternion::FromEuler / ToEuler");
    {
        const Vector3 Euler(0.3f, 0.5f, 0.2f);
        const Quaternion FromVec   = Quaternion::FromEuler(Euler);
        const Quaternion FromFloat = Quaternion::FromEuler(Euler.X, Euler.Y, Euler.Z);
        TEST_EXPECT(FromVec.IsEqual(FromFloat));
        TEST_EXPECT(FromVec.ToEuler().IsEqual(Euler));
    }

    TEST_SECTION("Quaternion::FromAxisAngle");
    TEST_EXPECT(Rotation.IsEqual(Quaternion(0.0f, 0.0f, 0.70710678f, 0.70710678f)));

    TEST_SECTION("Quaternion::Slerp");
    TEST_EXPECT(Quaternion::Slerp(Quaternion::Identity, Rotation, 0.0f).RotateVector(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(1.0f, 0.0f, 0.0f)));
    TEST_EXPECT(Quaternion::Slerp(Quaternion::Identity, Rotation, 1.0f).RotateVector(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(0.0f, 1.0f, 0.0f)));
    TEST_EXPECT(Quaternion::Slerp(Quaternion::Identity, Rotation, 0.5f).RotateVector(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(0.70710678f, 0.70710678f, 0.0f)));

    TEST_SECTION("Quaternion::Nlerp");
    {
        TEST_EXPECT(Quaternion::Nlerp(Quaternion::Identity, Rotation, 0.0f).RotateVector(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(1.0f, 0.0f, 0.0f)));
        TEST_EXPECT(Quaternion::Nlerp(Quaternion::Identity, Rotation, 1.0f).RotateVector(Vector3(1.0f, 0.0f, 0.0f)).IsEqual(Vector3(0.0f, 1.0f, 0.0f)));
        TEST_EXPECT(Quaternion::Nlerp(Quaternion::Identity, Rotation, 0.5f).IsUnit());
    }

    TEST_SECTION("Quaternion::operator* (quaternion)");
    TEST_EXPECT((Rotation * Quaternion::Identity).IsEqual(Rotation));
    {
        Quaternion Accum = Rotation;
        Accum *= Rotation;

        TEST_EXPECT(Accum.IsEqual(Rotation180));
    }

    TEST_SECTION("Quaternion::operator* (scalar)");
    TEST_EXPECT((Quaternion(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f).IsEqual(Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
    {
        Quaternion Scaled(1.0f, 2.0f, 3.0f, 4.0f);
        Scaled *= 2.0f;

        TEST_EXPECT(Scaled.IsEqual(Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
    }

    TEST_SECTION("Quaternion::operator-");
    TEST_EXPECT((-Quaternion(1.0f, 2.0f, 3.0f, 4.0f)).IsEqual(Quaternion(-1.0f, -2.0f, -3.0f, -4.0f)));

    TEST_SECTION("Quaternion::operator[]");
    {
        const Quaternion Quat(1.0f, 2.0f, 3.0f, 4.0f);
        TEST_EXPECT(Quat[0] == 1.0f);
        TEST_EXPECT(Quat[1] == 2.0f);
        TEST_EXPECT(Quat[2] == 3.0f);
        TEST_EXPECT(Quat[3] == 4.0f);
    }

    TEST_END();
}
