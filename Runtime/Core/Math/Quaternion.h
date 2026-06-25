#pragma once
#include "Core/Math/Math.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/VectorMath/VectorMath.h"

/**
 * @brief Quaternion representing a 3D rotation. Stored as (X, Y, Z, W), where (X,Y,Z)
 * is the vector part and W is the scalar part. Identity quaternion = (0,0,0,1).
 */
class VECTOR_ALIGN Quaternion
{
public:

    /** @brief Identity quaternion (no rotation). */
    static CORE_API const Quaternion Identity;

public:

    /**
     * @brief Default constructor (identity rotation).
     */
    FORCEINLINE Quaternion() noexcept
        : X(0.0f)
        , Y(0.0f)
        , Z(0.0f)
        , W(1.0f)
    {
    }

    /**
     * @brief Constructor initializing all components.
     * @param InX X component.
     * @param InY Y component.
     * @param InZ Z component.
     * @param InW W component.
     */
    FORCEINLINE explicit Quaternion(float InX, float InY, float InZ, float InW) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
        , W(InW)
    {
    }

    /**
     * @brief Constructor from vector part + scalar part.
     * @param InXYZ Vector part (X,Y,Z).
     * @param InW Scalar part.
     */
    FORCEINLINE explicit Quaternion(const Vector3& InXYZ, float InW) noexcept
        : X(InXYZ.X)
        , Y(InXYZ.Y)
        , Z(InXYZ.Z)
        , W(InW)
    {
    }

public:

    /**
     * @brief Returns the squared length of the quaternion.
     * @return Squared length.
     */
    FORCEINLINE float GetLengthSquared() const noexcept
    {
    #if !USE_VECTOR_MATH
        return (X * X) + (Y * Y) + (Z * Z) + (W * W);
    #else
        const FFloat128 QuaternionVector = FVectorMath::VectorLoad(XYZW);
        const FFloat128 DotProductVector = FVectorMath::VectorDot(QuaternionVector, QuaternionVector);
        return FVectorMath::VectorGetX(DotProductVector);
    #endif
    }

    /**
     * @brief Returns the length of the quaternion.
     * @return Length.
     */
    FORCEINLINE float GetLength() const noexcept
    {
        return Math::Sqrt(GetLengthSquared());
    }

    /**
     * @brief Normalizes this quaternion in-place.
     * @return Reference to this quaternion.
     */
    inline Quaternion& Normalize() noexcept
    {
    #if !USE_VECTOR_MATH
        const float LengthSquared = GetLengthSquared();
        if (LengthSquared != 0.0f)
        {
            const float InverseLength = 1.0f / Math::Sqrt(LengthSquared);
            X *= InverseLength;
            Y *= InverseLength;
            Z *= InverseLength;
            W *= InverseLength;
        }
    #else
        FFloat128       QuaternionVector    = FVectorMath::VectorLoad(XYZW);
        const FFloat128 LengthSquaredVector = FVectorMath::VectorDot(QuaternionVector, QuaternionVector);

        const float LengthSquared = FVectorMath::VectorGetX(LengthSquaredVector);
        if (LengthSquared != 0.0f)
        {
            // invLen = rsqrt(lenSq). Broadcast to all lanes.
            FFloat128 InverseLengthVector = FVectorMath::VectorRecipSqrt(LengthSquaredVector);
            InverseLengthVector = FVectorMath::VectorBroadcast<0>(InverseLengthVector);

            QuaternionVector = FVectorMath::VectorMul(QuaternionVector, InverseLengthVector);
            FVectorMath::VectorStore(QuaternionVector, XYZW);
        }
    #endif

        return *this;
    }

    /**
     * @brief Returns a normalized copy of this quaternion.
     * @return Normalized quaternion.
     */
    FORCEINLINE Quaternion GetNormalized() const noexcept
    {
        Quaternion Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief Checks if this quaternion is a unit quaternion.
     * @return True if unit quaternion.
     */
    FORCEINLINE bool IsUnit() const noexcept
    {
        const float Difference = Math::Abs(1.0f - GetLengthSquared());
        return Difference < Math::Constants::CmpThreshold;
    }

    /**
     * @brief Checks if this quaternion is component-wise equal to another within a threshold.
     * @param Other The quaternion to compare with.
     * @param Threshold Per-component tolerance.
     * @return True if all components are within the threshold.
     */
    FORCEINLINE bool IsEqual(const Quaternion& Other, float Threshold = Math::Constants::CmpThreshold) const noexcept
    {
        Threshold = Math::Abs(Threshold);
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (Math::Abs(XYZW[Index] - Other.XYZW[Index]) > Threshold)
            {
                return false;
            }
        }

        return true;
    }

public:

