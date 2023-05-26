#pragma once
#include "Vector3.h"
#include "VectorOp.h"

class VECTOR_ALIGN FVector4
{
public:

    /**
     * @brief - Default constructor (Initialize components to zero) 
     */
    FORCEINLINE FVector4() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
        , w(0.0f)
    {
    }

    /**
     * @brief     - Constructor initializing all components with a corresponding value.
     * @param InX - The x-coordinate
     * @param InY - The y-coordinate
     * @param InZ - The z-coordinate
     * @param InW - The w-coordinate
     */
    FORCEINLINE explicit FVector4(float InX, float InY, float InZ, float InW) noexcept
        : x(InX)
        , y(InY)
        , z(InZ)
        , w(InW)
    {
    }

    /**
     * @brief     - Constructor initializing all components with an array.
     * @param Arr - Array with 4 elements
     */
    FORCEINLINE explicit FVector4(const float* Arr) noexcept
        : x(Arr[0])
        , y(Arr[1])
        , z(Arr[2])
        , w(Arr[3])
    {
    }

    /**
     * @brief        - Constructor initializing all components with a single value.
     * @param Scalar - Value to set all components to
     */
    FORCEINLINE explicit FVector4(float Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
        , z(Scalar)
        , w(Scalar)
    {
    }

    /**
     * @brief     - Constructor copying a 3-D vector (x, y, z) into the first components, setting w-component to zero
     * @param XYZ - Value to set first components to
     */
    FORCEINLINE FVector4(const FVector3& XYZ) noexcept
        : x(XYZ.x)
        , y(XYZ.y)
        , z(XYZ.z)
        , w(0.0f)
    {
    }

    /**
     * @brief     - Constructor copying a 3-D vector (x, y, z) into the first components, setting w-component to a specific value
     * @param XYZ - Value to set first components to
     * @param InW - Value to set the w-component to
     */
    FORCEINLINE explicit FVector4(const FVector3& XYZ, float InW) noexcept
        : x(XYZ.x)
        , y(XYZ.y)
        , z(XYZ.z)
        , w(InW)
    {
    }

     /** @brief - Normalize this vector */
    void Normalize() noexcept
    {
#if !USE_VECTOR_OP
        const float fLengthSquared = LengthSquared();
        if (fLengthSquared != 0.0f)
        {
            const float fRecipLength = 1.0f / FMath::Sqrt(fLengthSquared);
            x *= fRecipLength;
            y *= fRecipLength;
            z *= fRecipLength;
            w *= fRecipLength;
        }
#else
        NVectorOp::Float128 Temp0 = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Temp1 = NVectorOp::Dot(Temp0, Temp0);

        const float fLengthSquared = NVectorOp::GetX(Temp1);
        if (fLengthSquared != 0.0f)
        {
            Temp1 = NVectorOp::Shuffle<0, 1, 0, 1>(Temp1);
            Temp1 = NVectorOp::RecipSqrt(Temp1);
            Temp0 = NVectorOp::Mul(Temp0, Temp1);
            NVectorOp::StoreAligned(Temp0, this);
        }
#endif
    }

    /**
     * @brief  - Returns a normalized version of this vector
     * @return - A copy of this vector normalized
     */
    FORCEINLINE FVector4 GetNormalized() const noexcept
    {
        FVector4 Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief       - Compares, within a threshold Epsilon, this vector with another vector
     * @param Other - vector to compare against
     * @return      - True if equal, false if not
     */
    bool IsEqual(const FVector4& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
#if !USE_VECTOR_OP
        Epsilon = FMath::Abs(Epsilon);

        for (int32 Index = 0; Index < 4; ++Index)
        {
            float Diff = reinterpret_cast<const float*>(this)[Index] - reinterpret_cast<const float*>(&Other)[Index];
            if (FMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
#else
        NVectorOp::Float128 Espilon128 = NVectorOp::Load(Epsilon);
        Espilon128 = NVectorOp::Abs(Espilon128);

        NVectorOp::Float128 Diff = NVectorOp::Sub(this, &Other);
        Diff = NVectorOp::Abs(Diff);

        return NVectorOp::LessThan(Diff, Espilon128);
#endif
    }

    /**
     * @brief  - Checks weather this vector is a unit vector not
     * @return - True if the length equals one, false if not
     */
    FORCEINLINE bool IsUnitVector() const noexcept
    {
        const float fLengthSquared = FMath::Abs(1.0f - LengthSquared());
        return (fLengthSquared < FMath::kIsEqualEpsilon);
    }

    /**
     * @brief  - Checks weather this vector has any component that equals NaN
     * @return - True if the any component equals NaN, false if not
     */
    FORCEINLINE bool HasNaN() const noexcept
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (FMath::IsNaN(reinterpret_cast<const float*>(this)[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief  - Checks weather this vector has any component that equals infinity
     * @return - True if the any component equals infinity, false if not
     */
    FORCEINLINE bool HasInfinity() const noexcept
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (FMath::IsInfinity(reinterpret_cast<const float*>(this)[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief  - Checks weather this vector has any value that equals infinity or NaN
     * @return - False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return !HasNaN() && !HasInfinity();
    }

    /**
     * @brief  - Returns the length of this vector
     * @return - The length of the vector
     */
    FORCEINLINE float Length() const noexcept
    {
        const float fLengthSquared = LengthSquared();
        return FMath::Sqrt(fLengthSquared);
    }

    /**
     * @brief  - Returns the length of this vector squared
     * @return - The length of the vector squared
     */
    FORCEINLINE float LengthSquared() const noexcept
    {
        return DotProduct(*this);
    }

    /**
     * @brief       - Returns the dot product between this and another vector
     * @param Other - The vector to perform dot product with
     * @return      - The dot product
     */
    FORCEINLINE float DotProduct(const FVector4& Other) const noexcept
    {
#if !USE_VECTOR_OP
        return (x * Other.x) + (y * Other.y) + (z * Other.z) + (w * Other.w);
#else
        NVectorOp::Float128 Temp0 = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Temp1 = NVectorOp::LoadAligned(&Other);
        NVectorOp::Float128 Dot   = NVectorOp::Dot(Temp0, Temp1);
        return NVectorOp::GetX(Dot);
#endif
    }

    /**
     * @brief - Returns the cross product of this vector and another vector.
     *     This function does not take the w-component into account.
     * 
     * @param Other - The vector to perform cross product with
     * @return      - The cross product
     */
    FVector4 CrossProduct(const FVector4& Other) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4((y * Other.z) - (z * Other.y)
                       ,(z * Other.x) - (x * Other.z)
                       ,(x * Other.y) - (y * Other.x)
                       ,0.0f);
#else
        FVector4 NewVector;

        NVectorOp::Float128 Temp0  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Temp1  = NVectorOp::LoadAligned(&Other);
        NVectorOp::Float128 Result = NVectorOp::Cross(Temp0, Temp1);
        NVectorOp::Int128   Mask   = NVectorOp::Load(~0, ~0, ~0, 0);
        Result = NVectorOp::And(Result, NVectorOp::CastIntToFloat(Mask));

        NVectorOp::StoreAligned(Result, &NewVector);
        return NewVector;
#endif
    }

    /**
     * @brief       - Returns the resulting vector after projecting this vector onto another.
     * @param Other - The vector to project onto
     * @return      - The projected vector
     */
    FVector4 ProjectOn(const FVector4& Other) const noexcept
    {
#if !USE_VECTOR_OP
        float AdotB = DotProduct(Other);
        float BdotB = Other.LengthSquared();
        return (AdotB / BdotB) * Other;
#else
        FVector4 Result;

        NVectorOp::Float128 Temp0 = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Temp1 = NVectorOp::LoadAligned(&Other);

        NVectorOp::Float128 AdotB = NVectorOp::Dot(Temp0, Temp1);
        AdotB = NVectorOp::Shuffle<0, 1, 0, 1>(AdotB);

        NVectorOp::Float128 BdotB = NVectorOp::Dot(Temp1, Temp1);
        BdotB = NVectorOp::Shuffle<0, 1, 0, 1>(BdotB);
        BdotB = NVectorOp::Div(AdotB, BdotB);
        BdotB = NVectorOp::Mul(BdotB, Temp1);

        NVectorOp::StoreAligned(BdotB, &Result);
        return Result;
#endif
    }

    /**
     * @brief        - Returns the reflected vector after reflecting this vector around a normal.
     * @param Normal - Vector to reflect around
     * @return       - The reflected vector
     */
    FVector4 Reflect(const FVector4& Normal) const noexcept
    {
#if !USE_VECTOR_OP
        float VdotN = DotProduct(Normal);
        float NdotN = Normal.LengthSquared();
        return *this - ((2.0f * (VdotN / NdotN)) * Normal);
#else
        FVector4 Result;

        NVectorOp::Float128 Temp0 = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Temp1 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Normal));

        NVectorOp::Float128 VdotN = NVectorOp::Dot(Temp0, Temp1);
        VdotN = NVectorOp::Shuffle<0, 1, 0, 1>(VdotN);

        NVectorOp::Float128 NdotN = NVectorOp::Dot(Temp1, Temp1);
        NdotN = NVectorOp::Shuffle<0, 1, 0, 1>(NdotN);

        NVectorOp::Float128 Reg2 = NVectorOp::Load(2.0f);
        VdotN = NVectorOp::Div(VdotN, NdotN);
        VdotN = NVectorOp::Mul(Reg2, VdotN);
        Temp1 = NVectorOp::Mul(VdotN, Temp1);
        Temp0 = NVectorOp::Sub(Temp0, Temp1);

        NVectorOp::StoreAligned(Temp0, &Result);
        return Result;
#endif
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
     * @brief        - Returns a vector with the smallest of each component of two vectors
     * @param First  - First vector to compare with
     * @param Second - Second vector to compare with
     * @return       - A vector with the smallest components of First and Second
     */
    friend FORCEINLINE FVector4 Min(const FVector4& First, const FVector4& Second) noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(
            FMath::Min(First.x, Second.x),
            FMath::Min(First.y, Second.y),
            FMath::Min(First.z, Second.z),
            FMath::Min(First.w, Second.w));
#else
        FVector4 Result;

        NVectorOp::Float128 Temp0 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&First));
        NVectorOp::Float128 Temp1 = NVectorOp::LoadAligned(&Second);
        Temp0 = NVectorOp::Min(Temp0, Temp1);

        NVectorOp::StoreAligned(Temp0, &Result);
        return Result;
#endif
    }

    /**
     * @brief        - Returns a vector with the largest of each component of two vectors
     * @param First  - First vector to compare with
     * @param Second - Second vector to compare with
     * @return       - A vector with the largest components of First and Second
     */
    friend FORCEINLINE FVector4 Max(const FVector4& First, const FVector4& Second) noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(
            FMath::Max(First.x, Second.x),
            FMath::Max(First.y, Second.y),
            FMath::Max(First.z, Second.z),
            FMath::Max(First.w, Second.w));
#else
        FVector4 Result;

        NVectorOp::Float128 Temp0 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&First));
        NVectorOp::Float128 Temp1 = NVectorOp::LoadAligned(&Second);
        Temp0 = NVectorOp::Max(Temp0, Temp1);

        NVectorOp::StoreAligned(Temp0, &Result);
        return Result;
#endif
    }

    /**
     * @brief        - Returns the linear interpolation between two vectors
     * @param First  - First vector to interpolate
     * @param Second - Second vector to interpolate
     * @param Factor - Factor to interpolate with. Zero returns First, One returns seconds
     * @return       - A vector with the result of interpolation
     */
    friend FORCEINLINE FVector4 Lerp(const FVector4& First, const FVector4& Second, float t) noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(
            (1.0f - t) * First.x + t * Second.x,
            (1.0f - t) * First.y + t * Second.y,
            (1.0f - t) * First.z + t * Second.z,
            (1.0f - t) * First.w + t * Second.w);
#else
        FVector4 Result;

        NVectorOp::Float128 Temp0 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&First));
        NVectorOp::Float128 Temp1 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Second));
        NVectorOp::Float128 Temp3 = NVectorOp::Load(t);
        
        NVectorOp::Float128 Ones  = NVectorOp::MakeOnes();
        
        NVectorOp::Float128 Temp4 = NVectorOp::Sub(Ones, Temp3);
        Temp0 = NVectorOp::Mul(Temp0, Temp4);
        Temp1 = NVectorOp::Mul(Temp1, Temp3);
        Temp0 = NVectorOp::Add(Temp0, Temp1);

        NVectorOp::StoreAligned(Temp0, &Result);
        return Result;
