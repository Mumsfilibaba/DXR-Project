#pragma once
#include "Defines.h"
#include "Types.h"

/*
* Float64
*/

struct Float64
{
	FORCEINLINE Float64()
		: Float(0.0)
	{
	}

	FORCEINLINE Float64(Double F64)
		: Float(F64)
	{
	}

	FORCEINLINE Float64(const Float64& Other)
		: Float(Other.Float)
	{
	}

	FORCEINLINE void SetFloat(Double F64)
	{
		Float = F64;
	}

	FORCEINLINE Double GetFloat() const
	{
		return Float;
	}

	FORCEINLINE Float64& operator=(Double F64)
	{
		Float = F64;
		return *this;
	}

	FORCEINLINE Float64& operator=(const Float64& Other)
	{
		Float = Other.Float;
		return *this;
	}

	union
	{
		Double	Float;
		UInt64	Encoded;
		struct
		{
			UInt64 Mantissa : 52;
			UInt64 Exponent	: 11;
			UInt64 Sign		: 1;
		};
	};
};

/*
* Float32
*/

struct Float32
{
	FORCEINLINE Float32()
		: Float(0.0f)
	{
	}

	FORCEINLINE Float32(Float F32)
		: Float(F32)
	{
	}

	FORCEINLINE Float32(const Float32& Other)
		: Float(Other.Float)
	{
	}

	FORCEINLINE void SetFloat(Float F32)
	{
		Float = F32;
	}

	FORCEINLINE Float GetFloat() const
	{
		return Float;
	}

	FORCEINLINE Float32& operator=(Float F32)
	{
		Float = F32;
		return *this;
	}

	FORCEINLINE Float32& operator=(const Float32& Other)
	{
		Float = Other.Float;
		return *this;
	}

	union
	{
		Float	Float;
		UInt32	Encoded;
		struct
		{
			UInt32 Mantissa	: 23;
			UInt32 Exponent	: 8;
			UInt32 Sign		: 1;
		};
	};
};

/*
* Float16
*/

struct Float16
{
	FORCEINLINE Float16()
		: Encoded(0)
	{
	}

	FORCEINLINE Float16(Float F32)
		: Encoded(0)
	{
		SetFloat(F32);
	}

	FORCEINLINE Float16(const Float16& Other)
		: Encoded(Other.Encoded)
	{
	}

	FORCEINLINE void SetFloat(Float F32)
	{
		const Float32 In(F32);
		Sign = In.Sign;
		
		// Adjust E for exponent bias difference between 16-bit and 32-bit
		const UInt32 E = In.Exponent - (127u - 15u);
		
		// Round mantissa
		const UInt32 RoundedMantissa = In.Mantissa + ((In.Mantissa & 0x00001000) << 1);

	}

	FORCEINLINE Float GetFloat() const
	{
		Float32 Ret;
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
			UInt16 Mantissa	: 10;
			UInt16 Exponent	: 5;
			UInt16 Sign		: 1;
		};
	};
};