#pragma once
#include "Core/Math/MathCommon.h"
#include <cfloat>

// Float32 (Single Precision) Constants
static constexpr uint32 FP32_HIDDEN_BIT      = 0x00800000U; // The implicit leading 1 in normalized float32 mantissa
static constexpr uint32 FP32_MAX_EXPONENT    = 0xFFU;       // Maximum exponent value for float32 (all bits set)
static constexpr int32  FP32_EXPONENT_BIAS   = 127;         // Exponent bias for float32
static constexpr int32  FP32_MANTISSA_BITS   = 23;          // Number of mantissa bits in float32

// Float16 (Half Precision) Constants
static constexpr uint16 FP16_MAX_EXPONENT    = 0x1FU;       // Maximum exponent value for float16 (all bits set)
static constexpr int32  FP16_EXPONENT_BIAS   = 15;          // Exponent bias for float16
static constexpr int32  FP16_MANTISSA_BITS   = 10;          // Number of mantissa bits in float16

// Float16 Specific Magic Constants
static constexpr uint16 FP16_MANTISSA_MASK   = 0x03FF;      // Mask to extract mantissa (10 bits)
static constexpr uint16 FP16_EXPONENT_MASK   = 0x1F;        // Mask to extract exponent (5 bits)
static constexpr uint16 FP16_SIGN_MASK       = 0x8000;      // Mask to extract sign bit (1 bit)

static constexpr uint32 FP16_MANTISSA_SHIFT  = 13;          // Number of bits to shift mantissa from float32 to float16
static constexpr uint32 FP16_SUBNORMAL_SHIFT = 23;          // Number of bits to shift mantissa for subnormals

static constexpr uint16 FP16_INFINITE        = FP16_MAX_EXPONENT << FP16_MANTISSA_BITS;           // Representation of infinity
static constexpr uint16 FP16_NAN             = (FP16_MAX_EXPONENT << FP16_MANTISSA_BITS) | 0x200; // Representation of NaN

struct FFloat64
{
    /** 
     * @brief Default constructor initializes the double value to zero.
     */
    FORCEINLINE FFloat64()
        : Float64(0.0)
    {
    }

    /**
     * @brief Constructs an FFloat64 with a double value.
     * @param InFloat64 The double value to initialize with.
     */
    FORCEINLINE FFloat64(double InFloat64)
        : Float64(InFloat64)
    {
    }

    /**
     * @brief Copy constructor.
     * @param Other The FFloat64 instance to copy from.
     */
    FORCEINLINE FFloat64(const FFloat64& Other)
        : Float64(Other.Float64)
    {
    }

    /**
     * @brief Sets the internal double value.
     * @param InFloat64 The double value to set.
     */
    FORCEINLINE void SetFloat(double InFloat64)
    {
        Float64 = InFloat64;
    }

    /**
     * @brief Retrieves the internal double value.
     * @return The stored double value.
     */
    FORCEINLINE double GetFloat() const
    {
        return Float64;
    }

    /**
     * @brief Returns the raw encoded bits of the double.
     * @return The 64-bit encoded value.
     */
    FORCEINLINE uint64 GetBits() const
    {
        return Encoded;
    }

    /**
     * @brief Equality operator.
     * @param Other The right-hand side FFloat64 to compare with.
     * @return True if the stored double values are equal.
     */
    FORCEINLINE bool operator==(const FFloat64& Other) const
    {
        return Float64 == Other.Float64;
    }

