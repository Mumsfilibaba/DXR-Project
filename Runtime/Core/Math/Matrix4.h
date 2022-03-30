#pragma once
#include "Vector3.h"
#include "Matrix3.h"
#include "Vector4.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 4x4 matrix with SIMD capabilities

class VECTOR_ALIGN CMatrix4
{
public:

    /** Default constructor (Initialize components to zero) */
    FORCEINLINE CMatrix4() noexcept;

    /**
     * @brief: Constructor initializing all values on the diagonal with a single value. The other values are set to zero.
     *
     * @param Diagonal: Value to set on the diagonal
     */
    FORCEINLINE explicit CMatrix4(float Diagonal) noexcept;

    /**
     * @brief: Constructor initializing all values with vectors specifying each row
     *
     * @param Row0: Vector to set the first row to
     * @param Row1: Vector to set the second row to
     * @param Row2: Vector to set the third row to
     * @param Row3: Vector to set the fourth row to
     */
    FORCEINLINE explicit CMatrix4(const CVector4& Row0, const CVector4& Row1, const CVector4& Row2, const CVector4& Row3) noexcept;

    /**
     * @brief: Constructor initializing all values with corresponding value
     *
     * @param In00: Value to set on row 0 and column 0
     * @param In01: Value to set on row 0 and column 1
     * @param In02: Value to set on row 0 and column 2
     * @param In03: Value to set on row 0 and column 3
     * @param In10: Value to set on row 1 and column 0
     * @param In11: Value to set on row 1 and column 1
     * @param In12: Value to set on row 1 and column 2
     * @param In13: Value to set on row 1 and column 3
     * @param In20: Value to set on row 2 and column 0
     * @param In21: Value to set on row 2 and column 1
     * @param In22: Value to set on row 2 and column 2
     * @param In23: Value to set on row 2 and column 3
     * @param In30: Value to set on row 3 and column 0
     * @param In31: Value to set on row 3 and column 1
     * @param In32: Value to set on row 3 and column 2
     * @param In33: Value to set on row 3 and column 3
     */
    FORCEINLINE explicit CMatrix4(
        float In00, float In01, float In02, float In03,
        float In10, float In11, float In12, float In13,
        float In20, float In21, float In22, float In23,
        float In30, float In31, float In32, float In33) noexcept;

    /**
     * @brief: Constructor initializing all components with an array
     *
     * @param Arr: Array with at least 16 elements
     */
    FORCEINLINE explicit CMatrix4(const float* Arr) noexcept;

    /**
     * @brief: Transform a 3-D vector as position, fourth component to one
     *
     * @param Position: Vector to transform
     * @return Transformed vector
     */
    FORCEINLINE CVector3 TransformPosition(const CVector3& Position) noexcept;

    /**
     * @brief: Transform a 3-D vector as direction, fourth component to zero
     *
     * @param Direction: Vector to transform
     * @return Transformed vector
     */
    FORCEINLINE CVector3 TransformDirection(const CVector3& Direction) noexcept;

    /**
     * @brief: Returns the transposed version of this matrix
     *
     * @return Transposed matrix
     */
    inline CMatrix4 Transpose() const noexcept;

    /**
     * @brief: Returns the inverted version of this matrix
     *
     * @return Inverse matrix
     */
    inline CMatrix4 Invert() const noexcept;

    /**
     * @brief: Returns the adjugate of this matrix
     *
     * @return Adjugate matrix
     */
    inline CMatrix4 Adjoint() const noexcept;

    /**
     * @brief: Returns the determinant of this matrix
     *
     * @return The determinant
     */
    inline float Determinant() const noexcept;

    /**
     * @brief: Checks weather this matrix has any value that equals NaN
     *
     * @return True if the any value equals NaN, false if not
     */
    inline bool HasNan() const noexcept;

    /**
     * @brief: Checks weather this matrix has any value that equals infinity
     *
     * @return True if the any value equals infinity, false if not
     */
    inline bool HasInfinity() const noexcept;

    /**
     * @brief: Checks weather this matrix has any value that equals infinity or NaN
     *
     * @return False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept;

    /**
     * @brief: Compares, within a threshold Epsilon, this matrix with another matrix
     *
     * @param Other: matrix to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual(const CMatrix4& Other, float Epsilon = NMath::IS_EQUAL_EPISILON) const noexcept;

    /* Sets this matrix to an identity matrix */
    FORCEINLINE void SetIdentity() noexcept;

    /**
     * @brief: Sets the upper 3x3 matrix
     *
     * @param RotationAndScale: 3x3 to set the upper quadrant to
     */
    FORCEINLINE void SetRotationAndScale(const CMatrix3& RotationAndScale) noexcept;

    /**
     * @brief: Sets the translation part of a matrix
     *
     * @param Translation: The translation part
     */
    FORCEINLINE void SetTranslation(const CVector3& Translation) noexcept;

    /**
     * @brief: Returns a row of this matrix
     *
     * @param Row: The row to retrive
     * @return A vector containing the specified row
     */
    FORCEINLINE CVector4 GetRow(int Row) const noexcept;

    /**
     * @brief: Returns a column of this matrix
     *
     * @param Column: The column to retrive
     * @return A vector containing the specified column
     */
    FORCEINLINE CVector4 GetColumn(int Column) const noexcept;

    /**
     * @brief: Returns the translation part of this matrix, that is
     * the x-, y-, and z-coordinates of the fourth row
     *
     * @return A vector containing the translation
     */
    FORCEINLINE CVector3 GetTranslation() const noexcept;

    /**
     * @brief: Returns the 3x3 matrix thats forming the upper quadrant of this matrix.
     *
     * @return A matrix containing the upper part of the matrix
     */
    FORCEINLINE CMatrix3 GetRotationAndScale() const noexcept;

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE float* GetData() noexcept;

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE const float* GetData() const noexcept;

public:
    /**
     * @brief: Transforms a 4-D vector
     *
     * @param Rhs: The vector to transform
     * @return A vector containing the transformation
     */
    FORCEINLINE CVector4 operator*(const CVector4& Rhs) const noexcept;

    /**
     * @brief: Multiplies a matrix with another matrix
     *
     * @param Rhs: The other matrix
     * @return A matrix containing the result of the multiplication
     */
    FORCEINLINE CMatrix4 operator*(const CMatrix4& Rhs) const noexcept;

    /**
     * @brief: Multiplies this matrix with another matrix
     *
     * @param Rhs: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix4& operator*=(const CMatrix4& Rhs) noexcept;

    /**
     * @brief: Multiplies a matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A matrix containing the result of the multiplication
     */
    FORCEINLINE CMatrix4 operator*(float Rhs) const noexcept;

    /**
     * @brief: Multiplies this matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix4& operator*=(float Rhs) noexcept;

    /**
     * @brief: Adds a matrix component-wise with another matrix
     *
     * @param Rhs: The other matrix
     * @return A matrix containing the result of the addition
     */
    FORCEINLINE CMatrix4 operator+(const CMatrix4& Rhs) const noexcept;

    /**
     * @brief: Adds this matrix component-wise with another matrix
     *
     * @param Rhs: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix4& operator+=(const CMatrix4& Rhs) noexcept;

    /**
     * @brief: Adds a matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A matrix containing the result of the addition
     */
    FORCEINLINE CMatrix4 operator+(float Rhs) const noexcept;

    /**
     * @brief: Adds this matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix4& operator+=(float Rhs) noexcept;

    /**
     * @brief: Subtracts a matrix component-wise with another matrix
     *
     * @param Rhs: The other matrix
     * @return A matrix containing the result of the subtraction
     */
    FORCEINLINE CMatrix4 operator-(const CMatrix4& Rhs) const noexcept;

