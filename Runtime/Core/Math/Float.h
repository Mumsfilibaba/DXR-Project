#pragma once
#include "MathCommon.h"

#include <cfloat>

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4556) // Disable "value of intrinsic immediate argument '8' is out of range '0 - 7'"
#endif

// Constant masks
#define FP32_HIDDEN_BIT   (0x800000U)
#define FP32_MAX_EXPONENT (0xff)
#define FP16_MAX_EXPONENT (0x1f)
#define MAX_EXPONENT      (FP16_MAX_EXPONENT + 127 - 15)
#define DENORM_EXPONENT   (127 - 15)
#define MIN_EXPONENT      (DENORM_EXPONENT - 10)

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SFloat64
{
    FORCEINLINE SFloat64()
        : Float64( 0.0 )
    {
    }

    FORCEINLINE SFloat64( double InFloat64 )
        : Float64( InFloat64 )
    {
    }

    FORCEINLINE SFloat64( const SFloat64& Other )
        : Float64( Other.Float64 )
    {
    }

    FORCEINLINE void SetFloat( double InFloat64 )
    {
        Float64 = InFloat64;
    }

    FORCEINLINE double GetFloat() const
    {
        return Float64;
    }

    FORCEINLINE SFloat64& operator=( double InFloat64 )
    {
        Float64 = InFloat64;
        return *this;
    }

    FORCEINLINE SFloat64& operator=( const SFloat64& Other )
    {
        Float64 = Other.Float64;
        return *this;
    }

    union
    {
        double Float64;
        uint64 Encoded;
        struct
        {
            uint64 Mantissa : 52;
            uint64 Exponent : 11;
            uint64 Sign : 1;
        };
    };
};

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SFloat32
{
    FORCEINLINE SFloat32()
        : Float32( 0.0f )
    {
    }

    FORCEINLINE SFloat32( float InFloat32 )
        : Float32( InFloat32 )
    {
    }

    FORCEINLINE SFloat32( const SFloat32& Other )
        : Float32( Other.Float32 )
    {
    }

    FORCEINLINE void SetFloat( float InFloat32 )
    {
        Float32 = InFloat32;
    }

    FORCEINLINE float GetFloat() const
    {
        return Float32;
    }

    FORCEINLINE SFloat32& operator=( float InFloat32 )
    {
        Float32 = InFloat32;
        return *this;
    }

    FORCEINLINE SFloat32& operator=( const SFloat32& Other )
    {
        Float32 = Other.Float32;
        return *this;
    }

    union
    {
        float  Float32;
        uint32 Encoded;
        struct
        {
            uint32 Mantissa : 23;
            uint32 Exponent : 8;
            uint32 Sign : 1;
        };
    };
};

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SFloat16
{
    FORCEINLINE SFloat16()
        : Encoded( 0 )
    {
    }

    FORCEINLINE SFloat16( float Float32 )
        : Encoded( 0 )
    {
        SetFloat( Float32 );
    }

    FORCEINLINE SFloat16( const SFloat16& Other )
        : Encoded( Other.Encoded )
    {
    }

    FORCEINLINE void SetFloat( float Float32 )
    {
    #if ARCHITECTURE_X86_X64 && !PLATFORM_MACOS
        __m128  Reg0 = _mm_set_ss( Float32 );
        __m128i Reg1 = _mm_cvtps_ph( Reg0, _MM_FROUND_NO_EXC );
        Encoded = static_cast<uint16>(_mm_cvtsi128_si32( Reg1 ));
    #else
        // Convert
        const SFloat32 In( Float32 );
        Sign = In.Sign;

        // This value is to large to be represented with Fp16 (Alt. Infinity or NaN)
        if ( In.Exponent >= MAX_EXPONENT )
        {
            // Set mantissa to NaN if these bit are set otherwise Infinity
            constexpr uint32 SIGN_EXLUDE_MASK = 0x7fffffff;
            const uint32 InEncoded = (In.Encoded & SIGN_EXLUDE_MASK);
            Mantissa = (InEncoded > 0x7F800000) ? (0x200 | (In.Mantissa & 0x3ffu)) : 0u;
            Exponent = FP16_MAX_EXPONENT;
        }
        else if ( In.Exponent <= MIN_EXPONENT )
        {
            // These values are too small to be represented by Fp16, these values results in +/- zero
            Exponent = 0;
            Mantissa = 0;
        }
        else if ( In.Exponent <= DENORM_EXPONENT )
        {
            // Calculate new mantissa with hidden bit
            const uint32 NewMantissa = FP32_HIDDEN_BIT | In.Mantissa;
            const uint32 Shift = 125u - In.Exponent; // Calculate how much to shift to go from normalized to de-normalized

            Exponent = 0;
            Mantissa = NewMantissa >> (Shift + 1);

            // Check for rounding and add one
            if ( (NewMantissa & ((1u << Shift) - 1)) )
            {
                Encoded++;
            }
        }
        else
        {
            // All other values
            const int32 NewExponent = int32( In.Exponent ) - 127 + 15; // Unbias and bias the exponents
            Exponent = uint16( NewExponent );

            const int32 NewMantissa = int32( In.Mantissa ) >> 13; // Bit-Shift diff in number of mantissa bits
            Mantissa = uint16( NewMantissa );
        }
    #endif
    }

    FORCEINLINE void SetFloatFast( float Float32 )
    {
    #if ARCHITECTURE_X86_X64
        __m128  Reg0 = _mm_set_ss( Float32 );
        __m128i Reg1 = _mm_cvtps_ph( Reg0, _MM_FROUND_NO_EXC );
        Encoded = static_cast<uint16>(_mm_cvtsi128_si32( Reg1 ));
    #else
        SFloat32 In( Float32 );
        Exponent = uint16( int32( In.Exponent ) - 127 + 15 ); // Unbias and bias the exponents
        Mantissa = uint16( In.Mantissa >> 13 );               // Bit-Shift diff in number of mantissa bits
        Sign = In.Sign;
    #endif
    }

    FORCEINLINE float GetFloat() const
    {
    #if ARCHITECTURE_X86_X64
        __m128i Reg0 = _mm_cvtsi32_si128( static_cast<uint32_t>(Encoded) );
        __m128  Reg1 = _mm_cvtph_ps( Reg0 );
        return _mm_cvtss_f32( Reg1 );
    #else
        SFloat32 Ret;
        Ret.Sign = Sign;

        // Infinity/NaN
        if ( Exponent == FP16_MAX_EXPONENT )
        {
            Ret.Exponent = FP32_MAX_EXPONENT;
            Ret.Mantissa = (Mantissa != 0) ? (Mantissa << 13) : 0u;
        }
        else if ( Exponent == 0 )
        {
            if ( Mantissa == 0 )
            {
                // Zero
                Ret.Exponent = 0;
                Ret.Mantissa = 0;
            }
            else
            {
                // De-normalized
                const uint32 Shift = 10 - uint32( std::log2( (float)Mantissa ) );
                Ret.Exponent = 127 - 14 - Shift;
                Ret.Mantissa = Mantissa << (Shift + 13);
            }
        }
        else
        {
            // All other values
            const int32 NewExponent = int32( Exponent ) - 15 + 127; // Unbias and bias the exponents
            Ret.Exponent = uint32( NewExponent );

            const int32 NewMantissa = int32( Mantissa ) << 13; // Bit-Shift diff in number of mantissa bits
            Ret.Mantissa = uint32( NewMantissa );
        }

        return Ret.Float32;
    #endif
    }

    FORCEINLINE SFloat16& operator=( float F32 )
    {
        SetFloat( F32 );
        return *this;
    }

    FORCEINLINE SFloat16& operator=( const SFloat16& Other )
    {
        Encoded = Other.Encoded;
        return *this;
    }

    union
    {
        uint16 Encoded;
        struct
        {
            uint16 Mantissa : 10;
            uint16 Exponent : 5;
            uint16 Sign : 1;
        };
    };
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif