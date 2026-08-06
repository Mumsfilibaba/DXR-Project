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

// FRGBA16Snorm Constants
static constexpr int32 FRGBA16SNORM_COLOR_BITS = 16;
static constexpr float FRGBA16SNORM_MAX        = 32767.0f;

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
        Vector3 Vector(InR, InG, InB);
        Vector.Normalize();

        Vector.X = Math::Clamp(Vector.X, 0.0f, 1.0f);
        Vector.Y = Math::Clamp(Vector.Y, 0.0f, 1.0f);
        Vector.Z = Math::Clamp(Vector.Z, 0.0f, 1.0f);

        const float ClampedA = Math::Clamp(InA, 0.0f, 1.0f);

        R = static_cast<uint32>(Math::RoundToInt(Vector.X * static_cast<float>(FR10G10B10A2_COLOR_MASK)));
        G = static_cast<uint32>(Math::RoundToInt(Vector.Y * static_cast<float>(FR10G10B10A2_COLOR_MASK)));
        B = static_cast<uint32>(Math::RoundToInt(Vector.Z * static_cast<float>(FR10G10B10A2_COLOR_MASK)));
        A = static_cast<uint32>(Math::RoundToInt(ClampedA * static_cast<float>(FR10G10B10A2_ALPHA_MASK)));
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

template<>
struct THash<FR10G10B10A2>
{
    static uint64 GetHash(const FR10G10B10A2& Value)
    {
        // Using a prime multiplier for hashing
        uint64 Result = 17;
        Result = Result * 31 + static_cast<uint64>(Value.A);
        Result = Result * 31 + static_cast<uint64>(Value.R);
        Result = Result * 31 + static_cast<uint64>(Value.G);
        Result = Result * 31 + static_cast<uint64>(Value.B);
        return Result;
    }
};

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

template<>
struct THash<FRG16F>
{
    static uint64 GetHash(const FRG16F& Value)
    {
        return THash<decltype(Value.RG)>::GetHash(Value.RG);
    }
};

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

template<>
struct THash<FRGBA16F>
{
    static uint64 GetHash(const FRGBA16F& Value)
    {
        return THash<decltype(Value.ARGB)>::GetHash(Value.ARGB);
    }
};

/**
 * @brief Four signed-normalized 16-bit channels in R, G, B, A memory order.
 */
struct FRGBA16Snorm
{
    /**
     * @brief Encodes a float in [-1, 1] to a signed-normalized 16-bit channel.
     * @param Value Value to encode, clamped to the representable range.
     * @return The encoded channel.
     */
    static FORCEINLINE int16 EncodeChannel(float Value)
    {
        const float Clamped = Math::Clamp(Value, -1.0f, 1.0f);
        return static_cast<int16>(Math::RoundToInt(Clamped * FRGBA16SNORM_MAX));
    }

    /**
     * @brief Decodes a signed-normalized 16-bit channel to a float in [-1, 1].
     * @param Value Channel to decode.
     * @return The decoded value.
     */
    static FORCEINLINE float DecodeChannel(int16 Value)
    {
        // -32768 maps below -1.0f, which every API clamps rather than represents.
        return Math::Max(static_cast<float>(Value) / FRGBA16SNORM_MAX, -1.0f);
    }

    /**
     * @brief Default constructor initializes all channels to zero.
     */
    FORCEINLINE FRGBA16Snorm()
        : RGBA(0)
    {
    }

    /**
     * @brief Constructs FRGBA16Snorm with pre-encoded channel values.
     * @param InR 16-bit Red value.
     * @param InG 16-bit Green value.
     * @param InB 16-bit Blue value.
     * @param InA 16-bit Alpha value.
     */
    FORCEINLINE FRGBA16Snorm(int16 InR, int16 InG, int16 InB, int16 InA)
        : R(InR)
        , G(InG)
        , B(InB)
        , A(InA)
    {
    }

    /**
     * @brief Constructs FRGBA16Snorm from signed-normalized float values.
     * @param InR Red component (-1.0f to 1.0f).
     * @param InG Green component (-1.0f to 1.0f).
     * @param InB Blue component (-1.0f to 1.0f).
     * @param InA Alpha component (-1.0f to 1.0f, default is 0).
     */
    FORCEINLINE FRGBA16Snorm(float InR, float InG, float InB, float InA = 0.0f)
        : R(EncodeChannel(InR))
        , G(EncodeChannel(InG))
        , B(EncodeChannel(InB))
        , A(EncodeChannel(InA))
    {
    }

    /**
     * @brief Constructs FRGBA16Snorm from a vector and a separate fourth channel.
     * @param InRGB Red, Green and Blue components (-1.0f to 1.0f).
     * @param InA Alpha component (-1.0f to 1.0f, default is 0).
     */
    FORCEINLINE explicit FRGBA16Snorm(const Vector3& InRGB, float InA = 0.0f)
        : R(EncodeChannel(InRGB.X))
        , G(EncodeChannel(InRGB.Y))
        , B(EncodeChannel(InRGB.Z))
        , A(EncodeChannel(InA))
    {
    }

    /**
     * @brief Decodes the Red, Green and Blue channels into a vector.
     * @return The decoded vector.
     */
    FORCEINLINE Vector3 ToVector3() const
    {
        return Vector3(DecodeChannel(R), DecodeChannel(G), DecodeChannel(B));
    }

    /**
     * @brief Decodes the Alpha channel.
     * @return The decoded value.
     */
    FORCEINLINE float GetAlpha() const
    {
        return DecodeChannel(A);
    }

    /**
     * @brief Packs the RGBA channels into a 64-bit unsigned integer.
     * @return A 64-bit unsigned integer representing the packed RGBA channels.
     */
    FORCEINLINE uint64 ToPackedRGBA() const
    {
        return RGBA;
    }

    /**
     * @brief Equality operator.
     * @param Other The right-hand side FRGBA16Snorm to compare with.
     * @return True if all channels are equal.
     */
    FORCEINLINE bool operator==(const FRGBA16Snorm& Other) const
    {
        return RGBA == Other.RGBA;
    }

    /**
     * @brief Inequality operator.
     * @param Other The right-hand side FRGBA16Snorm to compare with.
     * @return True if any channel differs.
     */
    FORCEINLINE bool operator!=(const FRGBA16Snorm& Other) const
    {
        return !(*this == Other);
    }

public:

    union
    {
        struct
        {
            /** @brief 16-bit Red channel */
            int16 R;

            /** @brief 16-bit Green channel */
            int16 G;

            /** @brief 16-bit Blue channel */
            int16 B;

            /** @brief 16-bit Alpha channel */
            int16 A;
        };

        uint64 RGBA;
    };
};

static_assert(sizeof(FRGBA16Snorm) == sizeof(uint64), "FRGBA16Snorm is assumed to have the same size as a uint64");
MARK_AS_REALLOCATABLE(FRGBA16Snorm);

template<>
struct THash<FRGBA16Snorm>
{
    static uint64 GetHash(const FRGBA16Snorm& Value)
    {
        return THash<decltype(Value.RGBA)>::GetHash(Value.RGBA);
    }
};

#pragma pack(pop) // End struct packing