    /**
     * @brief: Subtracts this matrix component-wise with another matrix
     *
     * @param Rhs: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix4& operator-=(const CMatrix4& Rhs) noexcept;

    /**
     * @brief: Subtracts a matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A matrix containing the result of the subtraction
     */
    FORCEINLINE CMatrix4 operator-(float Rhs) const noexcept;

    /**
     * @brief: Subtracts this matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix4& operator-=(float Rhs) noexcept;

    /**
     * @brief: Divides a matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A matrix containing the result of the division
     */
    FORCEINLINE CMatrix4 operator/(float Rhs) const noexcept;

    /**
     * @brief: Divides this matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix4& operator/=(float Rhs) noexcept;

    /**
     * @brief: Returns the result after comparing this and another matrix
     *
     * @param Other: The matrix to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CMatrix4& Other) const noexcept;

    /**
     * @brief: Returns the negated result after comparing this and another matrix
     *
     * @param Other: The matrix to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CMatrix4& Other) const noexcept;

public:
    /**
     * @brief: Creates and returns a identity matrix
     *
     * @return A identity matrix
     */
    inline static CMatrix4 Identity() noexcept;

    /**
     * @brief: Creates and returns a uniform scale matrix
     *
     * @param Scale: Uniform scale that represents this matrix
     * @return A scale matrix
     */
    inline static CMatrix4 Scale(float Scale) noexcept;

    /**
     * @brief: Creates and returns a scale matrix for each axis
     *
     * @param x: Scale for the x-axis
     * @param y: Scale for the y-axis
     * @param z: Scale for the z-axis
     * @return A scale matrix
     */
    inline static CMatrix4 Scale(float x, float y, float z) noexcept;

    /**
     * @brief: Creates and returns a scale matrix for each axis
     *
     * @param VectorWithScale: A vector containing the scale for each axis in the x-, y-, z-components
     * @return A scale matrix
     */
    inline static CMatrix4 Scale(const CVector3& VectorWithScale) noexcept;

    /**
     * @brief: Creates and returns a translation matrix
     *
     * @param x: Translation for the x-axis
     * @param y: Translation for the y-axis
     * @param z: Translation for the z-axis
     * @return A translation matrix
     */
    inline static CMatrix4 Translation(float x, float y, float z) noexcept;

    /**
     * @brief: Creates and returns a translation matrix
     *
     * @param Translation: A vector containing the translation
     * @return A translation matrix
     */
    inline static CMatrix4 Translation(const CVector3& Translation) noexcept;

    /**
     * @brief: Creates and returns a rotation matrix from Roll, pitch, and Yaw in radians
     *
     * @param Pitch: Rotation around the x-axis in radians
     * @param Yaw: Rotation around the y-axis in radians
     * @param Roll: Rotation around the z-axis in radians
     * @return A rotation matrix
     */
    inline static CMatrix4 RotationRollPitchYaw(float Pitch, float Yaw, float Roll) noexcept;

    /**
     * @brief: Creates and returns a rotation matrix from Roll, pitch, and Yaw in radians
     *
     * @param PitchYawRoll: A vector containing the PitchYawRoll (x = Pitch, y = Yaw, z = Roll)
     * @return A rotation matrix
     */
    inline static CMatrix4 RotationRollPitchYaw(const CVector3& PitchYawRoll) noexcept;

    /**
     * @brief: Creates and returns a rotation matrix around the x-axis
     *
     * @param x: Rotation around the x-axis in radians
     * @return A rotation matrix
     */
    inline static CMatrix4 RotationX(float x) noexcept;

    /**
     * @brief: Creates and returns a rotation matrix around the y-axis
     *
     * @param y: Rotation around the y-axis in radians
     * @return A rotation matrix
     */
    inline static CMatrix4 RotationY(float y) noexcept;

    /**
     * @brief: Creates and returns a rotation matrix around the z-axis
     *
     * @param z: Rotation around the z-axis in radians
     * @return A rotation matrix
     */
    inline static CMatrix4 RotationZ(float z) noexcept;

    /**
     * @brief: Creates a ortographic-projection matrix (Left-handed)
     *
     * @param Width: Width of the projection plane in pixels
     * @param Height: Height of the projection plane in pixels
     * @param NearZ: The distance to the near plane in world-units
     * @param FarZ: The distance to the far plane in world-units
     * @return A ortographic-projection matrix
     */
    inline static CMatrix4 OrtographicProjection(float Width, float Height, float NearZ, float FarZ) noexcept;

    /**
     * @brief: Creates a ortographic-projection matrix (Left-handed)
     *
     * @param Left: Negative offset on the x-axis in world-units
     * @param Right: Positive offset on the x-axis in world-units
     * @param Bottom: Negative offset on the y-axis in world-units
     * @param Top: Positive offset on the y-axis in world-units
     * @param NearZ: The distance to the near plane in world-units
     * @param FarZ: The distance to the far plane in world-units
     * @return A ortographic-projection matrix
     */
    inline static CMatrix4 OrtographicProjection(float Left, float Right, float Bottom, float Top, float NearZ, float FarZ) noexcept;

    /**
     * @brief: Creates a perspective-projection matrix (Left-handed)
     *
     * @param Fov: Field of view of the projection in radians
     * @param AspectRatio: Aspect ratio of the projection (Width / Height)
     * @param NearZ: The distance to the near plane in world-units
     * @param FarZ: The distance to the far plane in world-units
     * @return A perspective-projection matrix
     */
    inline static CMatrix4 PerspectiveProjection(float Fov, float AspectRatio, float NearZ, float FarZ) noexcept;

    /**
     * @brief: Creates a perspective-projection matrix (Left-handed)
     *
     * @param Fov: Field of view of the projection in radians
     * @param Width: Width of the projection plane in pixels
     * @param Height: Height of the projection plane in pixels
     * @param NearZ: The distance to the near plane in world-units
     * @param FarZ: The distance to the far plane in world-units
     * @return A perspective-projection matrix
     */
    inline static CMatrix4 PerspectiveProjection(float Fov, float Width, float Height, float NearZ, float FarZ) noexcept;

    // Create a lookat matrix (Left-handed)

    /**
     * @brief: Creates a look-at matrix (Left-handed)
     *
     * @param Eye: Position to look from
     * @param At: Position to look at
     * @param Up: The up-axis of the new coordinate system in the current world-space
     * @return A look-at matrix
     */
    inline static CMatrix4 LookAt(const CVector3& Eye, const CVector3& At, const CVector3& Up) noexcept;

    /**
     * @brief: Creates a look-to matrix (Left-handed)
     *
     * @param Eye: Position to look from
     * @param Direction: Direction to look in
     * @param Up: The up-axis of the new coordinate system in the current world-space
     * @return A look-to matrix
     */
    inline static CMatrix4 LookTo(const CVector3& Eye, const CVector3& Direction, const CVector3& Up) noexcept;

public:
    union
    {
        /* Each element of the matrix */
        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };

        /* 2-D array of the matrix */
        float f[4][4];
    };
};

FORCEINLINE CMatrix4::CMatrix4() noexcept
    : m00(0.0f), m01(0.0f), m02(0.0f), m03(0.0f)
    , m10(0.0f), m11(0.0f), m12(0.0f), m13(0.0f)
    , m20(0.0f), m21(0.0f), m22(0.0f), m23(0.0f)
    , m30(0.0f), m31(0.0f), m32(0.0f), m33(0.0f)
{
}

FORCEINLINE CMatrix4::CMatrix4(float Diagonal) noexcept
    : m00(Diagonal), m01(0.0f), m02(0.0f), m03(0.0f)
    , m10(0.0f), m11(Diagonal), m12(0.0f), m13(0.0f)
    , m20(0.0f), m21(0.0f), m22(Diagonal), m23(0.0f)
    , m30(0.0f), m31(0.0f), m32(0.0f), m33(Diagonal)
{
}