    /**
     * @brief Inequality operator.
     * @param Other The right-hand side FFloat64 to compare with.
     * @return True if the stored double values are not equal.
     */
    FORCEINLINE bool operator!=(const FFloat64& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Assignment operator from double.
     * @param InFloat64 The double value to assign.
     * @return Reference to this FFloat64 instance.
     */
    FORCEINLINE FFloat64& operator=(double InFloat64)
    {
        Float64 = InFloat64;
        return *this;
    }

    /**
     * @brief Copy assignment operator.
     * @param Other The FFloat64 instance to copy from.
     * @return Reference to this FFloat64 instance.
     */
    FORCEINLINE FFloat64& operator=(const FFloat64& Other)
    {
        Float64 = Other.Float64;
        return *this;
    }

public:

    /** Union for accessing the raw bits of the double value */
    union
    {
        double Float64; /** @brief The double value */
        uint64 Encoded; /** @brief The raw bits of the double value */
        struct
        {
            uint64 Mantissa : 52; /** @brief Mantissa (fractional part) */
            uint64 Exponent : 11; /** @brief Exponent */
            uint64 Sign     : 1;  /** @brief Sign bit */
        } Bits;
    };
};

static_assert(sizeof(FFloat64) == sizeof(double), "FFloat64 should have the same size as a regular double");
MARK_AS_REALLOCATABLE(FFloat64);

struct FFloat32
{
    /**
     * @brief Default constructor initializes the float value to zero.
     */
    FORCEINLINE FFloat32()
        : Float32(0.0f)
    {
    }

    /**
     * @brief Constructs an FFloat32 with a float value.
     * @param InFloat32 The float value to initialize with.
     */
    FORCEINLINE FFloat32(float InFloat32)
        : Float32(InFloat32)
    {
    }

    /**
     * @brief Copy constructor.
     * @param Other The FFloat32 instance to copy from.
     */
    FORCEINLINE FFloat32(const FFloat32& Other)
        : Float32(Other.Float32)
    {
    }

    /**
     * @brief Sets the internal float value.
     * @param InFloat32 The float value to set.
     */
    FORCEINLINE void SetFloat(float InFloat32)
    {
        Float32 = InFloat32;
    }

    /**
     * @brief Retrieves the internal float value.
     * @return The stored float value.
     */
    FORCEINLINE float GetFloat() const
    {
        return Float32;
    }

    /**
     * @brief Returns the raw encoded bits of the float.
     * @return The 32-bit encoded value.
     */
    FORCEINLINE uint32 GetBits() const
    {
        return Encoded;
    }

    /**
     * @brief Equality operator.
     * @param Other The right-hand side FFloat32 to compare with.
     * @return True if the stored float values are equal.
     */
    FORCEINLINE bool operator==(const FFloat32& Other) const
    {
        return Float32 == Other.Float32;
    }

    /**
     * @brief Inequality operator.
     * @param Other The right-hand side FFloat32 to compare with.
     * @return True if the stored float values are not equal.
     */
    FORCEINLINE bool operator!=(const FFloat32& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Assignment operator from float.
     * @param InFloat32 The float value to assign.
     * @return Reference to this FFloat32 instance.
     */
    FORCEINLINE FFloat32& operator=(float InFloat32)
    {
        Float32 = InFloat32;
        return *this;
    }

    /**
     * @brief Copy assignment operator.
     * @param Other The FFloat32 instance to copy from.
     * @return Reference to this FFloat32 instance.
     */
    FORCEINLINE FFloat32& operator=(const FFloat32& Other)
    {
        Float32 = Other.Float32;
        return *this;
    }

public:

    /** Union for accessing the raw bits of the float value */
    union
    {
        float  Float32; /** @brief The float value */
        uint32 Encoded; /** @brief The raw bits of the float value */
        struct
        {
            uint32 Mantissa : 23; /** @brief Mantissa (fractional part) */
            uint32 Exponent : 8;  /** @brief Exponent */
            uint32 Sign     : 1;  /** @brief Sign bit */
        } Bits;
    };
};

static_assert(sizeof(FFloat32) == sizeof(float), "FFloat32 should have the same size as a regular float");
MARK_AS_REALLOCATABLE(FFloat32);

struct FFloat16
{
    /**
     * @brief Default constructor initializes the encoded value to zero.
     */
    FORCEINLINE FFloat16()
        : Encoded(0)
    {
    }

    /**
     * @brief Constructs an FFloat16 from a 32-bit float.
     * @param Float32 The float value to convert and store.
     */
    FORCEINLINE FFloat16(float Float32)
    {
        SetFloat(Float32);
    }

    /**
     * @brief Copy constructor.
     * @param Other The FFloat16 instance to copy from.
     */
    FORCEINLINE FFloat16(const FFloat16& Other)
        : Encoded(Other.Encoded)
    {
    }

