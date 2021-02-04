#pragma once
#include "Core.h"

struct Float64
{
    FORCEINLINE Float64()
        : Float(0.0)
    {
    }

    FORCEINLINE Float64(Double Fp64)
        : Float(Fp64)
    {
    }

    FORCEINLINE Float64(const Float64& Other)
        : Float(Other.Float)
    {
    }

    FORCEINLINE void SetFloat(Double FP64) { Float = FP64; }

    FORCEINLINE Double GetFloat() const { return Float; }

    FORCEINLINE Float64& operator=(Double Fp64)
    {
        Float = Fp64;
        return *this;
    }

    FORCEINLINE Float64& operator=(const Float64& Other)
    {
        Float = Other.Float;
        return *this;
    }

    union
    {
        Double Float;
        UInt64 Encoded;
        struct
        {
            UInt64 Mantissa : 52;
            UInt64 Exponent : 11;
            UInt64 Sign     : 1;
        };
    };
};

struct Float32
{
    FORCEINLINE Float32()
        : Float(0.0f)
    {
    }

    FORCEINLINE Float32(Float Fp32)
        : Float(Fp32)
    {
    }

    FORCEINLINE Float32(const Float32& Other)
        : Float(Other.Float)
    {
    }

    FORCEINLINE void SetFloat(Float FP32) { Float = FP32; }

    FORCEINLINE Float GetFloat() const { return Float; }

    FORCEINLINE Float32& operator=(Float Fp32)
    {
        Float = Fp32;
        return *this;
    }

    FORCEINLINE Float32& operator=(const Float32& Other)
    {
        Float = Other.Float;
        return *this;
    }

    union
    {
        Float  Float;
        UInt32 Encoded;
        struct
        {
            UInt32 Mantissa : 23;
            UInt32 Exponent : 8;
            UInt32 Sign     : 1;
        };
    };
};

struct Float16
{
    FORCEINLINE Float16()
        : Encoded(0)
    {
    }

    FORCEINLINE Float16(Float Fp32)
        : Encoded(0)
    {
        SetFloat(Fp32);
    }

    FORCEINLINE Float16(const Float16& Other)
        : Encoded(Other.Encoded)
    {
    }

    FORCEINLINE void SetFloat(Float Fp32)
    {
        // Constant masks
        constexpr UInt32 FP32_HIDDEN_BIT   = 0x800000U;
        constexpr UInt16 FP16_MAX_EXPONENT = 0x1f;
        constexpr UInt32 MAX_EXPONENT      = FP16_MAX_EXPONENT + 127 - 15;
        constexpr UInt32 DENORM_EXPONENT   = (127 - 15);
        constexpr UInt32 MIN_EXPONENT      = DENORM_EXPONENT - 10;

        // Convert
        const Float32 In(Fp32);
        Sign = In.Sign;

        // This value is to large to be represented with Fp16 (Alt. Infinity or NaN)
        if (In.Exponent >= MAX_EXPONENT)
        {
            // Set mantissa to NaN if these bit are set otherwise Infinity
            constexpr UInt32 SIGN_EXLUDE_MASK = 0x7fffffff;
            const UInt32 InEncoded = In.Encoded & SIGN_EXLUDE_MASK;
            Mantissa = (InEncoded > 0x7F800000) ? (0x200 | (In.Mantissa & 0x3ffu)) : 0u;
            Exponent = FP16_MAX_EXPONENT;
        }
        else if (In.Exponent <= MIN_EXPONENT)
        {
            // These values are too small to be represented by Fp16, these values reults in +/- zero
            Exponent = 0;
            Mantissa = 0;
        }
        else if (In.Exponent <= DENORM_EXPONENT)
        {
            // Calculate new mantissa with hidden bit
            const UInt32 NewMantissa = FP32_HIDDEN_BIT | In.Mantissa;
            const UInt32 Shift       = 125u - In.Exponent; // Calculate how much to shift to go from normalized to demormalized
            
            Exponent = 0;
            Mantissa = NewMantissa >> (Shift + 1);

            // Check for rounding and add one
            if ((NewMantissa & ((1u << Shift) - 1)))
            {
                Encoded++;
            }
        }
        else
        {
            // All other values
            const Int32 NewExponent = Int32(In.Exponent) - 127 + 15; // Unbias and bias the exponents
            Exponent = UInt16(NewExponent);
            
            const Int32 NewMantissa = Int32(In.Mantissa) >> 13; // Bit-Shift diff in number of mantissa bits
            Mantissa = UInt16(NewMantissa);
        }
    }

    FORCEINLINE void SetFloatFast(Float Fp32)
    {
        Float32 In(Fp32);
        Exponent = UInt16(Int32(In.Exponent) - 127 + 15); // Unbias and bias the exponents
        Mantissa = UInt16(In.Mantissa >> 13);             // Bit-Shift diff in number of mantissa bits
        Sign     = In.Sign;
    }

    FORCEINLINE Float GetFloat() const
    {
        constexpr UInt32 FP32_MAX_EXPONENT = 0xff;
        constexpr UInt16 FP16_MAX_EXPONENT = 0x1f;

        Float32 Ret;
        Ret.Sign = Sign;

        // Infinity/NaN
        if (Exponent == FP16_MAX_EXPONENT)
        {
            Ret.Exponent = FP32_MAX_EXPONENT;
            Ret.Mantissa = (Mantissa != 0) ? (Mantissa << 13) : 0u;
        }
        else if (Exponent == 0)
        {
            if (Mantissa == 0)
            {
                // Zero
                Ret.Exponent = 0;
                Ret.Mantissa = 0;
            }
            else
            {
                // Denormalized
                const UInt32 Shift = 10 - UInt32(log2((Float)Mantissa));
                Ret.Exponent = 127 - 14 - Shift;
                Ret.Mantissa = Mantissa << (Shift + 13);
            }
        }
        else
        {
            // All other values
            const Int32 NewExponent = Int32(Exponent) - 15 + 127; // Unbias and bias the exponents
            Ret.Exponent = UInt32(NewExponent);

            const Int32 NewMantissa = Int32(Mantissa) << 13; // Bit-Shift diff in number of mantissa bits
            Ret.Mantissa = UInt32(NewMantissa);
        }

        return Ret.Float;
    }

    FORCEINLINE Float16& operator=(Float F32)
    {
        SetFloat(F32);
        return *this;
    }

    FORCEINLINE Float16& operator=(const Float16& Other)
    {
        Encoded = Other.Encoded;
        return *this;
    }

    union
    {
        UInt16 Encoded;
        struct
        {
            UInt16 Mantissa : 10;
            UInt16 Exponent : 5;
            UInt16 Sign     : 1;
        };
    };
};