#endif
    }

    /**
     * @brief       - Returns a vector with all the components within the range of a min and max value
     * @param Min   - Vector with minimum values
     * @param Max   - Vector with maximum values
     * @param Value - Vector to clamp
     * @return      - A vector with the result of clamping
     */
    friend FORCEINLINE FVector4 Clamp(const FVector4& Min, const FVector4& Max, const FVector4& Value) noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(
            FMath::Min(FMath::Max(Value.x, Min.x), Max.x),
            FMath::Min(FMath::Max(Value.y, Min.y), Max.y),
            FMath::Min(FMath::Max(Value.z, Min.z), Max.z),
            FMath::Min(FMath::Max(Value.w, Min.w), Max.w));
#else
        FVector4 Result;

        NVectorOp::Float128 Min128   = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Min));
        NVectorOp::Float128 Max128   = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Max));
        NVectorOp::Float128 Value128 = NVectorOp::LoadAligned(&Value);
        Value128 = NVectorOp::Max(Value128, Min128);
        Value128 = NVectorOp::Min(Value128, Max128);

        NVectorOp::StoreAligned(Value128, &Result);
        return Result;
#endif
    }

    /**
     * @brief       - Returns a vector with all the components within the range zero and one
     * @param Value - Value to saturate
     * @return      - A vector with the result of saturation
     */
    friend FORCEINLINE FVector4 Saturate(const FVector4& Value) noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(
            FMath::Min(FMath::Max(Value.x, 0.0f), 1.0f),
            FMath::Min(FMath::Max(Value.y, 0.0f), 1.0f),
            FMath::Min(FMath::Max(Value.z, 0.0f), 1.0f),
            FMath::Min(FMath::Max(Value.w, 0.0f), 1.0f));