    /**
     * @brief Sets the half-precision float from a 32-bit float.
     * @param Float32 The 32-bit float to convert.
     * 
     * This function converts a 32-bit single-precision float to a 16-bit half-precision float,
     * handling special cases like NaN, infinity, zero, subnormal numbers, and normal numbers.
     * It aims to produce an accurate half-precision representation of the input float.
     */
    inline void SetFloat(float InFloat32)
    {
        // Utilize FFloat32 to access float components
        FFloat32 Float32(InFloat32);

        // Extract sign, exponent, and mantissa
        const uint32 InFloat32Sign     = Float32.Bits.Sign;
        const int32  InFloat32Exponent = static_cast<int32>(Float32.Bits.Exponent) - FP32_EXPONENT_BIAS;
        const uint32 InFloat32Mantissa = Float32.Bits.Mantissa;

        // Initialize encoded to zero
        Encoded = 0;

        // Set the sign bit
        Sign = static_cast<uint16>(InFloat32Sign);

        if (InFloat32Exponent == (FP32_MAX_EXPONENT - FP32_EXPONENT_BIAS)) // Exponent all ones: NaN or Infinity
        {
            Exponent = FP16_MAX_EXPONENT;
            Mantissa = (InFloat32Mantissa == 0) ? 0 : static_cast<uint16>((InFloat32Mantissa >> FP16_MANTISSA_SHIFT) | FP16_MANTISSA_MASK + 1);
        }
        else if (InFloat32Exponent > (FP16_EXPONENT_BIAS - 1)) // Overflow: Exponent exceeds half-precision range
        {
            // Set to infinity
            Exponent = FP16_MAX_EXPONENT;
            Mantissa = 0;
        }
        else if (InFloat32Exponent >= -14) // Normalized number
        {
            // Re-bias the exponent from float32 bias (127) to float16 bias (15)
            Exponent = static_cast<uint16>(InFloat32Exponent + FP16_EXPONENT_BIAS);
            // Shift mantissa to align with float16 mantissa (10 bits)
            Mantissa = static_cast<uint16>(InFloat32Mantissa >> FP16_MANTISSA_SHIFT);
        }
        else if (InFloat32Exponent >= -24) // Subnormal number
        {
            // Calculate the number of bits to shift to normalize the mantissa
            const int32  ExponentShift     = (-14) - InFloat32Exponent;
            const uint32 SubnormalMantissa = (InFloat32Mantissa | FP32_HIDDEN_BIT) >> (ExponentShift + FP16_MANTISSA_SHIFT);
            Exponent = 0;
            Mantissa = static_cast<uint16>(SubnormalMantissa);
        }
        else
        {
            // Underflow: too small to represent as subnormal half-precision float
            Exponent = 0;
            Mantissa = 0;
        }
    }

    /**
     * @brief Fast conversion from float32 to float16.
     * @param InFloat32 The 32-bit float to convert.
     * 
     * This function provides a faster conversion from float32 to float16 by directly adjusting the
     * exponent and mantissa without handling special cases. It is intended for use when performance
     * is critical and the input values are known to be within the normal range representable by float16.
     * 
     * **Limitations**:
     * - Does not handle NaN or infinity correctly.
     * - Does not handle denormalized numbers or zero correctly.
     * - Exponent underflow or overflow may occur without handling.
     */
    inline void SetFloatFast(float InFloat32)
    {
        FFloat32 Float32(InFloat32);

        // Calculate the new exponent by adjusting the bias
        const int32 NewExponent = static_cast<int32>(Float32.Bits.Exponent) - FP32_EXPONENT_BIAS + FP16_EXPONENT_BIAS;

        // Shift mantissa to fit into float16 mantissa bits
        const uint16 NewMantissa = static_cast<uint16>(Float32.Bits.Mantissa >> FP16_MANTISSA_SHIFT);

        // Assign the values directly
        Sign     = static_cast<uint16>(Float32.Bits.Sign);
        Exponent = static_cast<uint16>(NewExponent);
        Mantissa = NewMantissa;
    }