FORCEINLINE CMatrix4::CMatrix4(const CVector4& Row0, const CVector4& Row1, const CVector4& Row2, const CVector4& Row3) noexcept
    : m00(Row0.x), m01(Row0.y), m02(Row0.z), m03(Row0.w)
    , m10(Row1.x), m11(Row1.y), m12(Row1.z), m13(Row1.w)
    , m20(Row2.x), m21(Row2.y), m22(Row2.z), m23(Row2.w)
    , m30(Row3.x), m31(Row3.y), m32(Row3.z), m33(Row3.w)
{
}

FORCEINLINE CMatrix4::CMatrix4(
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

FORCEINLINE CMatrix4::CMatrix4(const float* Arr) noexcept
    : m00(Arr[0]), m01(Arr[1]), m02(Arr[2]), m03(Arr[3])
    , m10(Arr[4]), m11(Arr[5]), m12(Arr[6]), m13(Arr[7])
    , m20(Arr[8]), m21(Arr[9]), m22(Arr[10]), m23(Arr[11])
    , m30(Arr[12]), m31(Arr[13]), m32(Arr[14]), m33(Arr[15])
{
}

FORCEINLINE CVector3 CMatrix4::TransformPosition(const CVector3& Position) noexcept
{
#if defined(DISABLE_SIMD)

    CVector3 Result;
    Result.x = (Position[0] * m00) + (Position[1] * m10) + (Position[2] * m20) + (1.0f * m30);
    Result.y = (Position[0] * m01) + (Position[1] * m11) + (Position[2] * m21) + (1.0f * m31);
    Result.z = (Position[0] * m02) + (Position[1] * m12) + (Position[2] * m22) + (1.0f * m32);
    return Result;

#else

    NSIMD::Float128 NewPosition = NSIMD::Load(Position.x, Position.y, Position.z, 1.0f);
    NewPosition = NSIMD::Transform(this, NewPosition);
    return CVector3(NSIMD::GetX(NewPosition), NSIMD::GetY(NewPosition), NSIMD::GetZ(NewPosition));

#endif
}

FORCEINLINE CVector3 CMatrix4::TransformDirection(const CVector3& Direction) noexcept
{
#if defined(DISABLE_SIMD)

    CVector3 Result;
    Result.x = (Direction[0] * m00) + (Direction[1] * m10) + (Direction[2] * m20);
    Result.y = (Direction[0] * m01) + (Direction[1] * m11) + (Direction[2] * m21);
    Result.z = (Direction[0] * m02) + (Direction[1] * m12) + (Direction[2] * m22);
    return Result;

#else

    NSIMD::Float128 NewDirection = NSIMD::Load(Direction.x, Direction.y, Direction.z, 0.0f);
    NewDirection = NSIMD::Transform(this, NewDirection);
    return CVector3(NSIMD::GetX(NewDirection), NSIMD::GetY(NewDirection), NSIMD::GetZ(NewDirection));

#endif
}

FORCEINLINE CMatrix4 CMatrix4::Transpose() const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix4 Transpose;
    Transpose.f[0][0] = f[0][0];
    Transpose.f[0][1] = f[1][0];
    Transpose.f[0][2] = f[2][0];
    Transpose.f[0][3] = f[3][0];

    Transpose.f[1][0] = f[0][1];
    Transpose.f[1][1] = f[1][1];
    Transpose.f[1][2] = f[2][1];
    Transpose.f[1][3] = f[3][1];

    Transpose.f[2][0] = f[0][2];
    Transpose.f[2][1] = f[1][2];
    Transpose.f[2][2] = f[2][2];
    Transpose.f[2][3] = f[3][2];

    Transpose.f[3][0] = f[0][3];
    Transpose.f[3][1] = f[1][3];
    Transpose.f[3][2] = f[2][3];
    Transpose.f[3][3] = f[3][3];
    return Transpose;

#else

    CMatrix4 Transpose;
    NSIMD::Transpose(this, &Transpose);
    return Transpose;

#endif
}

