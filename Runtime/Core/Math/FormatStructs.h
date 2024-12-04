#pragma once
#include "Core/Math/Float.h"
#include "Core/Math/Vector3.h"
#include "Core/Templates/TypeHash.h"

// FR10G10B10A2 Constants
static constexpr int32  FR10G10B10A2_ALPHA_BITS = 2;
static constexpr int32  FR10G10B10A2_COLOR_BITS = 10;
static constexpr uint32 FR10G10B10A2_ALPHA_MASK = (1 << FR10G10B10A2_ALPHA_BITS) - 1; // 0x3
static constexpr uint32 FR10G10B10A2_COLOR_MASK = (1 << FR10G10B10A2_COLOR_BITS) - 1; // 0x3FF

// FRG16F Constants
static constexpr int32 FRG16F_COLOR_BITS = 16;

// FRGBA16F Constants
static constexpr int32 FRGBA16F_COLOR_BITS = 16;

#pragma pack(push, 1) // Ensure no padding

/**
 * @brief 10-bit Red, 10-bit Green, 10-bit Blue, 2-bit Alpha color representation.
 */
struct FR10G10B10A2
{
    /**
     * @brief Default constructor initializes all channels to zero.
     */
    FORCEINLINE FR10G10B10A2()
        : ARGB(0)
    {
    }

    /**
     * @brief Constructs FR10G10B10A2 with specified channel values.
     * @param InA 2-bit Alpha value.
     * @param InR 10-bit Red value.
     * @param InG 10-bit Green value.
     * @param InB 10-bit Blue value.
     */
    FORCEINLINE FR10G10B10A2(uint8 InA, uint16 InR, uint16 InG, uint16 InB)
        : A(InA)
        , R(InR)
        , G(InG)
        , B(InB)
    {
    }

    /**
     * @brief Constructs FR10G10B10A2 from normalized float RGBA values.
     * @param InR Red component (0.0f to 1.0f).
     * @param InG Green component (0.0f to 1.0f).
     * @param InB Blue component (0.0f to 1.0f).
     * @param InA Alpha component (0.0f to 1.0f, default is 0).
     */
    FORCEINLINE FR10G10B10A2(float InR, float InG, float InB, float InA = 0.0f)
        : ARGB(0)
    {
        FVector3 Vector(InR, InG, InB);
        Vector.Normalize();

        Vector.x = FMath::Clamp(Vector.x, 0.0f, 1.0f);
        Vector.y = FMath::Clamp(Vector.y, 0.0f, 1.0f);
        Vector.z = FMath::Clamp(Vector.z, 0.0f, 1.0f);

        const float ClampedA = FMath::Clamp(InA, 0.0f, 1.0f);

        R = static_cast<uint32>(FMath::RoundToInt(Vector.x * static_cast<float>(FR10G10B10A2_COLOR_MASK)));
        G = static_cast<uint32>(FMath::RoundToInt(Vector.y * static_cast<float>(FR10G10B10A2_COLOR_MASK)));
        B = static_cast<uint32>(FMath::RoundToInt(Vector.z * static_cast<float>(FR10G10B10A2_COLOR_MASK)));
        A = static_cast<uint32>(FMath::RoundToInt(ClampedA * static_cast<float>(FR10G10B10A2_ALPHA_MASK)));
    }

    /**
     * @brief Packs the ARGB channels into a 32-bit unsigned integer.
     * @return A 32-bit unsigned integer representing the packed color.
     */
    FORCEINLINE uint32 ToPackedARGB() const
    {
        return ARGB;
    }

    /**
     * @brief Equality operator.
     * @param Other The right-hand side FR10G10B10A2 to compare with.
     * @return True if all channels are equal.
     */
    FORCEINLINE bool operator==(const FR10G10B10A2& Other) const
    {
        return ARGB == Other.ARGB;
    }

    /**
     * @brief Inequality operator.
     * @param Other The right-hand side FR10G10B10A2 to compare with.
     * @return True if any channel differs.
     */
    FORCEINLINE bool operator!=(const FR10G10B10A2& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Hash function for FR10G10B10A2.
     * @param Value The FR10G10B10A2 value to hash.
     * @return Hash value.
     */
    friend uint64 GetHashForType(const FR10G10B10A2& Value)
    {
        // Using a prime multiplier for hashing
        uint64 hash = 17;
        hash = hash * 31 + static_cast<uint64>(Value.A);
        hash = hash * 31 + static_cast<uint64>(Value.R);
        hash = hash * 31 + static_cast<uint64>(Value.G);
        hash = hash * 31 + static_cast<uint64>(Value.B);
        return hash;
    }

public:

    union
    {
        struct
        {
            /** @brief 2-bit Alpha channel */
            uint32 A : FR10G10B10A2_ALPHA_BITS;

            /** @brief 10-bit Red channel */
            uint32 R : FR10G10B10A2_COLOR_BITS;

            /** @brief 10-bit Green channel */
            uint32 G : FR10G10B10A2_COLOR_BITS;

            /** @brief 10-bit Blue channel */
            uint32 B : FR10G10B10A2_COLOR_BITS;
        };

        uint32 ARGB;
    };
};

static_assert(sizeof(FR10G10B10A2) == sizeof(uint32), "FR10G10B10A2 is assumed to have the same size as a uint32");
MARK_AS_REALLOCATABLE(FR10G10B10A2);

/**
 * @brief 16-bit Red and 16-bit Green floating-point representation.
 */
struct FRG16F
{
    /**
     * @brief Default constructor initializes R and G to zero.
     */
    FORCEINLINE FRG16F()
        : RG(0)
    {
    }