    /**
     * @brief Computes the dot product with another quaternion.
     * @param Other Other quaternion.
     * @return Dot product.
     */
    FORCEINLINE float DotProduct(const Quaternion& Other) const noexcept
    {
    #if !USE_VECTOR_MATH
        return (X * Other.X) + (Y * Other.Y) + (Z * Other.Z) + (W * Other.W);
    #else
        const FFloat128 ThisQuaternionVector  = FVectorMath::VectorLoad(XYZW);
        const FFloat128 OtherQuaternionVector = FVectorMath::VectorLoad(Other.XYZW);
        const FFloat128 DotProductVector      = FVectorMath::VectorDot(ThisQuaternionVector, OtherQuaternionVector);
        return FVectorMath::VectorGetX(DotProductVector);
    #endif
    }

    /**
     * @brief Returns the conjugate of this quaternion.
     * @return Conjugated quaternion.
     */
    FORCEINLINE Quaternion GetConjugated() const noexcept
    {
        return Quaternion(-X, -Y, -Z, W);
    }

    /**
     * @brief Returns the inverse of this quaternion.
     * @return Inversed quaternion.
     */
    FORCEINLINE Quaternion GetInversed() const noexcept
    {
        const float LengthSquared = GetLengthSquared();
        if (LengthSquared == 0.0f)
        {
            return Identity;
        }

        const float InverseLengthSquared = 1.0f / LengthSquared;
        return GetConjugated() * InverseLengthSquared;
    }

public:

    /**
     * @brief Rotates a vector by this quaternion.
     * @param Vector Vector to rotate.
     * @return Rotated vector.
     */
    FORCEINLINE Vector3 RotateVector(const Vector3& Vector) const noexcept
    {
        // v' = v + 2*w*(q.xyz x v) + 2*(q.xyz x (q.xyz x v))
        const Vector3 QuaternionVectorPart(X, Y, Z);

        const Vector3 T = 2.0f * QuaternionVectorPart.CrossProduct(Vector);
        return Vector + (W * T) + QuaternionVectorPart.CrossProduct(T);
    }

public:

    /**
     * @brief Converts this quaternion to a 3x3 rotation matrix.
     * @return Rotation matrix.
     */
    FORCEINLINE Matrix3 ToMatrix3() const noexcept
    {
        const float XX = X * X;
        const float YY = Y * Y;
        const float ZZ = Z * Z;

        const float XY = X * Y;
        const float XZ = X * Z;
        const float YZ = Y * Z;

        const float WX = W * X;
        const float WY = W * Y;
        const float WZ = W * Z;

        return Matrix3(
            1.0f - 2.0f * (YY + ZZ), 2.0f * (XY + WZ),        2.0f * (XZ - WY),
            2.0f * (XY - WZ),        1.0f - 2.0f * (XX + ZZ), 2.0f * (YZ + WX),
            2.0f * (XZ + WY),        2.0f * (YZ - WX),        1.0f - 2.0f * (XX + YY));
    }

    /**
     * @brief Converts this quaternion to a 4x4 rotation matrix.
     * @return Rotation matrix (no translation).
     */
    FORCEINLINE Matrix4 ToMatrix4() const noexcept
    {
        const Matrix3 RotationMatrix3 = ToMatrix3();

        return Matrix4(
            RotationMatrix3.M[0][0], RotationMatrix3.M[0][1], RotationMatrix3.M[0][2], 0.0f,
            RotationMatrix3.M[1][0], RotationMatrix3.M[1][1], RotationMatrix3.M[1][2], 0.0f,
            RotationMatrix3.M[2][0], RotationMatrix3.M[2][1], RotationMatrix3.M[2][2], 0.0f,
            0.0f,                    0.0f,                    0.0f,                    1.0f);
    }

public:

    /**
     * @brief Converts this quaternion into Euler angles (Pitch, Yaw, Roll) in radians.
     * This extraction matches Matrix3::RotationRollPitchYaw(Pitch, Yaw, Roll).
     * @return Vector3(Pitch, Yaw, Roll) in radians.
     */
    FORCEINLINE Vector3 ToEuler() const noexcept
    {
        // Use matrix extraction that matches:
        // Matrix3::RotationRollPitchYaw(Pitch, Yaw, Roll)
        //
        // Matrix layout from your RotationRollPitchYaw:
        // M[2][1] = -SinP
        // M[2][0] = CosP * SinY
        // M[2][2] = CosP * CosY
        // M[0][1] = SinR * CosP
        // M[1][1] = CosR * CosP

        const Matrix3 RotationMatrix = ToMatrix3();

        const float SinPitch        = -RotationMatrix.M[2][1];
        const float ClampedSinPitch = Math::Clamp(SinPitch, -1.0f, 1.0f);
        const float Pitch           = Math::Asin(ClampedSinPitch);
        const float CosPitch        = Math::Cos(Pitch);

        float Yaw  = 0.0f;
        float Roll = 0.0f;

        // If CosPitch is small, we're near gimbal lock
        if (Math::Abs(CosPitch) > 1e-6f)
        {
            // Yaw from:
            // M[2][0] = CosP * SinY
            // M[2][2] = CosP * CosY
            Yaw = Math::Atan2(RotationMatrix.M[2][0], RotationMatrix.M[2][2]);

            // Roll from:
            // M[0][1] = SinR * CosP
            // M[1][1] = CosR * CosP
            Roll = Math::Atan2(RotationMatrix.M[0][1], RotationMatrix.M[1][1]);
        }
        else
        {
            // Gimbal lock fallback:
            // Pitch is close to +/- 90 degrees -> Yaw and Roll become coupled.
            // We set Roll = 0 and recover Yaw from the remaining terms.
            Roll = 0.0f;

            // Yaw from:
            // M[0][0] = (CosR * CosY) + (SinR * SinP * SinY)
            // M[0][2] = (SinR * SinP * CosY) - (CosR * SinY)
            // For Pitch = +/- 90, this reduces and we can extract yaw as:
            Yaw = Math::Atan2(-RotationMatrix.M[0][2], RotationMatrix.M[0][0]);
        }

        return Vector3(Pitch, Yaw, Roll);
    }

public:

    /**
     * @brief Creates a quaternion from an axis and angle (radians).
     * @param Axis Rotation axis (does not need to be normalized).
     * @param AngleRadians Rotation angle in radians.
     * @return Quaternion.
     */
    static FORCEINLINE Quaternion FromAxisAngle(const Vector3& Axis, float AngleRadians) noexcept
    {
        const Vector3 NormalizedAxis = Axis.GetNormalized();

        const float HalfAngle    = AngleRadians * 0.5f;
        const float SinHalfAngle = Math::Sin(HalfAngle);
        const float CosHalfAngle = Math::Cos(HalfAngle);

        return Quaternion(NormalizedAxis.X * SinHalfAngle, NormalizedAxis.Y * SinHalfAngle, NormalizedAxis.Z * SinHalfAngle, CosHalfAngle);
    }

    /**
     * @brief Creates a quaternion from Euler angles (Pitch, Yaw, Roll) in radians.
     * This matches Matrix3::RotationRollPitchYaw(Pitch, Yaw, Roll).
     * 
     * @param EulerRadians Vector3(Pitch, Yaw, Roll) in radians.
     * @return Quaternion.
     */
    static FORCEINLINE Quaternion FromEuler(const Vector3& EulerRadians) noexcept
    {
        return FromEuler(EulerRadians.X, EulerRadians.Y, EulerRadians.Z);
    }

    /**
     * @brief Creates a quaternion from Euler angles (Pitch, Yaw, Roll) in radians.
     * This matches Matrix3::RotationRollPitchYaw(Pitch, Yaw, Roll).
     *
     * @param Pitch Rotation around X (radians).
     * @param Yaw Rotation around Y (radians).
     * @param Roll Rotation around Z (radians).
     * @return Quaternion.
     */
    static FORCEINLINE Quaternion FromEuler(float Pitch, float Yaw, float Roll) noexcept
    {
        const Matrix3 RotationMatrix = Matrix3::RotationRollPitchYaw(Pitch, Yaw, Roll);
        return FromRotationMatrix(RotationMatrix);
    }

    /**
     * @brief Creates a quaternion from a rotation matrix (assumed orthonormal).
     * @param Rotation Rotation matrix.
     * @return Quaternion.
     */
    static FORCEINLINE Quaternion FromRotationMatrix(const Matrix3& Rotation) noexcept
    {
        const float Trace = Rotation.M[0][0] + Rotation.M[1][1] + Rotation.M[2][2];

        if (Trace > 0.0f)
        {
            const float S    = Math::Sqrt(Trace + 1.0f) * 2.0f;
            const float InvS = 1.0f / S;
            const float OutW = 0.25f * S;
            const float OutX = (Rotation.M[1][2] - Rotation.M[2][1]) * InvS;
            const float OutY = (Rotation.M[2][0] - Rotation.M[0][2]) * InvS;
            const float OutZ = (Rotation.M[0][1] - Rotation.M[1][0]) * InvS;

            return Quaternion(OutX, OutY, OutZ, OutW);
        }

        if (Rotation.M[0][0] > Rotation.M[1][1] && Rotation.M[0][0] > Rotation.M[2][2])
        {
            const float S    = Math::Sqrt(1.0f + Rotation.M[0][0] - Rotation.M[1][1] - Rotation.M[2][2]) * 2.0f;
            const float InvS = 1.0f / S;
            const float OutW = (Rotation.M[1][2] - Rotation.M[2][1]) * InvS;
            const float OutX = 0.25f * S;
            const float OutY = (Rotation.M[1][0] + Rotation.M[0][1]) * InvS;
            const float OutZ = (Rotation.M[2][0] + Rotation.M[0][2]) * InvS;

            return Quaternion(OutX, OutY, OutZ, OutW);
        }

        if (Rotation.M[1][1] > Rotation.M[2][2])
        {
            const float S    = Math::Sqrt(1.0f + Rotation.M[1][1] - Rotation.M[0][0] - Rotation.M[2][2]) * 2.0f;
            const float InvS = 1.0f / S;
            const float OutW = (Rotation.M[2][0] - Rotation.M[0][2]) * InvS;
            const float OutX = (Rotation.M[1][0] + Rotation.M[0][1]) * InvS;
            const float OutY = 0.25f * S;
            const float OutZ = (Rotation.M[2][1] + Rotation.M[1][2]) * InvS;

            return Quaternion(OutX, OutY, OutZ, OutW);
        }

        {
            const float S    = Math::Sqrt(1.0f + Rotation.M[2][2] - Rotation.M[0][0] - Rotation.M[1][1]) * 2.0f;
            const float InvS = 1.0f / S;
            const float OutW = (Rotation.M[0][1] - Rotation.M[1][0]) * InvS;
            const float OutX = (Rotation.M[2][0] + Rotation.M[0][2]) * InvS;
            const float OutY = (Rotation.M[2][1] + Rotation.M[1][2]) * InvS;
            const float OutZ = 0.25f * S;

            return Quaternion(OutX, OutY, OutZ, OutW);
        }
    }

    /**
     * @brief Normalized linear interpolation (shortest path).
     * @param A Start.
     * @param B End.
     * @param Factor [0,1].
     * @return Interpolated quaternion.
     */
    static FORCEINLINE Quaternion Nlerp(const Quaternion& A, const Quaternion& B, float Factor) noexcept
    {
        // Ensure shortest path: if dot < 0, negate B.
        Quaternion EndQuaternion = B;

    #if USE_VECTOR_MATH
        const FFloat128 StartQuaternionVector = FVectorMath::VectorLoad(A.XYZW);
        const FFloat128 EndQuaternionVector   = FVectorMath::VectorLoad(B.XYZW);
        const FFloat128 DotProductVector      = FVectorMath::VectorDot(StartQuaternionVector, EndQuaternionVector);
        const float     DotProduct            = FVectorMath::VectorGetX(DotProductVector);

        if (DotProduct < 0.0f)
        {
            EndQuaternion = -B;
        }
    #else
        const float DotProduct = A.DotProduct(B);
        if (DotProduct < 0.0f)
        {
            EndQuaternion = -B;
        }
    #endif

        Quaternion Result(
            Math::Lerp(A.X, EndQuaternion.X, Factor),
            Math::Lerp(A.Y, EndQuaternion.Y, Factor),
            Math::Lerp(A.Z, EndQuaternion.Z, Factor),
            Math::Lerp(A.W, EndQuaternion.W, Factor));

        Result.Normalize();
        return Result;
    }