    /**
     * @brief Converts the half-precision float to a 32-bit float.
     * @return The 32-bit float representation.
     * 
     * This function converts the stored 16-bit half-precision float to a 32-bit single-precision float,
     * handling special cases and reconstructing the float32 representation.
     */
    inline float GetFloat() const
    {
        FFloat32 OutFloat32;
        if (Exponent == FP16_MAX_EXPONENT) // Exponent all ones: NaN or Infinity
        {
            OutFloat32.Bits.Sign     = Sign;
            OutFloat32.Bits.Exponent = FP32_MAX_EXPONENT;
            OutFloat32.Bits.Mantissa = (Mantissa != 0) ? (Mantissa << FP16_MANTISSA_SHIFT) : 0; // Preserve NaN payload
        }
        else if (Exponent == 0) // Zero or subnormal
        {
            if (Mantissa == 0)
            {
                // Zero
                OutFloat32.Bits.Sign     = Sign;
                OutFloat32.Bits.Exponent = 0;
                OutFloat32.Bits.Mantissa = 0;
            }
            else
            {
                // Subnormal number, Normalize the mantissa
                uint16 TempMantissa = Mantissa;
                int32  TempExponent = -14;
                while ((TempMantissa & (1 << (FP16_MANTISSA_BITS))) == 0)
                {
                    TempMantissa <<= 1;
                    TempExponent--;
                }

                TempMantissa &= (FP16_MANTISSA_MASK); // Remove the leading 1
                TempExponent++;                       // Adjust exponent
                TempExponent += FP32_EXPONENT_BIAS - FP16_EXPONENT_BIAS;

                OutFloat32.Bits.Sign     = Sign;
                OutFloat32.Bits.Exponent = static_cast<uint32>(TempExponent);
                OutFloat32.Bits.Mantissa = static_cast<uint32>(TempMantissa) << FP16_MANTISSA_SHIFT; // Align mantissa to float32
            }
        }
        else // Normalized number
        {
            const int32 AdjustedExponent = Exponent + FP32_EXPONENT_BIAS - FP16_EXPONENT_BIAS;
            OutFloat32.Bits.Sign     = Sign;
            OutFloat32.Bits.Exponent = static_cast<uint32>(AdjustedExponent);
            OutFloat32.Bits.Mantissa = static_cast<uint32>(Mantissa) << FP16_MANTISSA_SHIFT; // Align mantissa to float32
        }

        return OutFloat32.Float32;
    }

    /**
     * @brief Returns the raw encoded bits of the half-precision float.
     * @return The 16-bit encoded value.
     */
    FORCEINLINE uint16 GetBits() const
    {
        return Encoded;
    }

    /**
     * @brief Equality operator.
     * @param Other The right-hand side FFloat16 to compare with.
     * @return True if the encoded values are equal.
     */
    FORCEINLINE bool operator==(const FFloat16& Other) const
    {
        return Encoded == Other.Encoded;
    }

    /**
     * @brief Inequality operator.
     * @param Other The right-hand side FFloat16 to compare with.
     * @return True if the encoded values are not equal.
     */
    FORCEINLINE bool operator!=(const FFloat16& Other) const
    {
        return !(*this == Other);
    }

    /**
     * @brief Assignment operator from float.
     * @param InFloat32 The float value to assign.
     * @return Reference to this FFloat16 instance.
     */
    FORCEINLINE FFloat16& operator=(float InFloat32)
    {
        SetFloat(InFloat32);
        return *this;
    }

    /**
     * @brief Copy assignment operator.
     * @param Other The FFloat16 instance to copy from.
     * @return Reference to this FFloat16 instance.
     */
    FORCEINLINE FFloat16& operator=(const FFloat16& Other)
    {
        Encoded = Other.Encoded;
        return *this;
    }

public:

    /** Union for accessing the raw bits of the half-precision float */
    union
    {
        uint16 Encoded; /** @brief The raw bits of the half-precision float */
        struct
        {
            uint16 Mantissa : 10; /** @brief Mantissa (fractional part) */
            uint16 Exponent : 5;  /** @brief Exponent */
            uint16 Sign     : 1;  /** @brief Sign bit */
        };
    };
};

static_assert(sizeof(FFloat16) == sizeof(uint16), "FFloat16 should have the same size as a uint16");
MARK_AS_REALLOCATABLE(FFloat16);
