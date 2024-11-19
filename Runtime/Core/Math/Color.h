#pragma once
#include "Core/Math/Float.h"
#include "Core/Math/MathHash.h"

struct FColor
{
    /** @brief - Default constructor */
    FColor() = default;
    
    /**
     * @brief     - Initialize color with all channels
     * @param InR - Red channel
     * @param InG - Green channel
     * @param InB - Blue channel
     * @param InA - Alpha channel
     */
    FORCEINLINE FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA)
        : R(InR)
        , G(InG)
        , B(InB)
        , A(InA)
    {
    }

    FORCEINLINE uint8* Data()
    {
        return reinterpret_cast<uint8*>(this);
    }

    FORCEINLINE const uint8* Data() const
    {
        return reinterpret_cast<const uint8*>(this);
    }

    bool operator==(const FColor& RHS) const
    {
        return (R == RHS.R) && (G == RHS.G) && (B == RHS.B) && (A == RHS.A);
    }

    bool operator!=(const FColor& RHS) const
    {
        return !(*this == RHS);
    }

    friend uint64 GetHashForType(const FColor& Value)
    {
        uint64 Hash = 0;
        HashCombine(Hash, Value.R);
        HashCombine(Hash, Value.G);
        HashCombine(Hash, Value.B);
        HashCombine(Hash, Value.A);
        return Hash;
    }

    /** @brief - Red channel */
    uint8 R{0};

    /** @brief - Green channel */
    uint8 G{0};

    /** @brief - Blue channel */
    uint8 B{0};

    /** @brief - Alpha channel */
    uint8 A{0};
};

MARK_AS_REALLOCATABLE(FColor);


struct FFloatColor16
{
    /**
     * @brief - Default constructor
     */
    FORCEINLINE FFloatColor16()
        : R(0.0f)
        , G(0.0f)
        , B(0.0f)
        , A(0.0f)
    {
    }

    /**
     * @brief     - Initialize color with all channels
     * @param InR - Red channel
     * @param InG - Green channel
     * @param InB - Blue channel
     * @param InA - Alpha channel
     */
    FORCEINLINE FFloatColor16(FFloat16 InR, FFloat16 InG, FFloat16 InB, FFloat16 InA)
        : R(InR)
        , G(InG)
        , B(InB)
        , A(InA)
    {
    }

    FORCEINLINE uint16* Data()
    {
        return reinterpret_cast<uint16*>(this);
    }

    FORCEINLINE const uint16* Data() const
    {
        return reinterpret_cast<const uint16*>(this);
    }

    bool operator==(const FFloatColor16& RHS) const
    {
        return (R == RHS.R) && (G == RHS.G) && (B == RHS.B) && (A == RHS.A);
    }

    bool operator!=(const FFloatColor16& RHS) const
    {
        return !(*this == RHS);
    }

    friend uint64 GetHashForType(const FFloatColor16& Value)
    {
        uint64 Hash = 0;
        HashCombine(Hash, Value.R);
        HashCombine(Hash, Value.G);
        HashCombine(Hash, Value.B);
        HashCombine(Hash, Value.A);
        return Hash;
    }

    /** @brief - Red channel */
    FFloat16 R;

    /** @brief - Green channel */
    FFloat16 G;

    /** @brief - Blue channel */
    FFloat16 B;

    /** @brief - Alpha channel */
    FFloat16 A;
};

MARK_AS_REALLOCATABLE(FFloatColor16);


struct FFloatColor
{
    /** @brief - Default constructor */
    FFloatColor() = default;

    /**
     * @brief     - Initialize color with all channels
     * @param InR - Red channel
     * @param InG - Green channel
     * @param InB - Blue channel
     * @param InA - Alpha channel
     */
    FORCEINLINE FFloatColor(float InR, float InG, float InB, float InA)
        : r(InR)
        , g(InG)
        , b(InB)
        , a(InA)
    {
    }

    bool operator==(const FFloatColor& RHS) const
    {
        return r == RHS.r && g == RHS.g && b == RHS.b && a == RHS.a;
    }

    bool operator!=(const FFloatColor& RHS) const
    {
        return !(*this == RHS);
    }

    friend uint64 GetHashForType(const FFloatColor& Value)
    {
        uint64 Hash = 0;
        HashCombine(Hash, Value.r);
        HashCombine(Hash, Value.g);
        HashCombine(Hash, Value.b);
        HashCombine(Hash, Value.a);
        return Hash;
    }

    /** @brief - Red channel */
    float r{0.0f};
    
    /** @brief - Green channel */
    float g{0.0f};
    
    /** @brief - Blue channel */
    float b{0.0f};
    
    /** @brief - Alpha channel */
    float a{0.0f};
};

MARK_AS_REALLOCATABLE(FFloatColor);