inline CMatrix4 CMatrix4::Invert() const noexcept
{
#if defined(DISABLE_SIMD)

    float a = (m22 * m33) - (m23 * m32);
    float b = (m21 * m33) - (m23 * m31);
    float c = (m21 * m32) - (m22 * m31);
    float d = (m20 * m33) - (m23 * m30);
    float e = (m20 * m32) - (m22 * m30);
    float f = (m20 * m31) - (m21 * m30);

    CMatrix4 Inverse;
    //d11
    Inverse.m00 = (m11 * a) - (m12 * b) + (m13 * c);
    //d12
    Inverse.m10 = -((m10 * a) - (m12 * d) + (m13 * e));
    //d13
    Inverse.m20 = (m10 * b) - (m11 * d) + (m13 * f);
    //d14
    Inverse.m30 = -((m10 * c) - (m11 * e) + (m12 * f));

    //d21
    Inverse.m01 = -((m01 * a) - (m02 * b) + (m03 * c));
    //d22
    Inverse.m11 = (m00 * a) - (m02 * d) + (m03 * e);
    //d23
    Inverse.m21 = -((m00 * b) - (m01 * d) + (m03 * f));
    //d24
    Inverse.m31 = (m00 * c) - (m01 * e) + (m02 * f);

    const float Determinant = (Inverse.m00 * m00) - (Inverse.m10 * m01) + (Inverse.m20 * m02) - (Inverse.m30 * m03);
    const float ReprDeterminant = 1.0f / Determinant;

    Inverse.m00 *= ReprDeterminant;
    Inverse.m10 *= ReprDeterminant;
    Inverse.m20 *= ReprDeterminant;
    Inverse.m30 *= ReprDeterminant;
    Inverse.m01 *= ReprDeterminant;
    Inverse.m11 *= ReprDeterminant;
    Inverse.m21 *= ReprDeterminant;
    Inverse.m31 *= ReprDeterminant;

    a = (m12 * m33) - (m13 * m32);
    b = (m11 * m33) - (m13 * m31);
    c = (m11 * m32) - (m12 * m31);
    d = (m10 * m33) - (m13 * m30);
    e = (m10 * m32) - (m12 * m30);
    f = (m10 * m31) - (m11 * m30);

    //d31
    Inverse.m02 = ((m01 * a) - (m02 * b) + (m03 * c)) * ReprDeterminant;
    //d32
    Inverse.m12 = -((m00 * a) - (m02 * d) + (m03 * e)) * ReprDeterminant;
    //d33
    Inverse.m22 = ((m00 * b) - (m01 * d) + (m03 * f)) * ReprDeterminant;
    //d34
    Inverse.m32 = -((m00 * c) - (m01 * e) + (m02 * f)) * ReprDeterminant;

    a = (m12 * m23) - (m13 * m22);
    b = (m11 * m23) - (m13 * m21);
    c = (m11 * m22) - (m12 * m21);
    d = (m10 * m23) - (m13 * m20);
    e = (m10 * m22) - (m12 * m20);
    f = (m10 * m21) - (m11 * m20);

    //d41
    Inverse.m03 = -((m01 * a) - (m02 * b) + (m03 * c)) * ReprDeterminant;
    //d42
    Inverse.m13 = ((m00 * a) - (m02 * d) + (m03 * e)) * ReprDeterminant;
    //d43
    Inverse.m23 = -((m00 * b) - (m01 * d) + (m03 * f)) * ReprDeterminant;
    //d44
    Inverse.m33 = ((m00 * c) - (m01 * e) + (m02 * f)) * ReprDeterminant;

    return Inverse;

#else

    NSIMD::Float128 Temp0 = NSIMD::LoadAligned(f[0]);
    NSIMD::Float128 Temp1 = NSIMD::LoadAligned(f[1]);
    NSIMD::Float128 Temp2 = NSIMD::LoadAligned(f[2]);
    NSIMD::Float128 Temp3 = NSIMD::LoadAligned(f[3]);

    NSIMD::Float128 _0 = NSIMD::Shuffle0011<0, 1, 0, 1>(Temp0, Temp1);
    NSIMD::Float128 _1 = NSIMD::Shuffle0011<2, 3, 2, 3>(Temp0, Temp1);
    NSIMD::Float128 _2 = NSIMD::Shuffle0011<0, 1, 0, 1>(Temp2, Temp3);
    NSIMD::Float128 _3 = NSIMD::Shuffle0011<2, 3, 2, 3>(Temp2, Temp3);
    NSIMD::Float128 _4 = NSIMD::Shuffle0011<0, 2, 0, 2>(Temp0, Temp2);
    NSIMD::Float128 _5 = NSIMD::Shuffle0011<1, 3, 1, 3>(Temp1, Temp3);
    NSIMD::Float128 _6 = NSIMD::Shuffle0011<1, 3, 1, 3>(Temp0, Temp2);
    NSIMD::Float128 _7 = NSIMD::Shuffle0011<0, 2, 0, 2>(Temp1, Temp3);

    NSIMD::Float128 Mul0 = NSIMD::Mul(_4, _5);
    NSIMD::Float128 Mul1 = NSIMD::Mul(_6, _7);
    NSIMD::Float128 DetSub = NSIMD::Sub(Mul0, Mul1);

    NSIMD::Float128 DetA = NSIMD::Broadcast<0>(DetSub);
    NSIMD::Float128 DetB = NSIMD::Broadcast<1>(DetSub);
    NSIMD::Float128 DetC = NSIMD::Broadcast<2>(DetSub);
    NSIMD::Float128 DetD = NSIMD::Broadcast<3>(DetSub);

    NSIMD::Float128 dc = NSIMD::Mat2AdjointMul(_3, _2);
    NSIMD::Float128 ab = NSIMD::Mat2AdjointMul(_0, _1);

    NSIMD::Float128 x = NSIMD::Sub(NSIMD::Mul(DetD, _0), NSIMD::Mat2Mul(_1, dc));
    NSIMD::Float128 w = NSIMD::Sub(NSIMD::Mul(DetA, _3), NSIMD::Mat2Mul(_2, ab));

    NSIMD::Float128 DetM = NSIMD::Mul(DetA, DetD);

    NSIMD::Float128 y = NSIMD::Sub(NSIMD::Mul(DetB, _2), NSIMD::Mat2MulAdjoint(_3, ab));
    NSIMD::Float128 z = NSIMD::Sub(NSIMD::Mul(DetC, _1), NSIMD::Mat2MulAdjoint(_0, dc));

    DetM = NSIMD::Add(DetM, NSIMD::Mul(DetB, DetC));

    NSIMD::Float128 Trace = NSIMD::Mul(ab, NSIMD::Shuffle<0, 2, 1, 3>(dc));
    Trace = NSIMD::HorizontalAdd(Trace);
    Trace = NSIMD::HorizontalAdd(Trace);

    DetM = NSIMD::Sub(DetM, Trace);

    const NSIMD::Float128 AdjSignMask = NSIMD::Load(1.0f, -1.0f, -1.0f, 1.0f);
    DetM = NSIMD::Div(AdjSignMask, DetM);

    x = NSIMD::Mul(x, DetM);
    y = NSIMD::Mul(y, DetM);
    z = NSIMD::Mul(z, DetM);
    w = NSIMD::Mul(w, DetM);

    Temp0 = NSIMD::Shuffle0011<3, 1, 3, 1>(x, y);
    Temp1 = NSIMD::Shuffle0011<2, 0, 2, 0>(x, y);
    Temp2 = NSIMD::Shuffle0011<3, 1, 3, 1>(z, w);
    Temp3 = NSIMD::Shuffle0011<2, 0, 2, 0>(z, w);

    CMatrix4 Inverse;
    NSIMD::StoreAligned(Temp0, Inverse.f[0]);
    NSIMD::StoreAligned(Temp1, Inverse.f[1]);
    NSIMD::StoreAligned(Temp2, Inverse.f[2]);
    NSIMD::StoreAligned(Temp3, Inverse.f[3]);
    return Inverse;

#endif
}

