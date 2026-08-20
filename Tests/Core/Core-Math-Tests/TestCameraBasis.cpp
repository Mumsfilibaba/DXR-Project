#include "MathTest.h"

#include <Core/Math/Matrix4.h>
#include <Core/Math/Vector3.h>

struct FCameraBasis
{
    Vector3 Forward;
    Vector3 Right;
    Vector3 Up;
};

static FCameraBasis BuildCameraBasis(float PitchRadians, float YawRadians)
{
    FCameraBasis Basis;

    const float CosP = Math::Cos(PitchRadians);
    const float SinP = Math::Sin(PitchRadians);
    const float CosY = Math::Cos(YawRadians);
    const float SinY = Math::Sin(YawRadians);

    Basis.Forward = Vector3(CosP * SinY, -SinP, CosP * CosY);
    Basis.Forward.Normalize();

    Basis.Right = Basis.Forward.CrossProduct(Vector3::Up);
    const float RightLengthSquared = Basis.Right.GetLengthSquared();
    if (RightLengthSquared > 1.0e-6f)
    {
        Basis.Right /= Math::Sqrt(RightLengthSquared);
    }
    else
    {
        Basis.Right = Vector3(-CosY, 0.0f, SinY);
        Basis.Right.Normalize();
    }

    Basis.Up = Basis.Right.CrossProduct(Basis.Forward);
    Basis.Up.Normalize();

    return Basis;
}

static bool IsOrthonormalBasis(const FCameraBasis& Basis, float Tolerance)
{
    const auto IsNearUnit = [Tolerance](const Vector3& Vector)
    {
        return Math::Abs(Vector.GetLengthSquared() - 1.0f) <= Tolerance;
    };

    return IsNearUnit(Basis.Forward) &&
           IsNearUnit(Basis.Right) &&
           IsNearUnit(Basis.Up) &&
           Math::Abs(Basis.Forward.DotProduct(Basis.Right)) <= Tolerance &&
           Math::Abs(Basis.Forward.DotProduct(Basis.Up)) <= Tolerance &&
           Math::Abs(Basis.Right.DotProduct(Basis.Up)) <= Tolerance;
}

bool TestCameraBasis()
{
    TEST_BEGIN();

    constexpr float Tolerance = 1.0e-4f;

    TEST_SECTION("Camera basis matches identity orientation");
    {
        const FCameraBasis Basis = BuildCameraBasis(0.0f, 0.0f);

        TEST_EXPECT(Basis.Forward.IsEqual(Vector3::Forward, Tolerance));
        TEST_EXPECT(Basis.Right.IsEqual(Vector3(-1.0f, 0.0f, 0.0f), Tolerance));
        TEST_EXPECT(Basis.Up.IsEqual(Vector3::Up, Tolerance));
    }

    TEST_SECTION("Camera basis matches RotationRollPitchYaw forward");
    {
        const float Pitch = Math::DegreesToRadians(45.0f);
        const float Yaw   = Math::DegreesToRadians(30.0f);

        const FCameraBasis Basis = BuildCameraBasis(Pitch, Yaw);
        const Vector3 MatrixForward = Matrix4::RotationRollPitchYaw(Pitch, Yaw, 0.0f).TransformNormal(Vector3::Forward);

        TEST_EXPECT(Basis.Forward.IsEqual(MatrixForward, Tolerance));
    }

    TEST_SECTION("Camera basis is orthonormal at representative pitch angles");
    {
        const float PitchAngles[] = { 0.0f, 45.0f, 85.0f, -85.0f, 89.0f, -89.0f };
        const float Yaw           = Math::DegreesToRadians(15.0f);

        for (const float PitchDegrees : PitchAngles)
        {
            const FCameraBasis Basis = BuildCameraBasis(Math::DegreesToRadians(PitchDegrees), Yaw);
            TEST_EXPECT(IsOrthonormalBasis(Basis, Tolerance));
        }
    }

    TEST_SECTION("Camera right vector stays continuous near pitch limits");
    {
        const float Yaw = Math::DegreesToRadians(45.0f);
        Vector3 PreviousRight = BuildCameraBasis(Math::DegreesToRadians(80.0f), Yaw).Right;

        for (float PitchDegrees = 81.0f; PitchDegrees <= 89.0f; PitchDegrees += 1.0f)
        {
            const Vector3 CurrentRight = BuildCameraBasis(Math::DegreesToRadians(PitchDegrees), Yaw).Right;
            TEST_EXPECT(PreviousRight.DotProduct(CurrentRight) > 0.99f);
            PreviousRight = CurrentRight;
        }
    }

    TEST_SECTION("Camera right movement stays horizontal at high pitch");
    {
        const float Pitch = Math::DegreesToRadians(85.0f);
        const float Yaw   = Math::DegreesToRadians(20.0f);
        const FCameraBasis Basis = BuildCameraBasis(Pitch, Yaw);

        TEST_EXPECT(Math::Abs(Basis.Right.Y) <= Tolerance);
    }

    TEST_END();
}