#else
        FVector4 Result;

        NVectorOp::Float128 Zeros       = NVectorOp::MakeZeros();
        NVectorOp::Float128 Ones        = NVectorOp::MakeOnes();
        NVectorOp::Float128 VectorValue = NVectorOp::LoadAligned(&Value);
        VectorValue = NVectorOp::Max(VectorValue, Zeros);
        VectorValue = NVectorOp::Min(VectorValue, Ones);

        NVectorOp::StoreAligned(VectorValue, &Result);
        return Result;
#endif
    }

public:

    /**
     * @brief  - Return a vector with component-wise negation of this vector
     * @return - A negated vector
     */
    FORCEINLINE FVector4 operator-() const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(-x, -y, -z, -w);
#else
        FVector4 Result;

        NVectorOp::Float128 Zeros = NVectorOp::MakeZeros();
        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        This = NVectorOp::Sub(Zeros, This);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns the result of component-wise adding this and another vector
     * @param RHS - The vector to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FVector4 operator+(const FVector4& RHS) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(x + RHS.x, y + RHS.y, z + RHS.z, w + RHS.w);
#else
        FVector4 Result;

        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
        This = NVectorOp::Add(This, Other);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns this vector after component-wise adding this with another vector
     * @param RHS - The vector to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator+=(const FVector4& RHS) noexcept
    {
#if !USE_VECTOR_OP
        return *this = *this + RHS;
#else
        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
        This = NVectorOp::Add(This, Other);

        NVectorOp::StoreAligned(This, this);
        return *this;
#endif
    }

    /**
     * @brief     - Returns the result of adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FVector4 operator+(float RHS) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(x + RHS, y + RHS, z + RHS, w + RHS);
#else
        FVector4 Result;

        NVectorOp::Float128 This    = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
        This = NVectorOp::Add(This, Scalars);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns this vector after adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator+=(float RHS) noexcept
    {
#if !USE_VECTOR_OP
        return *this = *this + RHS;
#else
        NVectorOp::Float128 This    = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
        This = NVectorOp::Add(This, Scalars);

        NVectorOp::StoreAligned(This, this);
        return *this;
#endif
    }

    /**
     * @brief     - Returns the result of component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A vector with the result of subtraction
     */
    FORCEINLINE FVector4 operator-(const FVector4& RHS) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(x - RHS.x, y - RHS.y, z - RHS.z, w - RHS.w);