    /**
     * @brief Constructs FRG16F with specified channel values.
     * @param InR 16-bit Red value.
     * @param InG 16-bit Green value.
     */
    FORCEINLINE FRG16F(uint16 InR, uint16 InG)
        : R(InR)
        , G(InG)
    {
    }

    /**
     * @brief Constructs FRG16F from normalized float RG values.
     * @param InR Red component (0.0f to 1.0f).
     * @param InG Green component (0.0f to 1.0f).
     */
    FORCEINLINE FRG16F(float InR, float InG)
        : RG(0)
    {
        // Convert normalized float to 16-bit half-precision
        R = FFloat16(InR).Encoded;
        G = FFloat16(InG).Encoded;
    }

    /**
     * @brief Packs the RG channels into a 32-bit unsigned integer.
     * @return A 32-bit unsigned integer representing the packed RG channels.
     */
    FORCEINLINE uint32 ToPackedRG() const 
    { 
        return RG;
    }

    /**
     * @brief Equality operator.
     * @param Other The right-hand side FRG16F to compare with.
     * @return True if both R and G channels are equal.
     */
    FORCEINLINE bool operator==(const FRG16F& Other) const
    {
        return RG == Other.RG;
    }

    /**
     * @brief Inequality operator.
     * @param Other The right-hand side FRG16F to compare with.
     * @return True if either R or G channels differ.
     */
    FORCEINLINE bool operator!=(const FRG16F& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Hash function for FRG16F.
     * @param Value The FRG16F value to hash.
     * @return Hash value.
     */
    friend uint64 GetHashForType(const FRG16F& Value) 
    {
        return GetHashForType(Value.RG);
    }

    union
    {
        struct
        {
            /** @brief 16-bit Red channel */
            uint16 R;

            /** @brief 16-bit Green channel */
            uint16 G;
        };

        uint32 RG;
    };
};

static_assert(sizeof(FRG16F) == sizeof(uint32), "FRG16F is assumed to have the same size as a uint32");
MARK_AS_REALLOCATABLE(FRG16F);

/**
 * @brief 16-bit Alpha, Red, Green, and Blue floating-point representation.
 */
struct FRGBA16F
{
    /**
     * @brief Default constructor initializes all channels to zero.
     */
    FORCEINLINE FRGBA16F()
        : ARGB(0)
    {
    }

    /**
     * @brief Constructs FRGBA16F with specified channel values.
     * @param InA 16-bit Alpha value.
     * @param InR 16-bit Red value.
     * @param InG 16-bit Green value.
     * @param InB 16-bit Blue value.
     */
    FORCEINLINE FRGBA16F(uint16 InA, uint16 InR, uint16 InG, uint16 InB)
        : A(InA)
        , R(InR)
        , G(InG)
        , B(InB)
    {
    }

    /**
     * @brief Constructs FRGBA16F from normalized float ARGB values.
     * @param InA Alpha component (0.0f to 1.0f).
     * @param InR Red component (0.0f to 1.0f).
     * @param InG Green component (0.0f to 1.0f).
     * @param InB Blue component (0.0f to 1.0f).
     */
    FORCEINLINE FRGBA16F(float InA, float InR, float InG, float InB)
        : ARGB(0)
    {
        A = FFloat16(InA).Encoded;
        R = FFloat16(InR).Encoded;
        G = FFloat16(InG).Encoded;
        B = FFloat16(InB).Encoded;
    }

    /**
     * @brief Packs the ARGB channels into a 64-bit unsigned integer.
     * @return A 64-bit unsigned integer representing the packed ARGB channels.
     */
    FORCEINLINE uint64 ToPackedARGB() const 
    { 
        return ARGB; 
    }

    /**
     * @brief Equality operator.
     * @param Other The right-hand side FRGBA16F to compare with.
     * @return True if all channels are equal.
     */
    FORCEINLINE bool operator==(const FRGBA16F& Other) const
    {
        return ARGB == Other.ARGB;
    }

    /**
     * @brief Inequality operator.
     * @param Other The right-hand side FRGBA16F to compare with.
     * @return True if any channel differs.
     */
    FORCEINLINE bool operator!=(const FRGBA16F& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Hash function for FRGBA16F.
     * @param Value The FRGBA16F value to hash.
     * @return Hash value.
     */
    friend uint64 GetHashForType(const FRGBA16F& Value) 
    {
        return GetHashForType(Value.ARGB);
    }

public:

    union
    {
        struct 
        {
            /** @brief 16-bit Alpha channel */
            uint16 A;

            /** @brief 16-bit Red channel */
            uint16 R;

            /** @brief 16-bit Green channel */
            uint16 G;

            /** @brief 16-bit Blue channel */
            uint16 B;
        };

        uint64 ARGB;
    };
};

static_assert(sizeof(FRGBA16F) == sizeof(uint64), "FRGBA16F is assumed to have the same size as a uint64");
MARK_AS_REALLOCATABLE(FRGBA16F);

#pragma pack(pop) // End struct packing
