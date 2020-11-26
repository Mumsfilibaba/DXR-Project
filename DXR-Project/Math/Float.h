#pragma once
#include "Defines.h"
#include "Types.h"

/*
* Float64
*/

struct Float64
{
	inline Float64()
		: Float(0.0f)
	{
	}

	inline Float64(double F64)
		: Float(F64)
	{
	}

	FORCEINLINE void Set(double F64)
	{
		Float = F64;
	}

	FORCEINLINE double Get() const
	{
		return Float;
	}

	FORCEINLINE Float64& operator=(double F64)
	{
		Set(F64);
		return *this;
	}

	union
	{
		double	Float;
		uint64	Encoded;
		struct
		{
			uint64 Mantissa : 52;
			uint64 Exponent	: 11;
			uint64 Sign		: 1;
		};
	};
};

/*
* Float32
*/

struct Float32
{
	inline Float32()
		: Float(0.0f)
	{
	}

	inline Float32(float F32)
		: Float(F32)
	{
	}

	FORCEINLINE void Set(float F32)
	{
		Float = F32;
	}

	FORCEINLINE float Get() const
	{
		return Float;
	}

	FORCEINLINE Float32& operator=(float F32)
	{
		Set(F32);
		return *this;
	}

	union
	{
		float	Float;
		uint32	Encoded;
		struct
		{
			uint32 Mantissa	: 23;
			uint32 Exponent	: 8;
			uint32 Sign		: 1;
		};
	};
};

/*
* Float16
*/

struct Float16
{
	inline Float16()
		: Encoded(0)
	{
	}

	inline Float16(float F32)
		: Encoded(0)
	{
		Set(F32);
	}

	FORCEINLINE void Set(float F32)
	{
		Float32 In(F32);
		Sign = In.Sign;
		
		if (In.Exponent == 0)
		{
			// Handle Zero
			Exponent = 0;
			Mantissa = 0;
		}
		else if (In.Exponent == 0xff)
		{
			// Handle infinity or NaN
			Exponent = 31;
			Mantissa = (In.Mantissa != 0) ? 1 : 0;
		}
		else
		{
			// Other

		}
	}

	FORCEINLINE float Get() const
	{
		Float32 Ret;
		return Ret.Float;
	}

	FORCEINLINE Float16& operator=(float F32)
	{
		Set(F32);
		return *this;
	}

	union
	{
		uint16 Encoded;
		struct
		{
			uint16 Mantissa	: 10;
			uint16 Exponent	: 5;
			uint16 Sign		: 1;
		};
	};
};