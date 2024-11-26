#pragma once
#include "Core/Math/Float.h"
#include "Core/Math/MathHash.h"
#include "Core/Math/Vector4.h"
#include "Core/Memory/Memory.h"

class FFloatColor;

class FColor
{
public:

    /** @brief Default constructor */
    FColor()
        : PackedRGBA(0)
    {
    }

    /**
     * @brief Initializes color with all channels
     * @param InR Red channel
     * @param InG Green channel
     * @param InB Blue channel
     * @param InA Alpha channel
     */
    FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA)
        : R(InR)
        , G(InG)
        , B(InB)
        , A(InA)
    {
    }

    /**
     * @brief Packs the RGBA channels into a 32-bit unsigned integer.
     * @return A 32-bit unsigned integer representing the packed color.
     */
    uint32 ToPackedRGBA() const
    {
        return (static_cast<uint32>(R) << 24) | (static_cast<uint32>(G) << 16) | (static_cast<uint32>(B) << 8) | static_cast<uint32>(A);
    }

    /**
     * @brief Converts this color to a float color.
     * @return An `FFloatColor` representation of this color.
     */
    FFloatColor ToFloatColor() const;

    /** @brief Equality operator */
    bool operator==(const FColor& Other) const
    {
        return PackedRGBA == Other.PackedRGBA;
    }

    /** @brief Inequality operator */
    bool operator!=(const FColor& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Generates a hash value for this color.
     * @param Value The color to hash.
     * @return A 64-bit hash value.
     */
    friend uint64 GetHashForType(const FColor& Value)
    {
        uint64 Hash = 0;
        HashCombine(Hash, Value.R);
        HashCombine(Hash, Value.G);
        HashCombine(Hash, Value.B);
        HashCombine(Hash, Value.A);
        return Hash;
    }

public:
    union
    {
        struct
        {
            /** @brief Red channel */
            uint8 R;

            /** @brief Green channel */
            uint8 G;

            /** @brief Blue channel */
            uint8 B;

            /** @brief Alpha channel */
            uint8 A;
        };

        uint32 PackedRGBA;
    };
};

static_assert(TIsStandardLayout<FColor>::Value, "FColor must be a standard layout type");
MARK_AS_REALLOCATABLE(FColor);

class FFloatColor
{
public:

    /** @brief Black color */
    static CORE_API const FFloatColor Black;

    /** @brief White color */
    static CORE_API const FFloatColor White;

    /** @brief Red color */
    static CORE_API const FFloatColor Red;

    /** @brief Green color */
    static CORE_API const FFloatColor Green;

    /** @brief Blue color */
    static CORE_API const FFloatColor Blue;

public:

    /** @brief Default constructor */
    FFloatColor()
    {
        FMemory::Memzero(RGBA, sizeof(RGBA));
    }

    /**
     * @brief Initializes color with all channels
     * @param InR Red channel
     * @param InG Green channel
     * @param InB Blue channel
     * @param InA Alpha channel
     */
    FFloatColor(float InR, float InG, float InB, float InA)
        : R(InR)
        , G(InG)
        , B(InB)
        , A(InA)
    {
    }

    /**
     * @brief Constructs a float color from a 3D vector.
     * @param InVector The input vector containing RGB components.
     */
    explicit FFloatColor(const FVector3& InVector)
    {
        FMemory::Memcpy(RGBA, &InVector, sizeof(FVector3));
    }

    /**
     * @brief Constructs a float color from a 4D vector.
     * @param InVector The input vector containing RGBA components.
     */
    explicit FFloatColor(const FVector4& InVector)
    {
        FMemory::Memcpy(RGBA, &InVector, sizeof(RGBA));
    }