    /**
     * @brief Spherical linear interpolation (shortest path).
     * @param A Start.
     * @param B End.
     * @param Factor [0,1].
     * @return Interpolated quaternion.
     */
    static FORCEINLINE Quaternion Slerp(const Quaternion& A, const Quaternion& B, float Factor) noexcept
    {
        float DotProductValue = A.DotProduct(B);
        Quaternion EndQuaternion = B;

        if (DotProductValue < 0.0f)
        {
            DotProductValue = -DotProductValue;
            EndQuaternion   = -B;
        }

        if (DotProductValue > 0.9995f)
        {
            return Nlerp(A, EndQuaternion, Factor);
        }

        DotProductValue = Math::Clamp(DotProductValue, -1.0f, 1.0f);

        const float Theta0    = Math::Acos(DotProductValue);
        const float Theta     = Theta0 * Factor;
        const float SinTheta0 = Math::Sin(Theta0);
        const float SinTheta  = Math::Sin(Theta);
        const float ScaleA    = Math::Cos(Theta) - DotProductValue * (SinTheta / SinTheta0);
        const float ScaleB    = SinTheta / SinTheta0;

        return Quaternion(
            (A.X * ScaleA) + (EndQuaternion.X * ScaleB),
            (A.Y * ScaleA) + (EndQuaternion.Y * ScaleB),
            (A.Z * ScaleA) + (EndQuaternion.Z * ScaleB),
            (A.W * ScaleA) + (EndQuaternion.W * ScaleB));
    }

public:

    /**
     * @brief Unary negation (flip sign of all components).
     * @return Negated quaternion.
     */
    FORCEINLINE Quaternion operator-() const noexcept
    {
    #if !USE_VECTOR_MATH
        return Quaternion(-X, -Y, -Z, -W);
    #else
        // Flip sign bits using XOR with -0.0f.
        const FFloat128 SignFlipMaskVector      = FVectorMath::VectorSet(-0.0f, -0.0f, -0.0f, -0.0f);
        const FFloat128 QuaternionVector        = FVectorMath::VectorLoad(XYZW);
        const FFloat128 NegatedQuaternionVector = FVectorMath::VectorXor(QuaternionVector, SignFlipMaskVector);

        Quaternion Result;
        FVectorMath::VectorStore(NegatedQuaternionVector, Result.XYZW);
        return Result;
    #endif
    }

    /**
     * @brief Quaternion multiplication (rotation composition).
     * @param RHS Right-hand quaternion.
     * @return Result quaternion.
     */
    FORCEINLINE Quaternion operator*(const Quaternion& RHS) const noexcept
    {
    #if !USE_VECTOR_MATH
        return Quaternion(
            (W * RHS.X) + (RHS.W * X) + (Y * RHS.Z) - (Z * RHS.Y),
            (W * RHS.Y) + (RHS.W * Y) + (Z * RHS.X) - (X * RHS.Z),
            (W * RHS.Z) + (RHS.W * Z) + (X * RHS.Y) - (Y * RHS.X),
            (W * RHS.W) - (X * RHS.X) - (Y * RHS.Y) - (Z * RHS.Z));
    #else
        // SIMD version:
        // xyz = aw*b.xyz + bw*a.xyz + cross(a.xyz, b.xyz)
        // w   = aw*bw - dot(a.xyz, b.xyz)

        const FFloat128 ThisQuaternionVector  = FVectorMath::VectorLoad(XYZW);
        const FFloat128 OtherQuaternionVector = FVectorMath::VectorLoad(RHS.XYZW);
        const FFloat128 ThisWBroadcastVector  = FVectorMath::VectorBroadcast<3>(ThisQuaternionVector);
        const FFloat128 OtherWBroadcastVector = FVectorMath::VectorBroadcast<3>(OtherQuaternionVector);
        const FFloat128 ThisWTimesOtherVector = FVectorMath::VectorMul(ThisWBroadcastVector, OtherQuaternionVector);
        const FFloat128 OtherWTimesThisVector = FVectorMath::VectorMul(OtherWBroadcastVector, ThisQuaternionVector);

        FFloat128 ResultXYZWVector = FVectorMath::VectorAdd(ThisWTimesOtherVector, OtherWTimesThisVector);

        const FFloat128 CrossProductVector = FVectorMath::VectorCross(ThisQuaternionVector, OtherQuaternionVector);
        ResultXYZWVector = FVectorMath::VectorAdd(ResultXYZWVector, CrossProductVector);

        const FFloat128 MaskXYZVector          = FVectorMath::VectorSet(1.0f, 1.0f, 1.0f, 0.0f);
        const FFloat128 ThisXYZOnlyVector      = FVectorMath::VectorMul(ThisQuaternionVector, MaskXYZVector);
        const FFloat128 OtherXYZOnlyVector     = FVectorMath::VectorMul(OtherQuaternionVector, MaskXYZVector);
        const FFloat128 DotXYZVector           = FVectorMath::VectorDot(ThisXYZOnlyVector, OtherXYZOnlyVector);
        const FFloat128 ThisWTimesOtherWVector = FVectorMath::VectorMul(ThisWBroadcastVector, OtherWBroadcastVector);
        const FFloat128 ResultWVector          = FVectorMath::VectorSub(ThisWTimesOtherWVector, DotXYZVector);
        const FFloat128 MaskWVector            = FVectorMath::VectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        const FFloat128 ResultXYZOnlyVector    = FVectorMath::VectorMul(ResultXYZWVector, MaskXYZVector);
        const FFloat128 ResultWOnlyVector      = FVectorMath::VectorMul(ResultWVector, MaskWVector);
        const FFloat128 FinalQuaternionVector  = FVectorMath::VectorAdd(ResultXYZOnlyVector, ResultWOnlyVector);

        Quaternion Result;
        FVectorMath::VectorStore(FinalQuaternionVector, Result.XYZW);
        return Result;
    #endif
    }

