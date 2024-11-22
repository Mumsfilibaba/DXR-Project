#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/VectorMath/VectorMath.h"

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
    #if !USE_VECTOR_MATH
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
        FFloat128 Temp0 = FVectorMath::LoadAligned(reinterpret_cast<float*>(this));
        FFloat128 Temp1 = FVectorMath::VectorDot(Temp0, Temp0);

        const float fLengthSquared = FVectorMath::VectorGetX(Temp1);
        if (fLengthSquared != 0.0f)
        {
            Temp1 = FVectorMath::VectorShuffle<0, 1, 0, 1>(Temp1);
            Temp1 = FVectorMath::VectorRecipSqrt(Temp1);
            Temp0 = FVectorMath::VectorMul(Temp0, Temp1);
            FVectorMath::StoreAligned(Temp0, reinterpret_cast<float*>(this));
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
    #if !USE_VECTOR_MATH
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
        FFloat128 Espilon128 = FVectorMath::Load(Epsilon);
        Espilon128 = FVectorMath::VectorAbs(Espilon128);
        
        FFloat128 Diff = FVectorMath::VectorSub(reinterpret_cast<const float*>(this), reinterpret_cast<const float*>(&Other));
        Diff = FVectorMath::VectorAbs(Diff);
        
        return FVectorMath::LessThan(Diff, Espilon128);
    #endif
    }

    /**
     * @brief  - Checks weather this vector is a unit vector not
     * @return - True if the length equals one, false if not
     */
    FORCEINLINE bool IsUnitVector() const noexcept
    {
        const float fLengthSquared = FMath::Abs(1.0f - LengthSquared());
        return fLengthSquared < FMath::kIsEqualEpsilon;
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
    #if !USE_VECTOR_MATH
        return (x * Other.x) + (y * Other.y) + (z * Other.z) + (w * Other.w);
    #else
        FFloat128 Temp0 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Temp1 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Other));
        FFloat128 Dot   = FVectorMath::VectorDot(Temp0, Temp1);
        return FVectorMath::VectorGetX(Dot);
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
        FVector4 NewVector;
        
    #if !USE_VECTOR_MATH
        NewVector = FVector4((y * Other.z) - (z * Other.y), (z * Other.x) - (x * Other.z), (x * Other.y) - (y * Other.x), 0.0f);
    #else
        FFloat128 Temp0  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Temp1  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Other));
        FFloat128 Result = FVectorMath::VectorCross(Temp0, Temp1);
        
        FInt128 Mask = FVectorMath::Load(~0, ~0, ~0, 0);
        Result = FVectorMath::And(Result, FVectorMath::VectorIntToFloat(Mask));

        FVectorMath::StoreAligned(Result, reinterpret_cast<float*>(&NewVector));
    #endif

        return NewVector;
    }

    /**
     * @brief       - Returns the resulting vector after projecting this vector onto another.
     * @param Other - The vector to project onto
     * @return      - The projected vector
     */
    FVector4 ProjectOn(const FVector4& Other) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        float AdotB = DotProduct(Other);
        float BdotB = Other.LengthSquared();
        Result = (AdotB / BdotB) * Other;
    #else
        FFloat128 Temp0 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Temp1 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Other));

        FFloat128 AdotB = FVectorMath::VectorDot(Temp0, Temp1);
        AdotB = FVectorMath::VectorShuffle<0, 1, 0, 1>(AdotB);

        FFloat128 BdotB = FVectorMath::VectorDot(Temp1, Temp1);
        BdotB = FVectorMath::VectorShuffle<0, 1, 0, 1>(BdotB);
        BdotB = FVectorMath::VectorDiv(AdotB, BdotB);
        BdotB = FVectorMath::VectorMul(BdotB, Temp1);

        FVectorMath::StoreAligned(BdotB, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief        - Returns the reflected vector after reflecting this vector around a normal.
     * @param Normal - Vector to reflect around
     * @return       - The reflected vector
     */
    FVector4 Reflect(const FVector4& Normal) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        float VdotN = DotProduct(Normal);
        float NdotN = Normal.LengthSquared();
        Result = *this - ((2.0f * (VdotN / NdotN)) * Normal);
    #else
        FFloat128 Temp0 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Temp1 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Normal));

        FFloat128 VdotN = FVectorMath::VectorDot(Temp0, Temp1);
        VdotN = FVectorMath::VectorShuffle<0, 1, 0, 1>(VdotN);

        FFloat128 NdotN = FVectorMath::VectorDot(Temp1, Temp1);
        NdotN = FVectorMath::VectorShuffle<0, 1, 0, 1>(NdotN);

        FFloat128 Reg2 = FVectorMath::Load(2.0f);
        VdotN = FVectorMath::VectorDiv(VdotN, NdotN);
        VdotN = FVectorMath::VectorMul(Reg2, VdotN);
        Temp1 = FVectorMath::VectorMul(VdotN, Temp1);
        Temp0 = FVectorMath::VectorSub(Temp0, Temp1);

        FVectorMath::StoreAligned(Temp0, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(FMath::Min(First.x, Second.x), FMath::Min(First.y, Second.y), FMath::Min(First.z, Second.z), FMath::Min(First.w, Second.w));
    #else

        FFloat128 Temp0 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&First));
        FFloat128 Temp1 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Second));
        Temp0 = FVectorMath::Min(Temp0, Temp1);
        FVectorMath::StoreAligned(Temp0, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief        - Returns a vector with the largest of each component of two vectors
     * @param First  - First vector to compare with
     * @param Second - Second vector to compare with
     * @return       - A vector with the largest components of First and Second
     */
    friend FORCEINLINE FVector4 Max(const FVector4& First, const FVector4& Second) noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(FMath::Max(First.x, Second.x), FMath::Max(First.y, Second.y), FMath::Max(First.z, Second.z), FMath::Max(First.w, Second.w));
    #else
        FFloat128 Temp0 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&First));
        FFloat128 Temp1 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Second));
        Temp0 = FVectorMath::Max(Temp0, Temp1);
        FVectorMath::StoreAligned(Temp0, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4((1.0f - t) * First.x + t * Second.x, (1.0f - t) * First.y + t * Second.y, (1.0f - t) * First.z + t * Second.z, (1.0f - t) * First.w + t * Second.w);
    #else
        FFloat128 Temp0 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&First));
        FFloat128 Temp1 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Second));
        FFloat128 Temp3 = FVectorMath::Load(t);
        
        FFloat128 Ones  = FVectorMath::LoadOnes();
        
        FFloat128 Temp4 = FVectorMath::VectorSub(Ones, Temp3);
        Temp0 = FVectorMath::VectorMul(Temp0, Temp4);
        Temp1 = FVectorMath::VectorMul(Temp1, Temp3);
        Temp0 = FVectorMath::VectorAdd(Temp0, Temp1);

        FVectorMath::StoreAligned(Temp0, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(FMath::Clamp(Min.x, Max.x, Value.x), FMath::Clamp(Min.y, Max.y, Value.y), FMath::Clamp(Min.z, Max.z, Value.z), FMath::Clamp(Min.w, Max.w, Value.w));
    #else
        FFloat128 Min128   = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Min));
        FFloat128 Max128   = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Max));
        FFloat128 Value128 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Value));
        Value128 = FVectorMath::Max(Value128, Min128);
        Value128 = FVectorMath::Min(Value128, Max128);
        FVectorMath::StoreAligned(Value128, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief       - Returns a vector with all the components within the range zero and one
     * @param Value - Value to saturate
     * @return      - A vector with the result of saturation
     */
    friend FORCEINLINE FVector4 Saturate(const FVector4& Value) noexcept
    {
        FVector4 Result;
    
    #if !USE_VECTOR_MATH
        Result = FVector4(FMath::Clamp(0.0f, 1.0f, Value.x), FMath::Clamp(0.0f, 1.0f, Value.y), FMath::Clamp(0.0f, 1.0f, Value.z), FMath::Clamp(0.0f, 1.0f, Value.w));
    #else
        FFloat128 Zeros    = FVectorMath::LoadZeros();
        FFloat128 Ones     = FVectorMath::LoadOnes();
        FFloat128 Value128 = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&Value));
        Value128 = FVectorMath::Max(Value128, Zeros);
        Value128 = FVectorMath::Min(Value128, Ones);
        FVectorMath::StoreAligned(Value128, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

public:

    /**
     * @brief  - Return a vector with component-wise negation of this vector
     * @return - A negated vector
     */
    FORCEINLINE FVector4 operator-() const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(-x, -y, -z, -w);
    #else
        FFloat128 Zeros = FVectorMath::LoadZeros();
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        This = FVectorMath::VectorSub(Zeros, This);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns the result of component-wise adding this and another vector
     * @param RHS - The vector to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FVector4 operator+(const FVector4& RHS) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(x + RHS.x, y + RHS.y, z + RHS.z, w + RHS.w);
    #else
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Other = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorAdd(This, Other);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns this vector after component-wise adding this with another vector
     * @param RHS - The vector to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator+=(const FVector4& RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        return *this = *this + RHS;
    #else
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Other = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorAdd(This, Other);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(this));
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(x + RHS, y + RHS, z + RHS, w + RHS);
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Scalars = FVectorMath::Load(RHS);
        This = FVectorMath::VectorAdd(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns this vector after adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator+=(float RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        return *this = *this + RHS;
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Scalars = FVectorMath::Load(RHS);
        This = FVectorMath::VectorAdd(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(this));
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(x - RHS.x, y - RHS.y, z - RHS.z, w - RHS.w);
    #else
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Other = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorSub(This, Other);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns this vector after component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator-=(const FVector4& RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        return *this = *this - RHS;
    #else
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Other = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorSub(This, Other);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(this));
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(x - RHS, y - RHS, z - RHS, w - RHS);
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Scalars = FVectorMath::Load(RHS);
        This = FVectorMath::VectorSub(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns this vector after subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator-=(float RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        return *this = *this - RHS;
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Scalars = FVectorMath::Load(RHS);
        This = FVectorMath::VectorSub(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(this));
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        return FVector4(x * RHS.x, y * RHS.y, z * RHS.z, w * RHS.w);
    #else
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Other = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorMul(This, Other);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns this vector after component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator*=(const FVector4& RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        return *this = *this * RHS;
    #else
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Other = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorMul(This, Other);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(this));
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(x * RHS, y * RHS, z * RHS, w * RHS);
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Scalars = FVectorMath::Load(RHS);
        This = FVectorMath::VectorMul(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns the result of multiplying each component of a vector with a scalar
     * @param LHS - The scalar to multiply with
     * @param RHS - The vector to multiply with
     * @return    - A vector with the result of the multiplication
     */
    friend FORCEINLINE FVector4 operator*(float LHS, const FVector4& RHS) noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(RHS.x * LHS, RHS.y * LHS, RHS.z * LHS, RHS.w * LHS);
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        FFloat128 Scalars = FVectorMath::Load(LHS);
        This = FVectorMath::VectorMul(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns this vector after multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4 operator*=(float RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        return *this = *this * RHS;
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Scalars = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorMul(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(this));
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(x / RHS.x, y / RHS.y, z / RHS.z, w / RHS.w);
    #else
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Other = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorDiv(This, Other);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns this vector after component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator/=(const FVector4& RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        return *this = *this / RHS;
    #else
        FFloat128 This  = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Other = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        This = FVectorMath::VectorDiv(This, Other);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(this));
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
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(x / RHS, y / RHS, z / RHS, w / RHS);
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Scalars = FVectorMath::Load(RHS);
        This = FVectorMath::VectorDiv(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief     - Returns this vector after dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector4& operator/=(float RHS) noexcept
    {
    #if !USE_VECTOR_MATH
        return *this = *this / RHS;
    #else
        FFloat128 This    = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Scalars = FVectorMath::Load(RHS);
        This = FVectorMath::VectorDiv(This, Scalars);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(this));
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