inline CMatrix4 CMatrix4::Adjoint() const noexcept
{
#if defined(DISABLE_SIMD)

    float a = (m22 * m33) - (m23 * m32);
    float b = (m21 * m33) - (m23 * m31);
    float c = (m21 * m32) - (m22 * m31);
    float d = (m20 * m33) - (m23 * m30);
    float e = (m20 * m32) - (m22 * m30);
    float f = (m20 * m31) - (m21 * m30);

    CMatrix4 Adjugate;
    //d11
    Adjugate.m00 = (m11 * a) - (m12 * b) + (m13 * c);
    //d12
    Adjugate.m10 = -((m10 * a) - (m12 * d) + (m13 * e));
    //d13
    Adjugate.m20 = (m10 * b) - (m11 * d) + (m13 * f);
    //d14
    Adjugate.m30 = -((m10 * c) - (m11 * e) + (m12 * f));

    //d21
    Adjugate.m01 = -((m01 * a) - (m02 * b) + (m03 * c));
    //d22
    Adjugate.m11 = (m00 * a) - (m02 * d) + (m03 * e);
    //d23
    Adjugate.m21 = -((m00 * b) - (m01 * d) + (m03 * f));
    //d24
    Adjugate.m31 = (m00 * c) - (m01 * e) + (m02 * f);

    a = (m12 * m33) - (m13 * m32);
    b = (m11 * m33) - (m13 * m31);
    c = (m11 * m32) - (m12 * m31);
    d = (m10 * m33) - (m13 * m30);
    e = (m10 * m32) - (m12 * m30);
    f = (m10 * m31) - (m11 * m30);

    //d31
    Adjugate.m02 = (m01 * a) - (m02 * b) + (m03 * c);
    //d32
    Adjugate.m12 = -((m00 * a) - (m02 * d) + (m03 * e));
    //d33
    Adjugate.m22 = (m00 * b) - (m01 * d) + (m03 * f);
    //d34
    Adjugate.m32 = -((m00 * c) - (m01 * e) + (m02 * f));

    a = (m12 * m23) - (m13 * m22);
    b = (m11 * m23) - (m13 * m21);
    c = (m11 * m22) - (m12 * m21);
    d = (m10 * m23) - (m13 * m20);
    e = (m10 * m22) - (m12 * m20);
    f = (m10 * m21) - (m11 * m20);

    //d41
    Adjugate.m03 = -((m01 * a) - (m02 * b) + (m03 * c));
    //d42
    Adjugate.m13 = (m00 * a) - (m02 * d) + (m03 * e);
    //d43
    Adjugate.m23 = -((m00 * b) - (m01 * d) + (m03 * f));
    //d44
    Adjugate.m33 = (m00 * c) - (m01 * e) + (m02 * f);

    return Adjugate;

#else

    NSIMD::Float128 Temp0 = NSIMD::LoadAligned(f[0]);
    NSIMD::Float128 Temp1 = NSIMD::LoadAligned(f[1]);
    NSIMD::Float128 Temp2 = NSIMD::LoadAligned(f[2]);
    NSIMD::Float128 Temp3 = NSIMD::LoadAligned(f[3]);

    NSIMD::Float128 _0 = NSIMD::Shuffle0011<0, 1, 0, 1>(Temp0, Temp1);
    NSIMD::Float128 _1 = NSIMD::Shuffle0011<2, 3, 2, 3>(Temp0, Temp1);
    NSIMD::Float128 _2 = NSIMD::Shuffle0011<0, 1, 0, 1>(Temp2, Temp3);
    NSIMD::Float128 _3 = NSIMD::Shuffle0011<2, 3, 2, 3>(Temp2, Temp3);
    NSIMD::Float128 _4 = NSIMD::Shuffle0011<0, 2, 0, 2>(Temp0, Temp2);
    NSIMD::Float128 _5 = NSIMD::Shuffle0011<1, 3, 1, 3>(Temp1, Temp3);
    NSIMD::Float128 _6 = NSIMD::Shuffle0011<1, 3, 1, 3>(Temp0, Temp2);
    NSIMD::Float128 _7 = NSIMD::Shuffle0011<0, 2, 0, 2>(Temp1, Temp3);

    NSIMD::Float128 Mul0 = NSIMD::Mul(_4, _5);
    NSIMD::Float128 Mul1 = NSIMD::Mul(_6, _7);
    NSIMD::Float128 DetSub = NSIMD::Sub(Mul0, Mul1);

    NSIMD::Float128 DetA = NSIMD::Broadcast<0>(DetSub);
    NSIMD::Float128 DetB = NSIMD::Broadcast<1>(DetSub);
    NSIMD::Float128 DetC = NSIMD::Broadcast<2>(DetSub);
    NSIMD::Float128 DetD = NSIMD::Broadcast<3>(DetSub);

    NSIMD::Float128 dc = NSIMD::Mat2AdjointMul(_3, _2);
    NSIMD::Float128 ab = NSIMD::Mat2AdjointMul(_0, _1);

    NSIMD::Float128 x = NSIMD::Sub(NSIMD::Mul(DetD, _0), NSIMD::Mat2Mul(_1, dc));
    NSIMD::Float128 w = NSIMD::Sub(NSIMD::Mul(DetA, _3), NSIMD::Mat2Mul(_2, ab));

    NSIMD::Float128 y = NSIMD::Sub(NSIMD::Mul(DetB, _2), NSIMD::Mat2MulAdjoint(_3, ab));
    NSIMD::Float128 z = NSIMD::Sub(NSIMD::Mul(DetC, _1), NSIMD::Mat2MulAdjoint(_0, dc));

    const NSIMD::Float128 Mask = NSIMD::Load(1.0f, -1.0f, -1.0f, 1.0f);
    x = NSIMD::Mul(x, Mask);
    y = NSIMD::Mul(y, Mask);
    z = NSIMD::Mul(z, Mask);
    w = NSIMD::Mul(w, Mask);

    Temp0 = NSIMD::Shuffle0011<3, 1, 3, 1>(x, y);
    Temp1 = NSIMD::Shuffle0011<2, 0, 2, 0>(x, y);
    Temp2 = NSIMD::Shuffle0011<3, 1, 3, 1>(z, w);
    Temp3 = NSIMD::Shuffle0011<2, 0, 2, 0>(z, w);

    CMatrix4 Inverse;
    NSIMD::StoreAligned(Temp0, Inverse.f[0]);
    NSIMD::StoreAligned(Temp1, Inverse.f[1]);
    NSIMD::StoreAligned(Temp2, Inverse.f[2]);
    NSIMD::StoreAligned(Temp3, Inverse.f[3]);
    return Inverse;

#endif
}

inline float CMatrix4::Determinant() const noexcept
{
#if defined(DISABLE_SIMD)

    float a = (m22 * m33) - (m23 * m32);
    float b = (m21 * m33) - (m23 * m31);
    float c = (m21 * m32) - (m22 * m31);
    float d = (m20 * m33) - (m23 * m30);
    float e = (m20 * m32) - (m22 * m30);
    float f = (m20 * m31) - (m21 * m30);

    //d11
    float Determinant = m00 * ((m11 * a) - (m12 * b) + (m13 * c));
    //d12
    Determinant -= m01 * ((m10 * a) - (m12 * d) + (m13 * e));
    //d13
    Determinant += m02 * ((m10 * b) - (m11 * d) + (m13 * f));
    //d14
    Determinant -= m03 * ((m10 * c) - (m11 * e) + (m12 * f));
    return Determinant;

#else

    NSIMD::Float128 Temp0 = NSIMD::LoadAligned(f[0]);
    NSIMD::Float128 Temp1 = NSIMD::LoadAligned(f[1]);
    NSIMD::Float128 Temp2 = NSIMD::LoadAligned(f[2]);
    NSIMD::Float128 Temp3 = NSIMD::LoadAligned(f[3]);

    NSIMD::Float128 _0 = NSIMD::Shuffle0011<0, 1, 0, 1>(Temp0, Temp1);
    NSIMD::Float128 _1 = NSIMD::Shuffle0011<2, 3, 2, 3>(Temp0, Temp1);
    NSIMD::Float128 _2 = NSIMD::Shuffle0011<0, 1, 0, 1>(Temp2, Temp3);
    NSIMD::Float128 _3 = NSIMD::Shuffle0011<2, 3, 2, 3>(Temp2, Temp3);
    NSIMD::Float128 _4 = NSIMD::Shuffle0011<0, 2, 0, 2>(Temp0, Temp2);
    NSIMD::Float128 _5 = NSIMD::Shuffle0011<1, 3, 1, 3>(Temp1, Temp3);
    NSIMD::Float128 _6 = NSIMD::Shuffle0011<1, 3, 1, 3>(Temp0, Temp2);
    NSIMD::Float128 _7 = NSIMD::Shuffle0011<0, 2, 0, 2>(Temp1, Temp3);

    NSIMD::Float128 Mul0 = NSIMD::Mul(_4, _5);
    NSIMD::Float128 Mul1 = NSIMD::Mul(_6, _7);
    NSIMD::Float128 DetSub = NSIMD::Sub(Mul0, Mul1);

    NSIMD::Float128 DetA = NSIMD::Broadcast<0>(DetSub);
    NSIMD::Float128 DetB = NSIMD::Broadcast<1>(DetSub);
    NSIMD::Float128 DetC = NSIMD::Broadcast<2>(DetSub);
    NSIMD::Float128 DetD = NSIMD::Broadcast<3>(DetSub);

    NSIMD::Float128 dc = NSIMD::Mat2AdjointMul(_3, _2);
    NSIMD::Float128 ab = NSIMD::Mat2AdjointMul(_0, _1);

    NSIMD::Float128 DetM = NSIMD::Mul(DetA, DetD);
    Mul0 = NSIMD::Mul(DetB, DetC);
    DetM = NSIMD::Add(DetM, Mul0);
    Mul0 = NSIMD::Mul(ab, NSIMD::Shuffle<0, 2, 1, 3>(dc));

    NSIMD::Float128 Sum = NSIMD::HorizontalSum(Mul0);
    DetM = NSIMD::Sub(DetM, Sum);
    return NSIMD::GetX(DetM);

#endif
}

inline bool CMatrix4::HasNan() const noexcept
{
    for (int i = 0; i < 16; i++)
    {
        if (NMath::IsNan(reinterpret_cast<const float*>(this)[i]))
        {
            return true;
        }
    }

    return false;
}