    /**
     * @brief Converts this float color to an `FColor` by scaling and clamping each component.
     * @return An `FColor` representation of this float color.
     */
    FColor ToColor() const
    {
        // Clamp each component between 0.0f and 1.0f
        const float ClampedR = FMath::Clamp(R, 0.0f, 1.0f);
        const float ClampedG = FMath::Clamp(G, 0.0f, 1.0f);
        const float ClampedB = FMath::Clamp(B, 0.0f, 1.0f);
        const float ClampedA = FMath::Clamp(A, 0.0f, 1.0f);

        // Scale to [0, 255] and round to nearest integer
        uint8 IntR = static_cast<uint8>(FMath::RoundToInt(ClampedR * 255.0f));
        uint8 IntG = static_cast<uint8>(FMath::RoundToInt(ClampedG * 255.0f));
        uint8 IntB = static_cast<uint8>(FMath::RoundToInt(ClampedB * 255.0f));
        uint8 IntA = static_cast<uint8>(FMath::RoundToInt(ClampedA * 255.0f));

        return FColor(IntR, IntG, IntB, IntA);
    }

    /**
     * @brief Returns a color with component-wise negation of this color.
     * @return A negated color.
     */
    FFloatColor operator-() const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(-R, -G, -B, -A);
    #else
        FFloat128 Zero_128   = FVectorMath::VectorZero();
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Result_128 = FVectorMath::VectorSub(Zero_128, RGBA_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts another color from this color component-wise.
     * @param ColorB The color to subtract.
     * @return The result of the subtraction.
     */
    FFloatColor operator-(const FFloatColor& ColorB) const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(R - ColorB.R, G - ColorB.G, B - ColorB.B, A - ColorB.A);
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 ColorB_128 = FVectorMath::VectorLoad(ColorB.RGBA);
        FFloat128 Result_128 = FVectorMath::VectorSub(RGBA_128, ColorB_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts another color from this color component-wise and updates this color.
     * @param ColorB The color to subtract.
     * @return A reference to this color after subtraction.
     */
    FFloatColor& operator-=(const FFloatColor& ColorB) noexcept
    {
    #if !USE_VECTOR_MATH
        R -= ColorB.R;
        G -= ColorB.G;
        B -= ColorB.B;
        A -= ColorB.A;
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 ColorB_128 = FVectorMath::VectorLoad(ColorB.RGBA);
        FFloat128 Result_128 = FVectorMath::VectorSub(RGBA_128, ColorB_128);
        FVectorMath::VectorStore(Result_128, RGBA);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies this color by another color component-wise.
     * @param ColorB The color to multiply with.
     * @return The result of the multiplication.
     */
    FFloatColor operator*(const FFloatColor& ColorB) const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(R * ColorB.R, G * ColorB.G, B * ColorB.B, A * ColorB.A);
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 ColorB_128 = FVectorMath::VectorLoad(ColorB.RGBA);
        FFloat128 Result_128 = FVectorMath::VectorMul(RGBA_128, ColorB_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies this color by another color component-wise and updates this color.
     * @param ColorB The color to multiply with.
     * @return A reference to this color after multiplication.
     */
    FFloatColor& operator*=(const FFloatColor& ColorB) noexcept
    {
    #if !USE_VECTOR_MATH
        R *= ColorB.R;
        G *= ColorB.G;
        B *= ColorB.B;
        A *= ColorB.A;
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 ColorB_128 = FVectorMath::VectorLoad(ColorB.RGBA);
        FFloat128 Result_128 = FVectorMath::VectorMul(RGBA_128, ColorB_128);
        FVectorMath::VectorStore(Result_128, RGBA);
    #endif

        return *this;
    }

    /**
     * @brief Divides this color by another color component-wise.
     * @param ColorB The color to divide by.
     * @return The result of the division.
     */
    FFloatColor operator/(const FFloatColor& ColorB) const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(R / ColorB.R, G / ColorB.G, B / ColorB.B, A / ColorB.A);
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 ColorB_128 = FVectorMath::VectorLoad(ColorB.RGBA);
        FFloat128 Result_128 = FVectorMath::VectorDiv(RGBA_128, ColorB_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Divides this color by another color component-wise and updates this color.
     * @param ColorB The color to divide by.
     * @return A reference to this color after division.
     */
    FFloatColor& operator/=(const FFloatColor& ColorB) noexcept
    {
    #if !USE_VECTOR_MATH
        R /= ColorB.R;
        G /= ColorB.G;
        B /= ColorB.B;
        A /= ColorB.A;
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 ColorB_128 = FVectorMath::VectorLoad(ColorB.RGBA);
        FFloat128 Result_128 = FVectorMath::VectorDiv(RGBA_128, ColorB_128);
        FVectorMath::VectorStore(Result_128, RGBA);
    #endif

        return *this;
    }

    /**
     * @brief Adds another color to this color component-wise.
     * @param ColorB The color to add.
     * @return The result of the addition.
     */
    FFloatColor operator+(const FFloatColor& ColorB) const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(R + ColorB.R, G + ColorB.G, B + ColorB.B, A + ColorB.A);
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 ColorB_128 = FVectorMath::VectorLoad(ColorB.RGBA);
        FFloat128 Result_128 = FVectorMath::VectorAdd(RGBA_128, ColorB_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Adds another color to this color component-wise and updates this color.
     * @param ColorB The color to add.
     * @return A reference to this color after addition.
     */
    FFloatColor& operator+=(const FFloatColor& ColorB) noexcept
    {
    #if !USE_VECTOR_MATH
        R += ColorB.R;
        G += ColorB.G;
        B += ColorB.B;
        A += ColorB.A;
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 ColorB_128 = FVectorMath::VectorLoad(ColorB.RGBA);
        FFloat128 Result_128 = FVectorMath::VectorAdd(RGBA_128, ColorB_128);
        FVectorMath::VectorStore(Result_128, RGBA);
    #endif

        return *this;
    }

    /**
     * @brief Adds a scalar to each component of this color.
     * @param Scalar The scalar value to add.
     * @return The result of the addition.
     */
    FFloatColor operator+(float Scalar) const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(R + Scalar, G + Scalar, B + Scalar, A + Scalar);
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Scalar_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128 = FVectorMath::VectorAdd(RGBA_128, Scalar_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Adds a scalar to each component of this color and updates this color.
     * @param Scalar The scalar value to add.
     * @return A reference to this color after addition.
     */
    FFloatColor& operator+=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        R += Scalar;
        G += Scalar;
        B += Scalar;
        A += Scalar;
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Scalar_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128 = FVectorMath::VectorAdd(RGBA_128, Scalar_128);
        FVectorMath::VectorStore(Result_128, RGBA);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component of this color.
     * @param Scalar The scalar value to subtract.
     * @return The result of the subtraction.
     */
    FFloatColor operator-(float Scalar) const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(R - Scalar, G - Scalar, B - Scalar, A - Scalar);
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Scalar_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128 = FVectorMath::VectorSub(RGBA_128, Scalar_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts a scalar from each component of this color and updates this color.
     * @param Scalar The scalar value to subtract.
     * @return A reference to this color after subtraction.
     */
    FFloatColor& operator-=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        R -= Scalar;
        G -= Scalar;
        B -= Scalar;
        A -= Scalar;
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Scalar_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128 = FVectorMath::VectorSub(RGBA_128, Scalar_128);
        FVectorMath::VectorStore(Result_128, RGBA);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies each component of this color by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return The result of the multiplication.
     */
    FFloatColor operator*(float Scalar) const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(R * Scalar, G * Scalar, B * Scalar, A * Scalar);
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Scalar_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128 = FVectorMath::VectorMul(RGBA_128, Scalar_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies each component of this color by a scalar and updates this color.
     * @param Scalar The scalar value to multiply with.
     * @return A reference to this color after multiplication.
     */
    FFloatColor& operator*=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        R *= Scalar;
        G *= Scalar;
        B *= Scalar;
        A *= Scalar;
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Scalar_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128 = FVectorMath::VectorMul(RGBA_128, Scalar_128);
        FVectorMath::VectorStore(Result_128, RGBA);
    #endif

        return *this;
    }

    /**
     * @brief Divides each component of this color by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return The result of the division.
     */
    FFloatColor operator/(float Scalar) const noexcept
    {
        FFloatColor Result;

    #if !USE_VECTOR_MATH
        Result = FFloatColor(R / Scalar, G / Scalar, B / Scalar, A / Scalar);
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Scalar_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128 = FVectorMath::VectorDiv(RGBA_128, Scalar_128);
        FVectorMath::VectorStore(Result_128, Result.RGBA);
    #endif

        return Result;
    }

    /**
     * @brief Divides each component of this color by a scalar and updates this color.
     * @param Scalar The scalar value to divide by.
     * @return A reference to this color after division.
     */
    FFloatColor& operator/=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        R /= Scalar;
        G /= Scalar;
        B /= Scalar;
        A /= Scalar;
    #else
        FFloat128 RGBA_128   = FVectorMath::VectorLoad(RGBA);
        FFloat128 Scalar_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128 = FVectorMath::VectorDiv(RGBA_128, Scalar_128);
        FVectorMath::VectorStore(Result_128, RGBA);
    #endif

        return *this;
    }

    /**
     * @brief Equality operator with approximate comparison.
     * @param RHS The color to compare with.
     * @return `true` if colors are approximately equal, `false` otherwise.
     */
    bool operator==(const FFloatColor& RHS) const
    {
        constexpr float Epsilon = 1e-6f;
        return (FMath::Abs(R - RHS.R) <= Epsilon) && (FMath::Abs(G - RHS.G) <= Epsilon) && (FMath::Abs(B - RHS.B) <= Epsilon) && (FMath::Abs(A - RHS.A) <= Epsilon);
    }

    /** @brief Inequality operator */
    bool operator!=(const FFloatColor& RHS) const
    {
        return !(*this == RHS);
    }

    /**
     * @brief Generates a hash value for this color.
     * @param Value The color to hash.
     * @return A 64-bit hash value.
     */
    friend uint64 GetHashForType(const FFloatColor& Value)
    {
        uint64 Hash = 0;
        HashCombine(Hash, BitCast<uint32>(Value.R));
        HashCombine(Hash, BitCast<uint32>(Value.G));
        HashCombine(Hash, BitCast<uint32>(Value.B));
        HashCombine(Hash, BitCast<uint32>(Value.A));
        return Hash;
    }

public:
    union
    {
        struct
        {
            /** @brief Red channel */
            float R;

            /** @brief Green channel */
            float G;

            /** @brief Blue channel */
            float B;

            /** @brief Alpha channel */
            float A;
        };

        float RGBA[4];
    };
};

inline FFloatColor FColor::ToFloatColor() const
{
    return FFloatColor(static_cast<float>(R) / 255.0f, static_cast<float>(G) / 255.0f, static_cast<float>(B) / 255.0f, static_cast<float>(A) / 255.0f);
}

static_assert(TIsStandardLayout<FFloatColor>::Value, "FFloatColor must be a standard layout type");
MARK_AS_REALLOCATABLE(FFloatColor);

class FFloatColor16
{
public:

    /** @brief Default constructor */
    FORCEINLINE FFloatColor16()
        : R(0.0f)
        , G(0.0f)
        , B(0.0f)
        , A(0.0f)
    {
    }

    /**
     * @brief Initializes color with all channels
     * @param InR Red channel
     * @param InG Green channel
     * @param InB Blue channel
     * @param InA Alpha channel
     */
    FORCEINLINE FFloatColor16(FFloat16 InR, FFloat16 InG, FFloat16 InB, FFloat16 InA)
        : R(InR)
        , G(InG)
        , B(InB)
        , A(InA)
    {
    }

    /** @brief Equality operator */
    bool operator==(const FFloatColor16& RHS) const
    {
        return R == RHS.R && G == RHS.G && B == RHS.B && A == RHS.A;
    }

    /** @brief Inequality operator */
    bool operator!=(const FFloatColor16& RHS) const
    {
        return !(*this == RHS);
    }

    /**
     * @brief Generates a hash value for this color.
     * @param Value The color to hash.
     * @return A 64-bit hash value.
     */
    friend uint64 GetHashForType(const FFloatColor16& Value)
    {
        uint64 Hash = 0;
        HashCombine(Hash, Value.R);
        HashCombine(Hash, Value.G);
        HashCombine(Hash, Value.B);
        HashCombine(Hash, Value.A);
        return Hash;
    }

public:

    /** @brief Red channel */
    FFloat16 R;

    /** @brief Green channel */
    FFloat16 G;

    /** @brief Blue channel */
    FFloat16 B;

    /** @brief Alpha channel */
    FFloat16 A;
};

static_assert(TIsStandardLayout<FFloatColor16>::Value, "FFloatColor16 must be a standard layout type");
MARK_AS_REALLOCATABLE(FFloatColor16);