#else
        FVector4 Result;

        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
        This = NVectorOp::Sub(This, Other);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns this vector after component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator-=(const FVector4& RHS) noexcept
    {
#if !USE_VECTOR_OP
        return *this = *this - RHS;
#else
        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
        This = NVectorOp::Sub(This, Other);

        NVectorOp::StoreAligned(This, this);
        return *this;
#endif
    }

    /**
     * @brief     - Returns the result of subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A vector with the result of the subtraction
     */
    FORCEINLINE FVector4 operator-(float RHS) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(x - RHS, y - RHS, z - RHS, w - RHS);
#else
        FVector4 Result;

        NVectorOp::Float128 This    = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
        This = NVectorOp::Sub(This, Scalars);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns this vector after subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator-=(float RHS) noexcept
    {
#if !USE_VECTOR_OP
        return *this = *this - RHS;
#else
        NVectorOp::Float128 This    = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
        This = NVectorOp::Sub(This, Scalars);

        NVectorOp::StoreAligned(This, this);
        return *this;
#endif
    }


    /**
     * @brief     - Returns the result of component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A vector with the result of the multiplication
     */
    FORCEINLINE FVector4 operator*(const FVector4& RHS) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(x * RHS.x, y * RHS.y, z * RHS.z, w * RHS.w);
