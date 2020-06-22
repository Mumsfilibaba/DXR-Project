#pragma once

// Macro for deleting objects safley
#define SAFEDELETE(InOutObject)  if ((InOutObject)) { delete (InOutObject); (InOutObject) = nullptr; }

// Zero memory
#define ZERO_MEMORY(InOutObject, InSizeInBytes) memset(InOutObject, 0, InSizeInBytes)

// Forceinline
#define FORCEINLINE __forceinline

// Validate (a.k.a ASSERT)
#define VALIDATE(InCondition) assert(InCondition)

// Helpers for creating bitmasks
#define BIT(Bit)	(1 << Bit)
#define FLAG(Bit)	BIT(Bit)