#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3.h"
#include "Core/Math/Vector4.h"

class VECTOR_ALIGN FMatrix4
{
public:

    /**
     * @brief - Default constructor (Initialize components to zero)
     */
    FORCEINLINE FMatrix4() noexcept
        : m00(0.0f), m01(0.0f), m02(0.0f), m03(0.0f)
        , m10(0.0f), m11(0.0f), m12(0.0f), m13(0.0f)
        , m20(0.0f), m21(0.0f), m22(0.0f), m23(0.0f)
        , m30(0.0f), m31(0.0f), m32(0.0f), m33(0.0f)
    {
    }

    /**
     * @brief          - Constructor initializing all values on the diagonal with a single value. The other values are set to zero.
     * @param Diagonal - Value to set on the diagonal
     */
    FORCEINLINE explicit FMatrix4(float Diagonal) noexcept
        : m00(Diagonal), m01(0.0f), m02(0.0f), m03(0.0f)
        , m10(0.0f), m11(Diagonal), m12(0.0f), m13(0.0f)
        , m20(0.0f), m21(0.0f), m22(Diagonal), m23(0.0f)
        , m30(0.0f), m31(0.0f), m32(0.0f), m33(Diagonal)
    {
    }

    /**
     * @brief      - Constructor initializing all values with vectors specifying each row
     * @param Row0 - Vector to set the first row to
     * @param Row1 - Vector to set the second row to
     * @param Row2 - Vector to set the third row to
     * @param Row3 - Vector to set the fourth row to
     */
    FORCEINLINE explicit FMatrix4(const FVector4& Row0, const FVector4& Row1, const FVector4& Row2, const FVector4& Row3) noexcept
        : m00(Row0.x), m01(Row0.y), m02(Row0.z), m03(Row0.w)
        , m10(Row1.x), m11(Row1.y), m12(Row1.z), m13(Row1.w)
        , m20(Row2.x), m21(Row2.y), m22(Row2.z), m23(Row2.w)
        , m30(Row3.x), m31(Row3.y), m32(Row3.z), m33(Row3.w)
    {
    }

    /**
     * @brief      - Constructor initializing all values with corresponding value
     * @param In00 - Value to set on row 0 and column 0
     * @param In01 - Value to set on row 0 and column 1
     * @param In02 - Value to set on row 0 and column 2
     * @param In03 - Value to set on row 0 and column 3
     * @param In10 - Value to set on row 1 and column 0
     * @param In11 - Value to set on row 1 and column 1
     * @param In12 - Value to set on row 1 and column 2
     * @param In13 - Value to set on row 1 and column 3
     * @param In20 - Value to set on row 2 and column 0
     * @param In21 - Value to set on row 2 and column 1
     * @param In22 - Value to set on row 2 and column 2
     * @param In23 - Value to set on row 2 and column 3
     * @param In30 - Value to set on row 3 and column 0
     * @param In31 - Value to set on row 3 and column 1
     * @param In32 - Value to set on row 3 and column 2
     * @param In33 - Value to set on row 3 and column 3
     */
    FORCEINLINE explicit FMatrix4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33) noexcept
        : m00(m00), m01(m01), m02(m02), m03(m03)
        , m10(m10), m11(m11), m12(m12), m13(m13)
        , m20(m20), m21(m21), m22(m22), m23(m23)
        , m30(m30), m31(m31), m32(m32), m33(m33)
    {
    }

    /**
     * @brief     - Constructor initializing all components with an array
     * @param Arr - Array with at least 16 elements
     */
    FORCEINLINE explicit FMatrix4(const float* Arr) noexcept
        : m00(Arr[0]) , m01(Arr[1]) , m02(Arr[2]) , m03(Arr[3])
        , m10(Arr[4]) , m11(Arr[5]) , m12(Arr[6]) , m13(Arr[7])
        , m20(Arr[8]) , m21(Arr[9]) , m22(Arr[10]), m23(Arr[11])
        , m30(Arr[12]), m31(Arr[13]), m32(Arr[14]), m33(Arr[15])
    {
    }

    FORCEINLINE FVector4 Transform(const FVector4& Vector) const noexcept
    {
        FVector4 Result;
    
    #if !USE_VECTOR_MATH
        Result.x = (Vector[0] * m00) + (Vector[1] * m10) + (Vector[2] * m20) + (Vector[3] * m30);
        Result.y = (Vector[0] * m01) + (Vector[1] * m11) + (Vector[2] * m21) + (Vector[3] * m31);
        Result.z = (Vector[0] * m02) + (Vector[1] * m12) + (Vector[2] * m22) + (Vector[3] * m32);
        Result.w = (Vector[0] * m03) + (Vector[1] * m13) + (Vector[2] * m23) + (Vector[3] * m33);
    #else
        FFloat128 NewVector = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Vector));
        NewVector = FVectorMath::VectorTransform(reinterpret_cast<const float*>(this), NewVector);
        FVectorMath::StoreAligned(NewVector, reinterpret_cast<float*>(&Result));
    #endif
    
        return Result;
    }

    FORCEINLINE FVector3 Transform(const FVector3& Vector) const noexcept
    {
        FVector3 Result;
    
    #if !USE_VECTOR_MATH
        Result.x = (Vector[0] * m00) + (Vector[1] * m10) + (Vector[2] * m20) + (1.0f * m30);
        Result.y = (Vector[0] * m01) + (Vector[1] * m11) + (Vector[2] * m21) + (1.0f * m31);
        Result.z = (Vector[0] * m02) + (Vector[1] * m12) + (Vector[2] * m22) + (1.0f * m32);
    #else
        FFloat128 NewVector = FVectorMath::Load(Vector.x, Vector.y, Vector.z, 1.0f);
        NewVector = FVectorMath::VectorTransform(reinterpret_cast<const float*>(this), NewVector);
    #endif
    
        return FVector3(FVectorMath::VectorGetX(NewVector), FVectorMath::VectorGetY(NewVector), FVectorMath::VectorGetZ(NewVector));
    }

    /**
     * @brief          - Transform a 3-D vector as position, fourth component to one
     * @param Position - Vector to transform
     * @return         - Transformed vector
     */
    FORCEINLINE FVector3 TransformCoord(const FVector3& Position) const noexcept
    {
        FVector3 Result;

    #if !USE_VECTOR_MATH
        float ComponentW = (Position[0] * m03) + (Position[1] * m13) + (Position[2] * m23) + (1.0f * m33);
        ComponentW = 1.0f / ComponentW;

        Result.x = ((Position[0] * m00) + (Position[1] * m10) + (Position[2] * m20) + (1.0f * m30)) * ComponentW;
        Result.y = ((Position[0] * m01) + (Position[1] * m11) + (Position[2] * m21) + (1.0f * m31)) * ComponentW;
        Result.z = ((Position[0] * m02) + (Position[1] * m12) + (Position[2] * m22) + (1.0f * m32)) * ComponentW;
    #else
        FFloat128 NewPosition = FVectorMath::Load(Position.x, Position.y, Position.z, 1.0f);
        NewPosition = FVectorMath::VectorTransform(reinterpret_cast<const float*>(this), NewPosition);
        
        FFloat128 Temp0 = FVectorMath::LoadOnes();
        FFloat128 Temp1 = FVectorMath::VectorBroadcast<3>(NewPosition);
        Temp0       = FVectorMath::VectorDiv(Temp0, Temp1);
        NewPosition = FVectorMath::VectorMul(Temp0, NewPosition);
    #endif
        
        return FVector3(FVectorMath::VectorGetX(NewPosition), FVectorMath::VectorGetY(NewPosition), FVectorMath::VectorGetZ(NewPosition));
    }

    /**
     * @brief           - Transform a 3-D vector as direction, fourth component to zero
     * @param Direction - Vector to transform
     * @return          - Transformed vector
     */
    FORCEINLINE FVector3 TransformNormal(const FVector3& Direction) const noexcept
    {
        FVector3 Result;
    
    #if !USE_VECTOR_MATH
        Result.x = (Direction[0] * m00) + (Direction[1] * m10) + (Direction[2] * m20);
        Result.y = (Direction[0] * m01) + (Direction[1] * m11) + (Direction[2] * m21);
        Result.z = (Direction[0] * m02) + (Direction[1] * m12) + (Direction[2] * m22);
    #else
        FFloat128 NewDirection = FVectorMath::Load(Direction.x, Direction.y, Direction.z, 0.0f);
        NewDirection = FVectorMath::VectorTransform(reinterpret_cast<const float*>(this), NewDirection);
    #endif
    
        return FVector3(FVectorMath::VectorGetX(NewDirection), FVectorMath::VectorGetY(NewDirection), FVectorMath::VectorGetZ(NewDirection));
    }

    /**
     * @brief  - Returns the transposed version of this matrix
     * @return - Transposed matrix
     */
    inline FMatrix4 Transpose() const noexcept
    {
        FMatrix4 Result;
    
    #if !USE_VECTOR_MATH
        Result.f[0][0] = f[0][0];
        Result.f[0][1] = f[1][0];
        Result.f[0][2] = f[2][0];
        Result.f[0][3] = f[3][0];

        Result.f[1][0] = f[0][1];
        Result.f[1][1] = f[1][1];
        Result.f[1][2] = f[2][1];
        Result.f[1][3] = f[3][1];

        Result.f[2][0] = f[0][2];
        Result.f[2][1] = f[1][2];
        Result.f[2][2] = f[2][2];
        Result.f[2][3] = f[3][2];

        Result.f[3][0] = f[0][3];
        Result.f[3][1] = f[1][3];
        Result.f[3][2] = f[2][3];
        Result.f[3][3] = f[3][3];
    #else
        FVectorMath::Transpose(this, &Result);
    #endif
    
        return Result;
    }

    /**
     * @brief  - Returns the inverted version of this matrix
     * @return - Inverse matrix
     */
    inline FMatrix4 Invert() const noexcept
    {
        FMatrix4 Inverse;
        
    #if !USE_VECTOR_MATH
        float _a = (m22 * m33) - (m23 * m32);
        float _b = (m21 * m33) - (m23 * m31);
        float _c = (m21 * m32) - (m22 * m31);
        float _d = (m20 * m33) - (m23 * m30);
        float _e = (m20 * m32) - (m22 * m30);
        float _f = (m20 * m31) - (m21 * m30);

        //d11
        Inverse.m00 = (m11 * _a) - (m12 * _b) + (m13 * _c);
        //d12
        Inverse.m10 = -((m10 * _a) - (m12 * _d) + (m13 * _e));
        //d13
        Inverse.m20 = (m10 * _b) - (m11 * _d) + (m13 * _f);
        //d14
        Inverse.m30 = -((m10 * _c) - (m11 * _e) + (m12 * _f));

        //d21
        Inverse.m01 = -((m01 * _a) - (m02 * _b) + (m03 * _c));
        //d22
        Inverse.m11 = (m00 * _a) - (m02 * _d) + (m03 * _e);
        //d23
        Inverse.m21 = -((m00 * _b) - (m01 * _d) + (m03 * _f));
        //d24
        Inverse.m31 = (m00 * _c) - (m01 * _e) + (m02 * _f);

        const float Determinant     = (Inverse.m00 * m00) - (Inverse.m10 * m01) + (Inverse.m20 * m02) - (Inverse.m30 * m03);
        const float ReprDeterminant = 1.0f / Determinant;

        Inverse.m00 *= ReprDeterminant;
        Inverse.m10 *= ReprDeterminant;
        Inverse.m20 *= ReprDeterminant;
        Inverse.m30 *= ReprDeterminant;
        Inverse.m01 *= ReprDeterminant;
        Inverse.m11 *= ReprDeterminant;
        Inverse.m21 *= ReprDeterminant;
        Inverse.m31 *= ReprDeterminant;

        _a = (m12 * m33) - (m13 * m32);
        _b = (m11 * m33) - (m13 * m31);
        _c = (m11 * m32) - (m12 * m31);
        _d = (m10 * m33) - (m13 * m30);
        _e = (m10 * m32) - (m12 * m30);
        _f = (m10 * m31) - (m11 * m30);

        //d31
        Inverse.m02 = ((m01 * _a) - (m02 * _b) + (m03 * _c)) * ReprDeterminant;
        //d32
        Inverse.m12 = -((m00 * _a) - (m02 * _d) + (m03 * _e)) * ReprDeterminant;
        //d33
        Inverse.m22 = ((m00 * _b) - (m01 * _d) + (m03 * _f)) * ReprDeterminant;
        //d34
        Inverse.m32 = -((m00 * _c) - (m01 * _e) + (m02 * _f)) * ReprDeterminant;

        _a = (m12 * m23) - (m13 * m22);
        _b = (m11 * m23) - (m13 * m21);
        _c = (m11 * m22) - (m12 * m21);
        _d = (m10 * m23) - (m13 * m20);
        _e = (m10 * m22) - (m12 * m20);
        _f = (m10 * m21) - (m11 * m20);

        //d41
        Inverse.m03 = -((m01 * _a) - (m02 * _b) + (m03 * _c)) * ReprDeterminant;
        //d42
        Inverse.m13 = ((m00 * _a) - (m02 * _d) + (m03 * _e)) * ReprDeterminant;
        //d43
        Inverse.m23 = -((m00 * _b) - (m01 * _d) + (m03 * _f)) * ReprDeterminant;
        //d44
        Inverse.m33 = ((m00 * _c) - (m01 * _e) + (m02 * _f)) * ReprDeterminant;
    #else
        FFloat128 Temp0 = FVectorMath::LoadAligned(f[0]);
        FFloat128 Temp1 = FVectorMath::LoadAligned(f[1]);
        FFloat128 Temp2 = FVectorMath::LoadAligned(f[2]);
        FFloat128 Temp3 = FVectorMath::LoadAligned(f[3]);

        FFloat128 _0 = FVectorMath::VectorShuffle0011<0, 1, 0, 1>(Temp0, Temp1);
        FFloat128 _1 = FVectorMath::VectorShuffle0011<2, 3, 2, 3>(Temp0, Temp1);
        FFloat128 _2 = FVectorMath::VectorShuffle0011<0, 1, 0, 1>(Temp2, Temp3);
        FFloat128 _3 = FVectorMath::VectorShuffle0011<2, 3, 2, 3>(Temp2, Temp3);
        FFloat128 _4 = FVectorMath::VectorShuffle0011<0, 2, 0, 2>(Temp0, Temp2);
        FFloat128 _5 = FVectorMath::VectorShuffle0011<1, 3, 1, 3>(Temp1, Temp3);
        FFloat128 _6 = FVectorMath::VectorShuffle0011<1, 3, 1, 3>(Temp0, Temp2);
        FFloat128 _7 = FVectorMath::VectorShuffle0011<0, 2, 0, 2>(Temp1, Temp3);

        FFloat128 Mul0   = FVectorMath::VectorMul(_4, _5);
        FFloat128 Mul1   = FVectorMath::VectorMul(_6, _7);
        FFloat128 DetSub = FVectorMath::VectorSub(Mul0, Mul1);

        FFloat128 DetA = FVectorMath::VectorBroadcast<0>(DetSub);
        FFloat128 DetB = FVectorMath::VectorBroadcast<1>(DetSub);
        FFloat128 DetC = FVectorMath::VectorBroadcast<2>(DetSub);
        FFloat128 DetD = FVectorMath::VectorBroadcast<3>(DetSub);

        FFloat128 dc = FVectorMath::Mat2AdjointMul(_3, _2);
        FFloat128 ab = FVectorMath::Mat2AdjointMul(_0, _1);

        FFloat128 x = FVectorMath::VectorSub(FVectorMath::VectorMul(DetD, _0), FVectorMath::Mat2Mul(_1, dc));
        FFloat128 w = FVectorMath::VectorSub(FVectorMath::VectorMul(DetA, _3), FVectorMath::Mat2Mul(_2, ab));

        FFloat128 DetM = FVectorMath::VectorMul(DetA, DetD);

        FFloat128 y = FVectorMath::VectorSub(FVectorMath::VectorMul(DetB, _2), FVectorMath::Mat2MulAdjoint(_3, ab));
        FFloat128 z = FVectorMath::VectorSub(FVectorMath::VectorMul(DetC, _1), FVectorMath::Mat2MulAdjoint(_0, dc));

        DetM = FVectorMath::VectorAdd(DetM, FVectorMath::VectorMul(DetB, DetC));

        FFloat128 Trace = FVectorMath::VectorMul(ab, FVectorMath::VectorShuffle<0, 2, 1, 3>(dc));
        Trace = FVectorMath::HorizontalAdd(Trace);
        Trace = FVectorMath::HorizontalAdd(Trace);

        DetM = FVectorMath::VectorSub(DetM, Trace);

        const FFloat128 AdjSignMask = FVectorMath::Load(1.0f, -1.0f, -1.0f, 1.0f);
        DetM = FVectorMath::VectorDiv(AdjSignMask, DetM);

        x = FVectorMath::VectorMul(x, DetM);
        y = FVectorMath::VectorMul(y, DetM);
        z = FVectorMath::VectorMul(z, DetM);
        w = FVectorMath::VectorMul(w, DetM);

        Temp0 = FVectorMath::VectorShuffle0011<3, 1, 3, 1>(x, y);
        Temp1 = FVectorMath::VectorShuffle0011<2, 0, 2, 0>(x, y);
        Temp2 = FVectorMath::VectorShuffle0011<3, 1, 3, 1>(z, w);
        Temp3 = FVectorMath::VectorShuffle0011<2, 0, 2, 0>(z, w);

        FVectorMath::StoreAligned(Temp0, Inverse.f[0]);
        FVectorMath::StoreAligned(Temp1, Inverse.f[1]);
        FVectorMath::StoreAligned(Temp2, Inverse.f[2]);
        FVectorMath::StoreAligned(Temp3, Inverse.f[3]);
    #endif
    
        return Inverse;
    }

    /**
     * @brief  - Returns the adjugate of this matrix
     * @return - Adjugate matrix
     */
    inline FMatrix4 Adjoint() const noexcept
    {
        FMatrix4 Adjugate;

    #if !USE_VECTOR_MATH
        float _a = (m22 * m33) - (m23 * m32);
        float _b = (m21 * m33) - (m23 * m31);
        float _c = (m21 * m32) - (m22 * m31);
        float _d = (m20 * m33) - (m23 * m30);
        float _e = (m20 * m32) - (m22 * m30);
        float _f = (m20 * m31) - (m21 * m30);

        //d11
        Adjugate.m00 = (m11 * _a) - (m12 * _b) + (m13 * _c);
        //d12
        Adjugate.m10 = -((m10 * _a) - (m12 * _d) + (m13 * _e));
        //d13
        Adjugate.m20 = (m10 * _b) - (m11 * _d) + (m13 * _f);
        //d14
        Adjugate.m30 = -((m10 * _c) - (m11 * _e) + (m12 * _f));

        //d21
        Adjugate.m01 = -((m01 * _a) - (m02 * _b) + (m03 * _c));
        //d22
        Adjugate.m11 = (m00 * _a) - (m02 * _d) + (m03 * _e);
        //d23
        Adjugate.m21 = -((m00 * _b) - (m01 * _d) + (m03 * _f));
        //d24
        Adjugate.m31 = (m00 * _c) - (m01 * _e) + (m02 * _f);

        _a = (m12 * m33) - (m13 * m32);
        _b = (m11 * m33) - (m13 * m31);
        _c = (m11 * m32) - (m12 * m31);
        _d = (m10 * m33) - (m13 * m30);
        _e = (m10 * m32) - (m12 * m30);
        _f = (m10 * m31) - (m11 * m30);

        //d31
        Adjugate.m02 = (m01 * _a) - (m02 * _b) + (m03 * _c);
        //d32
        Adjugate.m12 = -((m00 * _a) - (m02 * _d) + (m03 * _e));
        //d33
        Adjugate.m22 = (m00 * _b) - (m01 * _d) + (m03 * _f);
        //d34
        Adjugate.m32 = -((m00 * _c) - (m01 * _e) + (m02 * _f));

        _a = (m12 * m23) - (m13 * m22);
        _b = (m11 * m23) - (m13 * m21);
        _c = (m11 * m22) - (m12 * m21);
        _d = (m10 * m23) - (m13 * m20);
        _e = (m10 * m22) - (m12 * m20);
        _f = (m10 * m21) - (m11 * m20);

        //d41
        Adjugate.m03 = -((m01 * _a) - (m02 * _b) + (m03 * _c));
        //d42
        Adjugate.m13 = (m00 * _a) - (m02 * _d) + (m03 * _e);
        //d43
        Adjugate.m23 = -((m00 * _b) - (m01 * _d) + (m03 * _f));
        //d44
        Adjugate.m33 = (m00 * _c) - (m01 * _e) + (m02 * _f);
    #else
        FFloat128 Temp0 = FVectorMath::LoadAligned(f[0]);
        FFloat128 Temp1 = FVectorMath::LoadAligned(f[1]);
        FFloat128 Temp2 = FVectorMath::LoadAligned(f[2]);
        FFloat128 Temp3 = FVectorMath::LoadAligned(f[3]);

        FFloat128 _0 = FVectorMath::VectorShuffle0011<0, 1, 0, 1>(Temp0, Temp1);
        FFloat128 _1 = FVectorMath::VectorShuffle0011<2, 3, 2, 3>(Temp0, Temp1);
        FFloat128 _2 = FVectorMath::VectorShuffle0011<0, 1, 0, 1>(Temp2, Temp3);
        FFloat128 _3 = FVectorMath::VectorShuffle0011<2, 3, 2, 3>(Temp2, Temp3);
        FFloat128 _4 = FVectorMath::VectorShuffle0011<0, 2, 0, 2>(Temp0, Temp2);
        FFloat128 _5 = FVectorMath::VectorShuffle0011<1, 3, 1, 3>(Temp1, Temp3);
        FFloat128 _6 = FVectorMath::VectorShuffle0011<1, 3, 1, 3>(Temp0, Temp2);
        FFloat128 _7 = FVectorMath::VectorShuffle0011<0, 2, 0, 2>(Temp1, Temp3);

        FFloat128 Mul0   = FVectorMath::VectorMul(_4, _5);
        FFloat128 Mul1   = FVectorMath::VectorMul(_6, _7);
        FFloat128 DetSub = FVectorMath::VectorSub(Mul0, Mul1);

        FFloat128 DetA = FVectorMath::VectorBroadcast<0>(DetSub);
        FFloat128 DetB = FVectorMath::VectorBroadcast<1>(DetSub);
        FFloat128 DetC = FVectorMath::VectorBroadcast<2>(DetSub);
        FFloat128 DetD = FVectorMath::VectorBroadcast<3>(DetSub);

        FFloat128 dc = FVectorMath::Mat2AdjointMul(_3, _2);
        FFloat128 ab = FVectorMath::Mat2AdjointMul(_0, _1);

        FFloat128 x = FVectorMath::VectorSub(FVectorMath::VectorMul(DetD, _0), FVectorMath::Mat2Mul(_1, dc));
        FFloat128 w = FVectorMath::VectorSub(FVectorMath::VectorMul(DetA, _3), FVectorMath::Mat2Mul(_2, ab));

        FFloat128 y = FVectorMath::VectorSub(FVectorMath::VectorMul(DetB, _2), FVectorMath::Mat2MulAdjoint(_3, ab));
        FFloat128 z = FVectorMath::VectorSub(FVectorMath::VectorMul(DetC, _1), FVectorMath::Mat2MulAdjoint(_0, dc));

        const FFloat128 Mask = FVectorMath::Load(1.0f, -1.0f, -1.0f, 1.0f);
        x = FVectorMath::VectorMul(x, Mask);
        y = FVectorMath::VectorMul(y, Mask);
        z = FVectorMath::VectorMul(z, Mask);
        w = FVectorMath::VectorMul(w, Mask);

        Temp0 = FVectorMath::VectorShuffle0011<3, 1, 3, 1>(x, y);
        Temp1 = FVectorMath::VectorShuffle0011<2, 0, 2, 0>(x, y);
        Temp2 = FVectorMath::VectorShuffle0011<3, 1, 3, 1>(z, w);
        Temp3 = FVectorMath::VectorShuffle0011<2, 0, 2, 0>(z, w);

        FVectorMath::StoreAligned(Temp0, Adjugate.f[0]);
        FVectorMath::StoreAligned(Temp1, Adjugate.f[1]);
        FVectorMath::StoreAligned(Temp2, Adjugate.f[2]);
        FVectorMath::StoreAligned(Temp3, Adjugate.f[3]);
    #endif
    
        return Adjugate;
    }

    /**
     * @brief  - Returns the determinant of this matrix
     * @return - The determinant
     */
    inline float Determinant() const noexcept
    {
        float Determinant = 0.0f;
        
    #if !USE_VECTOR_MATH
        float _a = (m22 * m33) - (m23 * m32);
        float _b = (m21 * m33) - (m23 * m31);
        float _c = (m21 * m32) - (m22 * m31);
        float _d = (m20 * m33) - (m23 * m30);
        float _e = (m20 * m32) - (m22 * m30);
        float _f = (m20 * m31) - (m21 * m30);

        //d11
        Determinant = m00 * ((m11 * _a) - (m12 * _b) + (m13 * _c));
        //d12
        Determinant -= m01 * ((m10 * _a) - (m12 * _d) + (m13 * _e));
        //d13
        Determinant += m02 * ((m10 * _b) - (m11 * _d) + (m13 * _f));
        //d14
        Determinant -= m03 * ((m10 * _c) - (m11 * _e) + (m12 * _f));
    #else
        FFloat128 Temp0 = FVectorMath::LoadAligned(f[0]);
        FFloat128 Temp1 = FVectorMath::LoadAligned(f[1]);
        FFloat128 Temp2 = FVectorMath::LoadAligned(f[2]);
        FFloat128 Temp3 = FVectorMath::LoadAligned(f[3]);

        FFloat128 _0 = FVectorMath::VectorShuffle0011<0, 1, 0, 1>(Temp0, Temp1);
        FFloat128 _1 = FVectorMath::VectorShuffle0011<2, 3, 2, 3>(Temp0, Temp1);
        FFloat128 _2 = FVectorMath::VectorShuffle0011<0, 1, 0, 1>(Temp2, Temp3);
        FFloat128 _3 = FVectorMath::VectorShuffle0011<2, 3, 2, 3>(Temp2, Temp3);
        FFloat128 _4 = FVectorMath::VectorShuffle0011<0, 2, 0, 2>(Temp0, Temp2);
        FFloat128 _5 = FVectorMath::VectorShuffle0011<1, 3, 1, 3>(Temp1, Temp3);
        FFloat128 _6 = FVectorMath::VectorShuffle0011<1, 3, 1, 3>(Temp0, Temp2);
        FFloat128 _7 = FVectorMath::VectorShuffle0011<0, 2, 0, 2>(Temp1, Temp3);

        FFloat128 Mul0   = FVectorMath::VectorMul(_4, _5);
        FFloat128 Mul1   = FVectorMath::VectorMul(_6, _7);
        FFloat128 DetSub = FVectorMath::VectorSub(Mul0, Mul1);

        FFloat128 DetA = FVectorMath::VectorBroadcast<0>(DetSub);
        FFloat128 DetB = FVectorMath::VectorBroadcast<1>(DetSub);
        FFloat128 DetC = FVectorMath::VectorBroadcast<2>(DetSub);
        FFloat128 DetD = FVectorMath::VectorBroadcast<3>(DetSub);

        FFloat128 dc = FVectorMath::Mat2AdjointMul(_3, _2);
        FFloat128 ab = FVectorMath::Mat2AdjointMul(_0, _1);

        FFloat128 DetM = FVectorMath::VectorMul(DetA, DetD);
        Mul0 = FVectorMath::VectorMul(DetB, DetC);
        DetM = FVectorMath::VectorAdd(DetM, Mul0);
        Mul0 = FVectorMath::VectorMul(ab, FVectorMath::VectorShuffle<0, 2, 1, 3>(dc));

        FFloat128 Sum = FVectorMath::HorizontalSum(Mul0);
        DetM = FVectorMath::VectorSub(DetM, Sum);
        Determinant = FVectorMath::VectorGetX(DetM);
    #endif
        
        return Determinant;
    }

    /**
     * @brief  - Checks weather this matrix has any value that equals NaN
     * @return - True if the any value equals NaN, false if not
     */
    inline bool HasNaN() const noexcept
    {
        for (int32 i = 0; i < 16; i++)
        {
            if (FMath::IsNaN(reinterpret_cast<const float*>(this)[i]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief  - Checks weather this matrix has any value that equals infinity
     * @return - True if the any value equals infinity, false if not
     */
    inline bool HasInfinity() const noexcept
    {
        for (int32 i = 0; i < 16; i++)
        {
            if (FMath::IsInfinity(reinterpret_cast<const float*>(this)[i]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief  - Checks weather this matrix has any value that equals infinity or NaN
     * @return - False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return !HasNaN() && !HasInfinity();
    }

    /**
     * @brief       - Compares, within a threshold Epsilon, this matrix with another matrix
     * @param Other - matrix to compare against
     * @return      - True if equal, false if not
     */
    inline bool IsEqual(const FMatrix4& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
    #if !USE_VECTOR_MATH
        Epsilon = FMath::Abs(Epsilon);

        for (int32 i = 0; i < 16; i++)
        {
            float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
            if (FMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
    #else
        FFloat128 Espilon128 = FVectorMath::Load(Epsilon);
        Espilon128 = FVectorMath::VectorAbs(Espilon128);

        for (int32 i = 0; i < 4; i++)
        {
            FFloat128 Diff = FVectorMath::VectorSub(f[i], Other.f[i]);
            Diff = FVectorMath::VectorAbs(Diff);

            if (FVectorMath::GreaterThan(Diff, Espilon128))
            {
                return false;
            }
        }

        return true;
    #endif
    }

     /** @brief - Sets this matrix to an identity matrix */
    FORCEINLINE void SetIdentity() noexcept
    {
    #if !USE_VECTOR_MATH
        m00 = 1.0f;
        m01 = 0.0f;
        m02 = 0.0f;
        m03 = 0.0f;

        m10 = 0.0f;
        m11 = 1.0f;
        m12 = 0.0f;
        m13 = 0.0f;

        m20 = 0.0f;
        m21 = 0.0f;
        m22 = 1.0f;
        m23 = 0.0f;

        m30 = 0.0f;
        m31 = 0.0f;
        m32 = 0.0f;
        m33 = 1.0f;
    #else
        FVectorMath::StoreAligned(FVectorMath::Load(1.0f, 0.0f, 0.0f, 0.0f), f[0]);
        FVectorMath::StoreAligned(FVectorMath::Load(0.0f, 1.0f, 0.0f, 0.0f), f[1]);
        FVectorMath::StoreAligned(FVectorMath::Load(0.0f, 0.0f, 1.0f, 0.0f), f[2]);
        FVectorMath::StoreAligned(FVectorMath::Load(0.0f, 0.0f, 0.0f, 1.0f), f[3]);
    #endif
    }

    /**
     * @brief                  - Sets the upper 3x3 matrix
     * @param RotationAndScale - 3x3 to set the upper quadrant to
     */
    FORCEINLINE void SetRotationAndScale(const FMatrix3& RotationAndScale) noexcept
    {
        m00 = RotationAndScale.m00;
        m01 = RotationAndScale.m01;
        m02 = RotationAndScale.m02;

        m10 = RotationAndScale.m10;
        m11 = RotationAndScale.m11;
        m12 = RotationAndScale.m12;

        m20 = RotationAndScale.m20;
        m21 = RotationAndScale.m21;
        m22 = RotationAndScale.m22;
    }

    /**
     * @brief             - Sets the translation part of a matrix
     * @param Translation - The translation part
     */
    FORCEINLINE void SetTranslation(const FVector3& Translation) noexcept
    {
        m30 = Translation.x;
        m31 = Translation.y;
        m32 = Translation.z;
    }

    /**
     * @brief     - Returns a row of this matrix
     * @param Row - The row to retrieve
     * @return    - A vector containing the specified row
     */
    FORCEINLINE FVector4 GetRow(int32 Row) const noexcept
    {
        CHECK(Row < 4);
        return FVector4(f[Row]);
    }

    /**
     * @brief        - Returns a column of this matrix
     * @param Column - The column to retrieve
     * @return       - A vector containing the specified column
     */
    FORCEINLINE FVector4 GetColumn(int32 Column) const noexcept
    {
        CHECK(Column < 4);
        return FVector4(f[0][Column], f[1][Column], f[2][Column], f[3][Column]);
    }

    /**
     * @brief  - Returns the translation part of this matrix, that is the x-, y-, and z-coordinates of the fourth row
     * @return - A vector containing the translation
     */
    FORCEINLINE FVector3 GetTranslation() const noexcept
    {
        return FVector3(m30, m31, m32);
    }

    /**
     * @brief  - Returns the 3x3 matrix thats forming the upper quadrant of this matrix.
     * @return - A matrix containing the upper part of the matrix
     */
    FORCEINLINE FMatrix3 GetRotationAndScale() const noexcept
    {
        return FMatrix3(m00, m01, m02, m10, m11, m12, m20, m21, m22);
    }

    /**
     * @brief  - Returns the data of this matrix as a pointer
     * @return - A pointer to the data
     */
    FORCEINLINE float* Data() noexcept
    {
        return reinterpret_cast<float*>(this);
    }

    /**
     * @brief  - Returns the data of this matrix as a pointer
     * @return - A pointer to the data
     */
    FORCEINLINE const float* Data() const noexcept
    {
        return reinterpret_cast<const float*>(this);
    }

public:

    /**
     * @brief     - Transforms a 4-D vector
     * @param RHS - The vector to transform
     * @return    - A vector containing the transformation
     */
    FORCEINLINE FVector4 operator*(const FVector4& RHS) const noexcept
    {
        return Transform(RHS);
    }

    /**
     * @brief     - Multiplies a matrix with another matrix
     * @param RHS - The other matrix
     * @return    - A matrix containing the result of the multiplication
     */
    FORCEINLINE FMatrix4 operator*(const FMatrix4& RHS) const noexcept
    {
        FMatrix4 Result;
    
    #if !USE_VECTOR_MATH
        Result.m00 = (m00 * RHS.m00) + (m01 * RHS.m10) + (m02 * RHS.m20) + (m03 * RHS.m30);
        Result.m01 = (m00 * RHS.m01) + (m01 * RHS.m11) + (m02 * RHS.m21) + (m03 * RHS.m31);
        Result.m02 = (m00 * RHS.m02) + (m01 * RHS.m12) + (m02 * RHS.m22) + (m03 * RHS.m32);
        Result.m03 = (m00 * RHS.m03) + (m01 * RHS.m13) + (m02 * RHS.m23) + (m03 * RHS.m33);

        Result.m10 = (m10 * RHS.m00) + (m11 * RHS.m10) + (m12 * RHS.m20) + (m13 * RHS.m30);
        Result.m11 = (m10 * RHS.m01) + (m11 * RHS.m11) + (m12 * RHS.m21) + (m13 * RHS.m31);
        Result.m12 = (m10 * RHS.m02) + (m11 * RHS.m12) + (m12 * RHS.m22) + (m13 * RHS.m32);
        Result.m13 = (m10 * RHS.m03) + (m11 * RHS.m13) + (m12 * RHS.m23) + (m13 * RHS.m33);

        Result.m20 = (m20 * RHS.m00) + (m21 * RHS.m10) + (m22 * RHS.m20) + (m23 * RHS.m30);
        Result.m21 = (m20 * RHS.m01) + (m21 * RHS.m11) + (m22 * RHS.m21) + (m23 * RHS.m31);
        Result.m22 = (m20 * RHS.m02) + (m21 * RHS.m12) + (m22 * RHS.m22) + (m23 * RHS.m32);
        Result.m23 = (m20 * RHS.m03) + (m21 * RHS.m13) + (m22 * RHS.m23) + (m23 * RHS.m33);

        Result.m30 = (m30 * RHS.m00) + (m31 * RHS.m10) + (m32 * RHS.m20) + (m33 * RHS.m30);
        Result.m31 = (m30 * RHS.m01) + (m31 * RHS.m11) + (m32 * RHS.m21) + (m33 * RHS.m31);
        Result.m32 = (m30 * RHS.m02) + (m31 * RHS.m12) + (m32 * RHS.m22) + (m33 * RHS.m32);
        Result.m33 = (m30 * RHS.m03) + (m31 * RHS.m13) + (m32 * RHS.m23) + (m33 * RHS.m33);
    #else
        FFloat128 Row0 = FVectorMath::LoadAligned(f[0]);
        Row0 = FVectorMath::VectorTransform(reinterpret_cast<const float*>(&RHS), Row0);

        FFloat128 Row1 = FVectorMath::LoadAligned(f[1]);
        Row1 = FVectorMath::VectorTransform(reinterpret_cast<const float*>(&RHS), Row1);

        FFloat128 Row2 = FVectorMath::LoadAligned(f[2]);
        Row2 = FVectorMath::VectorTransform(reinterpret_cast<const float*>(&RHS), Row2);

        FFloat128 Row3 = FVectorMath::LoadAligned(f[3]);
        Row3 = FVectorMath::VectorTransform(reinterpret_cast<const float*>(&RHS), Row3);

        FVectorMath::StoreAligned(Row0, Result.f[0]);
        FVectorMath::StoreAligned(Row1, Result.f[1]);
        FVectorMath::StoreAligned(Row2, Result.f[2]);
        FVectorMath::StoreAligned(Row3, Result.f[3]);
    #endif

        return Result;
    }

    /**
     * @brief     - Multiplies this matrix with another matrix
     * @param RHS - The other matrix
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix4& operator*=(const FMatrix4& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Multiplies a matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A matrix containing the result of the multiplication
     */
    FORCEINLINE FMatrix4 operator*(float RHS) const noexcept
    {
        FMatrix4 Result;
    
    #if !USE_VECTOR_MATH
        Result.m00 = m00 * RHS;
        Result.m01 = m01 * RHS;
        Result.m02 = m02 * RHS;
        Result.m03 = m03 * RHS;

        Result.m10 = m10 * RHS;
        Result.m11 = m11 * RHS;
        Result.m12 = m12 * RHS;
        Result.m13 = m13 * RHS;

        Result.m20 = m20 * RHS;
        Result.m21 = m21 * RHS;
        Result.m22 = m22 * RHS;
        Result.m23 = m23 * RHS;

        Result.m30 = m30 * RHS;
        Result.m31 = m31 * RHS;
        Result.m32 = m32 * RHS;
        Result.m33 = m33 * RHS;
    #else
        FFloat128 Scalars = FVectorMath::Load(RHS);
        FFloat128 Row0    = FVectorMath::VectorMul(f[0], Scalars);
        FFloat128 Row1    = FVectorMath::VectorMul(f[1], Scalars);
        FFloat128 Row2    = FVectorMath::VectorMul(f[2], Scalars);
        FFloat128 Row3    = FVectorMath::VectorMul(f[3], Scalars);

        FVectorMath::StoreAligned(Row0, Result.f[0]);
        FVectorMath::StoreAligned(Row1, Result.f[1]);
        FVectorMath::StoreAligned(Row2, Result.f[2]);
        FVectorMath::StoreAligned(Row3, Result.f[3]);
    #endif
    
        return Result;
    }

    /**
     * @brief     - Multiplies this matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix4& operator*=(float RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Adds a matrix component-wise with another matrix
     * @param RHS - The other matrix
     * @return    - A matrix containing the result of the addition
     */
    FORCEINLINE FMatrix4 operator+(const FMatrix4& RHS) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.m00 = m00 + RHS.m00;
        Result.m01 = m01 + RHS.m01;
        Result.m02 = m02 + RHS.m02;
        Result.m03 = m03 + RHS.m03;

        Result.m10 = m10 + RHS.m10;
        Result.m11 = m11 + RHS.m11;
        Result.m12 = m12 + RHS.m12;
        Result.m13 = m13 + RHS.m13;

        Result.m20 = m20 + RHS.m20;
        Result.m21 = m21 + RHS.m21;
        Result.m22 = m22 + RHS.m22;
        Result.m23 = m23 + RHS.m23;

        Result.m30 = m30 + RHS.m30;
        Result.m31 = m31 + RHS.m31;
        Result.m32 = m32 + RHS.m32;
        Result.m33 = m33 + RHS.m33;
    #else
        FFloat128 Row0 = FVectorMath::VectorAdd(f[0], RHS.f[0]);
        FFloat128 Row1 = FVectorMath::VectorAdd(f[1], RHS.f[1]);
        FFloat128 Row2 = FVectorMath::VectorAdd(f[2], RHS.f[2]);
        FFloat128 Row3 = FVectorMath::VectorAdd(f[3], RHS.f[3]);

        FVectorMath::StoreAligned(Row0, Result.f[0]);
        FVectorMath::StoreAligned(Row1, Result.f[1]);
        FVectorMath::StoreAligned(Row2, Result.f[2]);
        FVectorMath::StoreAligned(Row3, Result.f[3]);
    #endif
    
        return Result;
    }

    /**
     * @brief     - Adds this matrix component-wise with another matrix
     * @param RHS - The other matrix
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix4& operator+=(const FMatrix4& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Adds a matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A matrix containing the result of the addition
     */
    FORCEINLINE FMatrix4 operator+(float RHS) const noexcept
    {
        FMatrix4 Result;
    
    #if !USE_VECTOR_MATH
        Result.m00 = m00 + RHS;
        Result.m01 = m01 + RHS;
        Result.m02 = m02 + RHS;
        Result.m03 = m03 + RHS;

        Result.m10 = m10 + RHS;
        Result.m11 = m11 + RHS;
        Result.m12 = m12 + RHS;
        Result.m13 = m13 + RHS;

        Result.m20 = m20 + RHS;
        Result.m21 = m21 + RHS;
        Result.m22 = m22 + RHS;
        Result.m23 = m23 + RHS;

        Result.m30 = m30 + RHS;
        Result.m31 = m31 + RHS;
        Result.m32 = m32 + RHS;
        Result.m33 = m33 + RHS;
    #else
        FFloat128 Scalars = FVectorMath::Load(RHS);
        FFloat128 Row0    = FVectorMath::VectorAdd(f[0], Scalars);
        FFloat128 Row1    = FVectorMath::VectorAdd(f[1], Scalars);
        FFloat128 Row2    = FVectorMath::VectorAdd(f[2], Scalars);
        FFloat128 Row3    = FVectorMath::VectorAdd(f[3], Scalars);

        FVectorMath::StoreAligned(Row0, Result.f[0]);
        FVectorMath::StoreAligned(Row1, Result.f[1]);
        FVectorMath::StoreAligned(Row2, Result.f[2]);
        FVectorMath::StoreAligned(Row3, Result.f[3]);
    #endif

        return Result;
    }

    /**
     * @brief     - Adds this matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix4& operator+=(float RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Subtracts a matrix component-wise with another matrix
     * @param RHS - The other matrix
     * @return    - A matrix containing the result of the subtraction
     */
    FORCEINLINE FMatrix4 operator-(const FMatrix4& RHS) const noexcept
    {
        FMatrix4 Result;
    
    #if !USE_VECTOR_MATH
        Result.m00 = m00 - RHS.m00;
        Result.m01 = m01 - RHS.m01;
        Result.m02 = m02 - RHS.m02;
        Result.m03 = m03 - RHS.m03;

        Result.m10 = m10 - RHS.m10;
        Result.m11 = m11 - RHS.m11;
        Result.m12 = m12 - RHS.m12;
        Result.m13 = m13 - RHS.m13;

        Result.m20 = m20 - RHS.m20;
        Result.m21 = m21 - RHS.m21;
        Result.m22 = m22 - RHS.m22;
        Result.m23 = m23 - RHS.m23;

        Result.m30 = m30 - RHS.m30;
        Result.m31 = m31 - RHS.m31;
        Result.m32 = m32 - RHS.m32;
        Result.m33 = m33 - RHS.m33;
    #else
        FFloat128 Row0 = FVectorMath::VectorSub(f[0], RHS.f[0]);
        FFloat128 Row1 = FVectorMath::VectorSub(f[1], RHS.f[1]);
        FFloat128 Row2 = FVectorMath::VectorSub(f[2], RHS.f[2]);
        FFloat128 Row3 = FVectorMath::VectorSub(f[3], RHS.f[3]);

        FVectorMath::StoreAligned(Row0, Result.f[0]);
        FVectorMath::StoreAligned(Row1, Result.f[1]);
        FVectorMath::StoreAligned(Row2, Result.f[2]);
        FVectorMath::StoreAligned(Row3, Result.f[3]);
    #endif
    
        return Result;
    }

    /**
     * @brief     - Subtracts this matrix component-wise with another matrix
     * @param RHS - The other matrix
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix4& operator-=(const FMatrix4& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Subtracts a matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A matrix containing the result of the subtraction
     */
    FORCEINLINE FMatrix4 operator-(float RHS) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.m00 = m00 - RHS;
        Result.m01 = m01 - RHS;
        Result.m02 = m02 - RHS;
        Result.m03 = m03 - RHS;

        Result.m10 = m10 - RHS;
        Result.m11 = m11 - RHS;
        Result.m12 = m12 - RHS;
        Result.m13 = m13 - RHS;

        Result.m20 = m20 - RHS;
        Result.m21 = m21 - RHS;
        Result.m22 = m22 - RHS;
        Result.m23 = m23 - RHS;

        Result.m30 = m30 - RHS;
        Result.m31 = m31 - RHS;
        Result.m32 = m32 - RHS;
        Result.m33 = m33 - RHS;
    #else
        FFloat128 Scalars = FVectorMath::Load(RHS);
        FFloat128 Row0    = FVectorMath::VectorSub(f[0], Scalars);
        FFloat128 Row1    = FVectorMath::VectorSub(f[1], Scalars);
        FFloat128 Row2    = FVectorMath::VectorSub(f[2], Scalars);
        FFloat128 Row3    = FVectorMath::VectorSub(f[3], Scalars);

        FVectorMath::StoreAligned(Row0, Result.f[0]);
        FVectorMath::StoreAligned(Row1, Result.f[1]);
        FVectorMath::StoreAligned(Row2, Result.f[2]);
        FVectorMath::StoreAligned(Row3, Result.f[3]);
    #endif
    
        return Result;
    }

    /**
     * @brief     - Subtracts this matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix4& operator-=(float RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Divides a matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A matrix containing the result of the division
     */
    FORCEINLINE FMatrix4 operator/(float RHS) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        const float Recip = 1.0f / RHS;
        Result.m00 = m00 * Recip;
        Result.m01 = m01 * Recip;
        Result.m02 = m02 * Recip;
        Result.m03 = m03 * Recip;

        Result.m10 = m10 * Recip;
        Result.m11 = m11 * Recip;
        Result.m12 = m12 * Recip;
        Result.m13 = m13 * Recip;

        Result.m20 = m20 * Recip;
        Result.m21 = m21 * Recip;
        Result.m22 = m22 * Recip;
        Result.m23 = m23 * Recip;

        Result.m30 = m30 * Recip;
        Result.m31 = m31 * Recip;
        Result.m32 = m32 * Recip;
        Result.m33 = m33 * Recip;
    #else
        FFloat128 RecipScalars = FVectorMath::Load(1.0f / RHS);
        FFloat128 Row0 = FVectorMath::VectorMul(f[0], RecipScalars);
        FFloat128 Row1 = FVectorMath::VectorMul(f[1], RecipScalars);
        FFloat128 Row2 = FVectorMath::VectorMul(f[2], RecipScalars);
        FFloat128 Row3 = FVectorMath::VectorMul(f[3], RecipScalars);

        FVectorMath::StoreAligned(Row0, Result.f[0]);
        FVectorMath::StoreAligned(Row1, Result.f[1]);
        FVectorMath::StoreAligned(Row2, Result.f[2]);
        FVectorMath::StoreAligned(Row3, Result.f[3]);
    #endif
    
        return Result;
    }

    /**
     * @brief     - Divides this matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix4& operator/=(float RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief       - Returns the result after comparing this and another matrix
     * @param Other - The matrix to compare with
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool operator==(const FMatrix4& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief       - Returns the negated result after comparing this and another matrix
     * @param Other - The matrix to compare with
     * @return      - False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FMatrix4& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    /**
     * @brief  - Creates and returns a identity matrix
     * @return - A identity matrix
     */
    static FORCEINLINE FMatrix4 Identity() noexcept
    {
        return FMatrix4(1.0f);
    }

    /**
     * @brief       - Creates and returns a uniform scale matrix
     * @param Scale - Uniform scale that represents this matrix
     * @return      - A scale matrix
     */
    static FORCEINLINE FMatrix4 Scale(float Scale) noexcept
    {
        return FMatrix4(
            Scale, 0.0f, 0.0f, 0.0f,
            0.0f, Scale, 0.0f, 0.0f,
            0.0f, 0.0f, Scale, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief   - Creates and returns a scale matrix for each axis
     * @param x - Scale for the x-axis
     * @param y - Scale for the y-axis
     * @param z - Scale for the z-axis
     * @return  - A scale matrix
     */
    static FORCEINLINE FMatrix4 Scale(float x, float y, float z) noexcept
    {
        return FMatrix4(
            x, 0.0f, 0.0f, 0.0f,
            0.0f, y, 0.0f, 0.0f,
            0.0f, 0.0f, z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief                 - Creates and returns a scale matrix for each axis
     * @param VectorWithScale - A vector containing the scale for each axis in the x-, y-, z-components
     * @return                - A scale matrix
     */
    static FORCEINLINE FMatrix4 Scale(const FVector3& VectorWithScale) noexcept
    {
        return Scale(VectorWithScale.x, VectorWithScale.y, VectorWithScale.z);
    }

    /**
     * @brief   - Creates and returns a translation matrix
     * @param x - Translation for the x-axis
     * @param y - Translation for the y-axis
     * @param z - Translation for the z-axis
     * @return  - A translation matrix
     */
    static FORCEINLINE FMatrix4 Translation(float x, float y, float z) noexcept
    {
        return FMatrix4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            x, y, z, 1.0f);
    }

    /**
     * @brief             - Creates and returns a translation matrix
     * @param Translation - A vector containing the translation
     * @return            - A translation matrix
     */
    static FORCEINLINE FMatrix4 Translation(const FVector3& InTranslation) noexcept
    {
        return Translation(InTranslation.x, InTranslation.y, InTranslation.z);
    }

    /**
     * @brief       - Creates and returns a rotation matrix from Roll, pitch, and Yaw in radians
     * @param Pitch - Rotation around the x-axis in radians
     * @param Yaw   - Rotation around the y-axis in radians
     * @param Roll  - Rotation around the z-axis in radians
     * @return      - A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationRollPitchYaw(float Pitch, float Yaw, float Roll) noexcept
    {
        const float SinP = FMath::Sin(Pitch);
        const float SinY = FMath::Sin(Yaw);
        const float SinR = FMath::Sin(Roll);
        const float CosP = FMath::Cos(Pitch);
        const float CosY = FMath::Cos(Yaw);
        const float CosR = FMath::Cos(Roll);

        const float SinRSinP = SinR * SinP;
        const float CosRSinP = CosR * SinP;

        return FMatrix4(
            (CosR * CosY) + (SinRSinP * SinY), (SinR * CosP), (SinRSinP * CosY) - (CosR * SinY), 0.0f,
            (CosRSinP * SinY) - (SinR * CosY), (CosR * CosP), (SinR * SinY) + (CosRSinP * CosY), 0.0f,
            (CosP * SinY), -SinP, (CosP * CosY), 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief              - Creates and returns a rotation matrix from Roll, pitch, and Yaw in radians
     * @param PitchYawRoll - A vector containing the PitchYawRoll (x = Pitch, y = Yaw, z = Roll)
     * @return             - A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationRollPitchYaw(const FVector3& PitchYawRoll) noexcept
    {
        return RotationRollPitchYaw(PitchYawRoll.x, PitchYawRoll.y, PitchYawRoll.z);
    }

    /**
     * @brief   - Creates and returns a rotation matrix around the x-axis
     * @param x - Rotation around the x-axis in radians
     * @return  - A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationX(float x) noexcept
    {
        const float SinX = FMath::Sin(x);
        const float CosX = FMath::Cos(x);

        return FMatrix4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, CosX, SinX, 0.0f,
            0.0f, -SinX, CosX, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief   - Creates and returns a rotation matrix around the y-axis
     * @param y - Rotation around the y-axis in radians
     * @return  - A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationY(float y) noexcept
    {
        const float SinY = FMath::Sin(y);
        const float CosY = FMath::Cos(y);

        return FMatrix4(
            CosY, 0.0f, -SinY, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            SinY, 0.0f, CosY, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief   - Creates and returns a rotation matrix around the z-axis
     * @param z - Rotation around the z-axis in radians
     * @return  - A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationZ(float z) noexcept
    {
        const float SinZ = FMath::Sin(z);
        const float CosZ = FMath::Cos(z);

        return FMatrix4(
            CosZ, SinZ, 0.0f, 0.0f,
            -SinZ, CosZ, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief        - Creates a orthographic-projection matrix (Left-handed)
     * @param Width  - Width of the projection plane in pixels
     * @param Height - Height of the projection plane in pixels
     * @param NearZ  - The distance to the near plane in world-units
     * @param FarZ   - The distance to the far plane in world-units
     * @return       - A orthographic-projection matrix
     */
    static FORCEINLINE FMatrix4 OrtographicProjection(float Width, float Height, float NearZ, float FarZ) noexcept
    {
        return FMatrix4(
            2.0f / Width, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / Height, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f / (FarZ - NearZ), 0.0f,
            0.0f, 0.0f, -NearZ / (FarZ - NearZ), 1.0f);
    }

    /**
     * @brief        - Creates a orthographic-projection matrix (Left-handed)
     * @param Left   - Negative offset on the x-axis in world-units
     * @param Right  - Positive offset on the x-axis in world-units
     * @param Bottom - Negative offset on the y-axis in world-units
     * @param Top    - Positive offset on the y-axis in world-units
     * @param NearZ  - The distance to the near plane in world-units
     * @param FarZ   - The distance to the far plane in world-units
     * @return       - A orthographic-projection matrix
     */
    static FORCEINLINE FMatrix4 OrtographicProjection(float Left, float Right, float Bottom, float Top, float NearZ, float FarZ) noexcept
    {
        const float InvWidth  = 1.0f / (Right - Left);
        const float InvHeight = 1.0f / (Top - Bottom);
        const float Range     = 1.0f / (FarZ - NearZ);

        return FMatrix4(
             InvWidth + InvWidth, 0.0f, 0.0f, 0.0f,
             0.0f, InvHeight + InvHeight, 0.0f, 0.0f,
             0.0f, 0.0f, Range, 0.0f,
            -(Left + Right) * InvWidth, -(Top + Bottom) * InvHeight, -Range * NearZ, 1.0f);
    }

    /**
     * @brief             - Creates a perspective-projection matrix (Left-handed)
     * @param Fov         - Field of view of the projection in radians
     * @param AspectRatio - Aspect ratio of the projection (Width / Height)
     * @param NearZ       - The distance to the near plane in world-units
     * @param FarZ        - The distance to the far plane in world-units
     * @return            - A perspective-projection matrix
     */
    static FORCEINLINE FMatrix4 PerspectiveProjection(float Fov, float AspectRatio, float NearZ, float FarZ) noexcept
    {
        if ((Fov < FMath::kOneDegree_f) || (Fov > (FMath::kPI_f - FMath::kOneDegree_f)))
        {
            return FMatrix4();
        }

        const float ScaleY = (1.0f / FMath::Tan(Fov * 0.5f));
        const float ScaleX = (ScaleY / AspectRatio);
        const float Range  = (FarZ / (FarZ - NearZ));

        return FMatrix4(
            ScaleX, 0.0f, 0.0f, 0.0f,
            0.0f, ScaleY, 0.0f, 0.0f,
            0.0f, 0.0f, Range, 1.0f,
            0.0f, 0.0f, -Range * NearZ, 0.0f);
    }

    /**
     * @brief        - Creates a perspective-projection matrix (Left-handed)
     * @param Fov    - Field of view of the projection in radians
     * @param Width  - Width of the projection plane in pixels
     * @param Height - Height of the projection plane in pixels
     * @param NearZ  - The distance to the near plane in world-units
     * @param FarZ   - The distance to the far plane in world-units
     * @return       - A perspective-projection matrix
     */
    static FORCEINLINE FMatrix4 PerspectiveProjection(float Fov, float Width, float Height, float NearZ, float FarZ) noexcept
    {
        const float AspectRatio = Width / Height;
        return PerspectiveProjection(Fov, AspectRatio, NearZ, FarZ);
    }

    /**
     * @brief     - Creates a look-at matrix (Left-handed)
     * @param Eye - Position to look from
     * @param At  - Position to look at
     * @param Up  - The up-axis of the new coordinate system in the current world-space
     * @return    - A look-at matrix
     */
    static FORCEINLINE FMatrix4 LookAt(const FVector3& Eye, const FVector3& At, const FVector3& Up) noexcept
    {
        const FVector3 Direction = At - Eye;
        return LookTo(Eye, Direction, Up);
    }

    /**
     * @brief           - Creates a look-to matrix (Left-handed)
     * @param Eye       - Position to look from
     * @param Direction - Direction to look in
     * @param Up        - The up-axis of the new coordinate system in the current world-space
     * @return          - A look-to matrix
     */
    static FORCEINLINE FMatrix4 LookTo(const FVector3& Eye, const FVector3& Direction, const FVector3& Up) noexcept
    {
        FVector3 e2 = Direction.GetNormalized();
        FVector3 e0 = Up.CrossProduct(e2).GetNormalized();
        FVector3 e1 = e2.CrossProduct(e0).GetNormalized();

        FVector3 NegEye = -Eye;

        const float m30 = NegEye.DotProduct(e0);
        const float m31 = NegEye.DotProduct(e1);
        const float m32 = NegEye.DotProduct(e2);

        FMatrix4 Result(FVector4(e0, m30), FVector4(e1, m31), FVector4(e2, m32), FVector4(0.0f, 0.0f, 0.0f, 1.0f));
        return Result.Transpose();
    }

public:
    union
    {
         /** @brief - Each element of the matrix */
        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };

         /** @brief - 2-D array of the matrix */
        float f[4][4];
    };
};

MARK_AS_REALLOCATABLE(FMatrix4);