#else
        FVector4 Result;

        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
        This = NVectorOp::Mul(This, Other);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns this vector after component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator*=(const FVector4& RHS) noexcept
    {
#if !USE_VECTOR_OP
        return *this = *this * RHS;
#else
        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
        This = NVectorOp::Mul(This, Other);

        NVectorOp::StoreAligned(This, this);
        return *this;
#endif
    }

    /**
     * @brief     - Returns the result of multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A vector with the result of the multiplication
     */
    FORCEINLINE FVector4 operator*(float RHS) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(x * RHS, y * RHS, z * RHS, w * RHS);
#else
        FVector4 Result;

        NVectorOp::Float128 This    = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
        This = NVectorOp::Mul(This, Scalars);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns the result of multiplying each component of a vector with a scalar
     * @param LHS - The scalar to multiply with
     * @param RHS - The vector to multiply with
     * @return    - A vector with the result of the multiplication
     */
    friend FORCEINLINE FVector4 operator*(float LHS, const FVector4& RHS) noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(RHS.x * LHS, RHS.y * LHS, RHS.z * LHS, RHS.w * LHS);
#else
        FVector4 Result;

        NVectorOp::Float128 This    = NVectorOp::LoadAligned(&RHS);
        NVectorOp::Float128 Scalars = NVectorOp::Load(LHS);
        This = NVectorOp::Mul(This, Scalars);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns this vector after multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4 operator*=(float RHS) noexcept
    {
#if !USE_VECTOR_OP
        return *this = *this * RHS;
#else
        NVectorOp::Float128 This    = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
        This = NVectorOp::Mul(This, Scalars);

        NVectorOp::StoreAligned(This, this);
        return *this;
#endif
    }

    /**
     * @brief     - Returns the result of component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A vector with the result of the division
     */
    FORCEINLINE FVector4 operator/(const FVector4& RHS) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(x / RHS.x, y / RHS.y, z / RHS.z, w / RHS.w);
#else
        FVector4 Result;

        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
        This = NVectorOp::Div(This, Other);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns this vector after component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator/=(const FVector4& RHS) noexcept
    {
#if !USE_VECTOR_OP
        return *this = *this / RHS;
#else
        NVectorOp::Float128 This  = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
        This = NVectorOp::Div(This, Other);

        NVectorOp::StoreAligned(This, this);
        return *this;
#endif
    }

    /**
     * @brief     - Returns the result of dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A vector with the result of the division
     */
    FORCEINLINE FVector4 operator/(float RHS) const noexcept
    {
#if !USE_VECTOR_OP
        return FVector4(x / RHS, y / RHS, z / RHS, w / RHS);
#else
        FVector4 Result;

        NVectorOp::Float128 This    = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
        This = NVectorOp::Div(This, Scalars);

        NVectorOp::StoreAligned(This, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Returns this vector after dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator/=(float RHS) noexcept
    {
#if !USE_VECTOR_OP
        return *this = *this / RHS;
#else
        NVectorOp::Float128 This    = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
        This = NVectorOp::Div(This, Scalars);

        NVectorOp::StoreAligned(This, this);
        return *this;
#endif
    }

    /**
     * @brief       - Returns the result after comparing this and another vector
     * @param Other - The vector to compare with
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool operator==(const FVector4& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief       - Returns the negated result after comparing this and another vector
     * @param Other - The vector to compare with
     * @return      - False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FVector4& Other) const noexcept
    {
        return !IsEqual(Other);
    }

    /**
     * @brief       - Returns the component specified
     * @param Index - The component index
     * @return      - The component
     */
    FORCEINLINE float& operator[](int32 Index) noexcept
    {
        CHECK(Index < 4);
        return reinterpret_cast<float*>(this)[Index];
    }

    /**
     * @brief       - Returns the component specified
     * @param Index - The component index
     * @return      - The component
     */
    FORCEINLINE float operator[](int32 Index) const noexcept
    {
        CHECK(Index < 4);
        return reinterpret_cast<const float*>(this)[Index];
    }

public:

     /** @brief - The x-coordinate */
    float x;
    
    /** @brief - The y-coordinate */
    float y;
    
    /** @brief - The z-coordinate */
    float z;

     /** @brief - The w-coordinate */
    float w;
};

MARK_AS_REALLOCATABLE(FVector4);

template<>
FORCEINLINE FVector4 FMath::ToDegrees<FVector4>(FVector4 Radians)
{
	return FVector4(ToDegrees(Radians.x), ToDegrees(Radians.y), ToDegrees(Radians.z), ToDegrees(Radians.w));
}

template<>
FORCEINLINE FVector4 FMath::ToRadians<FVector4>(FVector4 Degrees)
{
	return FVector4(ToRadians(Degrees.x), ToRadians(Degrees.y), ToRadians(Degrees.z), ToRadians(Degrees.w));
}