    /**
     * @brief Quaternion multiply-assign.
     * @param RHS Right-hand quaternion.
     * @return Reference to this.
     */
    FORCEINLINE Quaternion& operator*=(const Quaternion& RHS) noexcept
    {
        *this = (*this) * RHS;
        return *this;
    }

    /**
     * @brief Scalar multiplication.
     * @param RHS Scalar.
     * @return Scaled quaternion.
     */
    FORCEINLINE Quaternion operator*(float RHS) const noexcept
    {
    #if !USE_VECTOR_MATH
        return Quaternion(X * RHS, Y * RHS, Z * RHS, W * RHS);
    #else
        const FFloat128 QuaternionVector       = FVectorMath::VectorLoad(XYZW);
        const FFloat128 ScalarVector           = FVectorMath::VectorSet1(RHS);
        const FFloat128 ScaledQuaternionVector = FVectorMath::VectorMul(QuaternionVector, ScalarVector);

        Quaternion Result;
        FVectorMath::VectorStore(ScaledQuaternionVector, Result.XYZW);
        return Result;
    #endif
    }

    /**
     * @brief Scalar multiply-assign.
     * @param RHS Scalar.
     * @return Reference to this.
     */
    FORCEINLINE Quaternion& operator*=(float RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        X *= RHS;
        Y *= RHS;
        Z *= RHS;
        W *= RHS;
    #else
        const FFloat128 QuaternionVector       = FVectorMath::VectorLoad(XYZW);
        const FFloat128 ScalarVector           = FVectorMath::VectorSet1(RHS);
        const FFloat128 ScaledQuaternionVector = FVectorMath::VectorMul(QuaternionVector, ScalarVector);

        FVectorMath::VectorStore(ScaledQuaternionVector, XYZW);
    #endif
        return *this;
    }

public:

    /**
     * @brief Component access.
     * @param Index 0=X, 1=Y, 2=Z, 3=W.
     * @return Reference.
     */
    FORCEINLINE float& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 4);
        return XYZW[Index];
    }

    /**
     * @brief Component access.
     * @param Index 0=X, 1=Y, 2=Z, 3=W.
     * @return Value.
     */
    FORCEINLINE float operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 4);
        return XYZW[Index];
    }

public:

    union
    {
        struct
        {
            /** @brief X component. */
            float X;

            /** @brief Y component. */
            float Y;

            /** @brief Z component. */
            float Z;

            /** @brief W component. */
            float W;
        };

        /** @brief Components as an array: [X,Y,Z,W]. */
        float XYZW[4];
    };
};

MARK_AS_REALLOCATABLE(Quaternion);