inline bool CMatrix4::HasInfinity() const noexcept
{
    for (int i = 0; i < 16; i++)
    {
        if (NMath::IsInf(reinterpret_cast<const float*>(this)[i]))
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CMatrix4::IsValid() const noexcept
{
    return !HasNan() && !HasInfinity();
}

inline bool CMatrix4::IsEqual(const CMatrix4& Other, float Epsilon) const noexcept
{
#if defined(DISABLE_SIMD)

    Epsilon = NMath::Abs(Epsilon);

    for (int i = 0; i < 16; i++)
    {
        float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
        if (NMath::Abs(Diff) > Epsilon)
        {
            return false;
        }
    }

    return true;

#else

    NSIMD::Float128 Espilon128 = NSIMD::Load(Epsilon);
    Espilon128 = NSIMD::Abs(Espilon128);

    for (int i = 0; i < 4; i++)
    {
        NSIMD::Float128 Diff = NSIMD::Sub(f[i], Other.f[i]);
        Diff = NSIMD::Abs(Diff);

        if (NSIMD::GreaterThan(Diff, Espilon128))
        {
            return false;
        }
    }

    return true;

#endif
}

FORCEINLINE void CMatrix4::SetIdentity() noexcept
{
#if defined(DISABLE_SIMD)

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

    NSIMD::StoreAligned(NSIMD::Load(1.0f, 0.0f, 0.0f, 0.0f), f[0]);
    NSIMD::StoreAligned(NSIMD::Load(0.0f, 1.0f, 0.0f, 0.0f), f[1]);
    NSIMD::StoreAligned(NSIMD::Load(0.0f, 0.0f, 1.0f, 0.0f), f[2]);
    NSIMD::StoreAligned(NSIMD::Load(0.0f, 0.0f, 0.0f, 1.0f), f[3]);

#endif
}

FORCEINLINE void CMatrix4::SetRotationAndScale(const CMatrix3& RotationAndScale) noexcept
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

FORCEINLINE void CMatrix4::SetTranslation(const CVector3& Translation) noexcept
{
    m30 = Translation.x;
    m31 = Translation.y;
    m32 = Translation.z;
}

FORCEINLINE CVector4 CMatrix4::GetRow(int Row) const noexcept
{
    Assert(Row < 4);
    return CVector4(f[Row]);
}

FORCEINLINE CVector4 CMatrix4::GetColumn(int Column) const noexcept
{
    Assert(Column < 4);
    return CVector4(f[0][Column], f[1][Column], f[2][Column], f[3][Column]);
}

FORCEINLINE CVector3 CMatrix4::GetTranslation() const noexcept
{
    return CVector3(m30, m31, m32);
}

FORCEINLINE CMatrix3 CMatrix4::GetRotationAndScale() const noexcept
{
    return CMatrix3(m00, m01, m02, m10, m11, m12, m20, m21, m22);
}

FORCEINLINE float* CMatrix4::GetData() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CMatrix4::GetData() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE bool CMatrix4::operator==(const CMatrix4& Other) const noexcept
{
    return IsEqual(Other);
}

FORCEINLINE bool CMatrix4::operator!=(const CMatrix4& Other) const noexcept
{
    return !IsEqual(Other);
}

FORCEINLINE CVector4 CMatrix4::operator*(const CVector4& Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CVector4 Result;
    Result.x = (Rhs[0] * m00) + (Rhs[1] * m10) + (Rhs[2] * m20) + (Rhs[3] * m30);
    Result.y = (Rhs[0] * m01) + (Rhs[1] * m11) + (Rhs[2] * m21) + (Rhs[3] * m31);
    Result.z = (Rhs[0] * m02) + (Rhs[1] * m12) + (Rhs[2] * m22) + (Rhs[3] * m32);
    Result.w = (Rhs[0] * m03) + (Rhs[1] * m13) + (Rhs[2] * m23) + (Rhs[3] * m33);
    return Result;

#else

    NSIMD::Float128 Temp = NSIMD::LoadAligned(&Rhs);
    Temp = NSIMD::Transform(this, Temp);

    CVector4 Result;
    NSIMD::StoreAligned(Temp, &Result);
    return Result;

#endif
}

NOINLINE CMatrix4 CMatrix4::operator*(const CMatrix4& Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix4 Result;
    Result.m00 = (m00 * Rhs.m00) + (m01 * Rhs.m10) + (m02 * Rhs.m20) + (m03 * Rhs.m30);
    Result.m01 = (m00 * Rhs.m01) + (m01 * Rhs.m11) + (m02 * Rhs.m21) + (m03 * Rhs.m31);
    Result.m02 = (m00 * Rhs.m02) + (m01 * Rhs.m12) + (m02 * Rhs.m22) + (m03 * Rhs.m32);
    Result.m03 = (m00 * Rhs.m03) + (m01 * Rhs.m13) + (m02 * Rhs.m23) + (m03 * Rhs.m33);

    Result.m10 = (m10 * Rhs.m00) + (m11 * Rhs.m10) + (m12 * Rhs.m20) + (m13 * Rhs.m30);
    Result.m11 = (m10 * Rhs.m01) + (m11 * Rhs.m11) + (m12 * Rhs.m21) + (m13 * Rhs.m31);
    Result.m12 = (m10 * Rhs.m02) + (m11 * Rhs.m12) + (m12 * Rhs.m22) + (m13 * Rhs.m32);
    Result.m13 = (m10 * Rhs.m03) + (m11 * Rhs.m13) + (m12 * Rhs.m23) + (m13 * Rhs.m33);

    Result.m20 = (m20 * Rhs.m00) + (m21 * Rhs.m10) + (m22 * Rhs.m20) + (m23 * Rhs.m30);
    Result.m21 = (m20 * Rhs.m01) + (m21 * Rhs.m11) + (m22 * Rhs.m21) + (m23 * Rhs.m31);
    Result.m22 = (m20 * Rhs.m02) + (m21 * Rhs.m12) + (m22 * Rhs.m22) + (m23 * Rhs.m32);
    Result.m23 = (m20 * Rhs.m03) + (m21 * Rhs.m13) + (m22 * Rhs.m23) + (m23 * Rhs.m33);

    Result.m30 = (m30 * Rhs.m00) + (m31 * Rhs.m10) + (m32 * Rhs.m20) + (m33 * Rhs.m30);
    Result.m31 = (m30 * Rhs.m01) + (m31 * Rhs.m11) + (m32 * Rhs.m21) + (m33 * Rhs.m31);
    Result.m32 = (m30 * Rhs.m02) + (m31 * Rhs.m12) + (m32 * Rhs.m22) + (m33 * Rhs.m32);
    Result.m33 = (m30 * Rhs.m03) + (m31 * Rhs.m13) + (m32 * Rhs.m23) + (m33 * Rhs.m33);
    return Result;

#else

    NSIMD::Float128 Row0 = NSIMD::LoadAligned(f[0]);
    Row0 = NSIMD::Transform(&Rhs, Row0);

    NSIMD::Float128 Row1 = NSIMD::LoadAligned(f[1]);
    Row1 = NSIMD::Transform(&Rhs, Row1);

    NSIMD::Float128 Row2 = NSIMD::LoadAligned(f[2]);
    Row2 = NSIMD::Transform(&Rhs, Row2);

    NSIMD::Float128 Row3 = NSIMD::LoadAligned(f[3]);
    Row3 = NSIMD::Transform(&Rhs, Row3);

    CMatrix4 Result;
    NSIMD::StoreAligned(Row0, Result.f[0]);
    NSIMD::StoreAligned(Row1, Result.f[1]);
    NSIMD::StoreAligned(Row2, Result.f[2]);
    NSIMD::StoreAligned(Row3, Result.f[3]);
    return Result;

#endif
}

NOINLINE CMatrix4& CMatrix4::operator*=(const CMatrix4& Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CMatrix4 CMatrix4::operator*(float Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix4 Result;
    Result.m00 = m00 * Rhs;
    Result.m01 = m01 * Rhs;
    Result.m02 = m02 * Rhs;
    Result.m03 = m03 * Rhs;

    Result.m10 = m10 * Rhs;
    Result.m11 = m11 * Rhs;
    Result.m12 = m12 * Rhs;
    Result.m13 = m13 * Rhs;

    Result.m20 = m20 * Rhs;
    Result.m21 = m21 * Rhs;
    Result.m22 = m22 * Rhs;
    Result.m23 = m23 * Rhs;

    Result.m30 = m30 * Rhs;
    Result.m31 = m31 * Rhs;
    Result.m32 = m32 * Rhs;
    Result.m33 = m33 * Rhs;
    return Result;

#else

    NSIMD::Float128 Scalars = NSIMD::Load(Rhs);
    NSIMD::Float128 Row0 = NSIMD::Mul(f[0], Scalars);
    NSIMD::Float128 Row1 = NSIMD::Mul(f[1], Scalars);
    NSIMD::Float128 Row2 = NSIMD::Mul(f[2], Scalars);
    NSIMD::Float128 Row3 = NSIMD::Mul(f[3], Scalars);

    CMatrix4 Result;
    NSIMD::StoreAligned(Row0, Result.f[0]);
    NSIMD::StoreAligned(Row1, Result.f[1]);
    NSIMD::StoreAligned(Row2, Result.f[2]);
    NSIMD::StoreAligned(Row3, Result.f[3]);
    return Result;

#endif
}

FORCEINLINE CMatrix4& CMatrix4::operator*=(float Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CMatrix4 CMatrix4::operator+(const CMatrix4& Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix4 Result;
    Result.m00 = m00 + Rhs.m00;
    Result.m01 = m01 + Rhs.m01;
    Result.m02 = m02 + Rhs.m02;
    Result.m03 = m03 + Rhs.m03;

    Result.m10 = m10 + Rhs.m10;
    Result.m11 = m11 + Rhs.m11;
    Result.m12 = m12 + Rhs.m12;
    Result.m13 = m13 + Rhs.m13;

    Result.m20 = m20 + Rhs.m20;
    Result.m21 = m21 + Rhs.m21;
    Result.m22 = m22 + Rhs.m22;
    Result.m23 = m23 + Rhs.m23;

    Result.m30 = m30 + Rhs.m30;
    Result.m31 = m31 + Rhs.m31;
    Result.m32 = m32 + Rhs.m32;
    Result.m33 = m33 + Rhs.m33;
    return Result;

#else

    NSIMD::Float128 Row0 = NSIMD::Add(f[0], Rhs.f[0]);
    NSIMD::Float128 Row1 = NSIMD::Add(f[1], Rhs.f[1]);
    NSIMD::Float128 Row2 = NSIMD::Add(f[2], Rhs.f[2]);
    NSIMD::Float128 Row3 = NSIMD::Add(f[3], Rhs.f[3]);

    CMatrix4 Result;
    NSIMD::StoreAligned(Row0, Result.f[0]);
    NSIMD::StoreAligned(Row1, Result.f[1]);
    NSIMD::StoreAligned(Row2, Result.f[2]);
    NSIMD::StoreAligned(Row3, Result.f[3]);
    return Result;

#endif
}

FORCEINLINE CMatrix4& CMatrix4::operator+=(const CMatrix4& Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CMatrix4 CMatrix4::operator+(float Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix4 Result;
    Result.m00 = m00 + Rhs;
    Result.m01 = m01 + Rhs;
    Result.m02 = m02 + Rhs;
    Result.m03 = m03 + Rhs;

    Result.m10 = m10 + Rhs;
    Result.m11 = m11 + Rhs;
    Result.m12 = m12 + Rhs;
    Result.m13 = m13 + Rhs;

    Result.m20 = m20 + Rhs;
    Result.m21 = m21 + Rhs;
    Result.m22 = m22 + Rhs;
    Result.m23 = m23 + Rhs;

    Result.m30 = m30 + Rhs;
    Result.m31 = m31 + Rhs;
    Result.m32 = m32 + Rhs;
    Result.m33 = m33 + Rhs;
    return Result;

#else

    NSIMD::Float128 Scalars = NSIMD::Load(Rhs);
    NSIMD::Float128 Row0 = NSIMD::Add(f[0], Scalars);
    NSIMD::Float128 Row1 = NSIMD::Add(f[1], Scalars);
    NSIMD::Float128 Row2 = NSIMD::Add(f[2], Scalars);
    NSIMD::Float128 Row3 = NSIMD::Add(f[3], Scalars);

    CMatrix4 Result;
    NSIMD::StoreAligned(Row0, Result.f[0]);
    NSIMD::StoreAligned(Row1, Result.f[1]);
    NSIMD::StoreAligned(Row2, Result.f[2]);
    NSIMD::StoreAligned(Row3, Result.f[3]);
    return Result;

#endif
}

FORCEINLINE CMatrix4& CMatrix4::operator+=(float Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CMatrix4 CMatrix4::operator-(const CMatrix4& Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix4 Result;
    Result.m00 = m00 - Rhs.m00;
    Result.m01 = m01 - Rhs.m01;
    Result.m02 = m02 - Rhs.m02;
    Result.m03 = m03 - Rhs.m03;

    Result.m10 = m10 - Rhs.m10;
    Result.m11 = m11 - Rhs.m11;
    Result.m12 = m12 - Rhs.m12;
    Result.m13 = m13 - Rhs.m13;

    Result.m20 = m20 - Rhs.m20;
    Result.m21 = m21 - Rhs.m21;
    Result.m22 = m22 - Rhs.m22;
    Result.m23 = m23 - Rhs.m23;

    Result.m30 = m30 - Rhs.m30;
    Result.m31 = m31 - Rhs.m31;
    Result.m32 = m32 - Rhs.m32;
    Result.m33 = m33 - Rhs.m33;
    return Result;

#else

    NSIMD::Float128 Row0 = NSIMD::Sub(f[0], Rhs.f[0]);
    NSIMD::Float128 Row1 = NSIMD::Sub(f[1], Rhs.f[1]);
    NSIMD::Float128 Row2 = NSIMD::Sub(f[2], Rhs.f[2]);
    NSIMD::Float128 Row3 = NSIMD::Sub(f[3], Rhs.f[3]);

    CMatrix4 Result;
    NSIMD::StoreAligned(Row0, Result.f[0]);
    NSIMD::StoreAligned(Row1, Result.f[1]);
    NSIMD::StoreAligned(Row2, Result.f[2]);
    NSIMD::StoreAligned(Row3, Result.f[3]);
    return Result;

#endif
}

FORCEINLINE CMatrix4& CMatrix4::operator-=(const CMatrix4& Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CMatrix4 CMatrix4::operator-(float Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix4 Result;
    Result.m00 = m00 - Rhs;
    Result.m01 = m01 - Rhs;
    Result.m02 = m02 - Rhs;
    Result.m03 = m03 - Rhs;

    Result.m10 = m10 - Rhs;
    Result.m11 = m11 - Rhs;
    Result.m12 = m12 - Rhs;
    Result.m13 = m13 - Rhs;

    Result.m20 = m20 - Rhs;
    Result.m21 = m21 - Rhs;
    Result.m22 = m22 - Rhs;
    Result.m23 = m23 - Rhs;

    Result.m30 = m30 - Rhs;
    Result.m31 = m31 - Rhs;
    Result.m32 = m32 - Rhs;
    Result.m33 = m33 - Rhs;
    return Result;

#else

    NSIMD::Float128 Scalars = NSIMD::Load(Rhs);
    NSIMD::Float128 Row0 = NSIMD::Sub(f[0], Scalars);
    NSIMD::Float128 Row1 = NSIMD::Sub(f[1], Scalars);
    NSIMD::Float128 Row2 = NSIMD::Sub(f[2], Scalars);
    NSIMD::Float128 Row3 = NSIMD::Sub(f[3], Scalars);

    CMatrix4 Result;
    NSIMD::StoreAligned(Row0, Result.f[0]);
    NSIMD::StoreAligned(Row1, Result.f[1]);
    NSIMD::StoreAligned(Row2, Result.f[2]);
    NSIMD::StoreAligned(Row3, Result.f[3]);
    return Result;

#endif
}

FORCEINLINE CMatrix4& CMatrix4::operator-=(float Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CMatrix4 CMatrix4::operator/(float Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    const float Recip = 1.0f / Rhs;

    CMatrix4 Result;
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
    return Result;

#else

    NSIMD::Float128 RecipScalars = NSIMD::Load(1.0f / Rhs);
    NSIMD::Float128 Row0 = NSIMD::Mul(f[0], RecipScalars);
    NSIMD::Float128 Row1 = NSIMD::Mul(f[1], RecipScalars);
    NSIMD::Float128 Row2 = NSIMD::Mul(f[2], RecipScalars);
    NSIMD::Float128 Row3 = NSIMD::Mul(f[3], RecipScalars);

    CMatrix4 Result;
    NSIMD::StoreAligned(Row0, Result.f[0]);
    NSIMD::StoreAligned(Row1, Result.f[1]);
    NSIMD::StoreAligned(Row2, Result.f[2]);
    NSIMD::StoreAligned(Row3, Result.f[3]);
    return Result;

#endif
}

FORCEINLINE CMatrix4& CMatrix4::operator/=(float Rhs) noexcept
{
    return *this = *this / Rhs;
}

inline CMatrix4 CMatrix4::Identity() noexcept
{
    return CMatrix4(1.0f);
}

inline CMatrix4 CMatrix4::Scale(float Scale) noexcept
{
    return CMatrix4(
        Scale, 0.0f, 0.0f, 0.0f,
        0.0f, Scale, 0.0f, 0.0f,
        0.0f, 0.0f, Scale, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

inline CMatrix4 CMatrix4::Scale(float x, float y, float z) noexcept
{
    return CMatrix4(
        x, 0.0f, 0.0f, 0.0f,
        0.0f, y, 0.0f, 0.0f,
        0.0f, 0.0f, z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

inline CMatrix4 CMatrix4::Scale(const CVector3& VectorWithScale) noexcept
{
    return Scale(VectorWithScale.x, VectorWithScale.y, VectorWithScale.z);
}

inline CMatrix4 CMatrix4::Translation(float x, float y, float z) noexcept
{
    return CMatrix4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, z, 1.0f);
}

inline CMatrix4 CMatrix4::Translation(const CVector3& InTranslation) noexcept
{
    return Translation(InTranslation.x, InTranslation.y, InTranslation.z);
}

inline CMatrix4 CMatrix4::RotationRollPitchYaw(float Pitch, float Yaw, float Roll) noexcept
{
    const float SinP = sinf(Pitch);
    const float SinY = sinf(Yaw);
    const float SinR = sinf(Roll);
    const float CosP = cosf(Pitch);
    const float CosY = cosf(Yaw);
    const float CosR = cosf(Roll);

    const float SinRSinP = SinR * SinP;
    const float CosRSinP = CosR * SinP;

    return CMatrix4(
        (CosR * CosY) + (SinRSinP * SinY), (SinR * CosP), (SinRSinP * CosY) - (CosR * SinY), 0.0f,
        (CosRSinP * SinY) - (SinR * CosY), (CosR * CosP), (SinR * SinY) + (CosRSinP * CosY), 0.0f,
        (CosP * SinY), -SinP, (CosP * CosY), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

inline CMatrix4 CMatrix4::RotationRollPitchYaw(const CVector3& PitchYawRoll) noexcept
{
    return RotationRollPitchYaw(PitchYawRoll.x, PitchYawRoll.y, PitchYawRoll.z);
}

inline CMatrix4 CMatrix4::RotationX(float x) noexcept
{
    const float SinX = sinf(x);
    const float CosX = cosf(x);

    return CMatrix4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, CosX, SinX, 0.0f,
        0.0f, -SinX, CosX, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

inline CMatrix4 CMatrix4::RotationY(float y) noexcept
{
    const float SinY = sinf(y);
    const float CosY = cosf(y);

    return CMatrix4(
        CosY, 0.0f, -SinY, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        SinY, 0.0f, CosY, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

inline CMatrix4 CMatrix4::RotationZ(float z) noexcept
{
    const float SinZ = sinf(z);
    const float CosZ = cosf(z);

    return CMatrix4(
        CosZ, SinZ, 0.0f, 0.0f,
        -SinZ, CosZ, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

inline CMatrix4 CMatrix4::OrtographicProjection(float Width, float Height, float NearZ, float FarZ) noexcept
{
    return CMatrix4(
        2.0f / Width, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / Height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f / (FarZ - NearZ), 0.0f,
        0.0f, 0.0f, -NearZ / (FarZ - NearZ), 1.0f);
}

inline CMatrix4 CMatrix4::OrtographicProjection(float Left, float Right, float Bottom, float Top, float NearZ, float FarZ) noexcept
{
    const float InvWidth = 1.0f / (Right - Left);
    const float InvHeight = 1.0f / (Top - Bottom);
    const float Range = 1.0f / (FarZ - NearZ);

    return CMatrix4(
        InvWidth + InvWidth, 0.0f, 0.0f, 0.0f,
        0.0f, InvHeight + InvHeight, 0.0f, 0.0f,
        0.0f, 0.0f, Range, 0.0f,
        -(Left + Right) * InvWidth, -(Top + Bottom) * InvHeight, -Range * NearZ, 1.0f);
}

inline CMatrix4 CMatrix4::PerspectiveProjection(float Fov, float AspectRatio, float NearZ, float FarZ) noexcept
{
    if (Fov < NMath::ONE_DEGREE_F || Fov >(NMath::PI_F - NMath::ONE_DEGREE_F))
    {
        return CMatrix4();
    }

    const float ScaleY = 1.0f / tanf(Fov * 0.5f);
    const float ScaleX = ScaleY / AspectRatio;
    const float Range = FarZ / (FarZ - NearZ);

    return CMatrix4(
        ScaleX, 0.0f, 0.0f, 0.0f,
        0.0f, ScaleY, 0.0f, 0.0f,
        0.0f, 0.0f, Range, 1.0f,
        0.0f, 0.0f, -Range * NearZ, 0.0f);
}

inline CMatrix4 CMatrix4::PerspectiveProjection(float Fov, float Width, float Height, float NearZ, float FarZ) noexcept
{
    const float AspectRatio = Width / Height;
    return PerspectiveProjection(Fov, AspectRatio, NearZ, FarZ);
}

inline CMatrix4 CMatrix4::LookAt(const CVector3& Eye, const CVector3& At, const CVector3& Up) noexcept
{
    const CVector3 Direction = At - Eye;
    return LookTo(Eye, Direction, Up);
}

inline CMatrix4 CMatrix4::LookTo(const CVector3& Eye, const CVector3& Direction, const CVector3& Up) noexcept
{
    CVector3 e2 = Direction.GetNormalized();
    CVector3 e0 = Up.CrossProduct(e2).GetNormalized();
    CVector3 e1 = e2.CrossProduct(e0).GetNormalized();

    CVector3 NegEye = -Eye;

    const float m30 = NegEye.DotProduct(e0);
    const float m31 = NegEye.DotProduct(e1);
    const float m32 = NegEye.DotProduct(e2);

    CMatrix4 Result(
        CVector4(e0, m30),
        CVector4(e1, m31),
        CVector4(e2, m32),
        CVector4(0.0f, 0.0f, 0.0f, 1.0f));
    return Result.Transpose();
}
