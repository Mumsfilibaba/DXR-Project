#pragma once

// Macro for deleting objects safley
#define SAFEDELETE(OutObject)  if ((OutObject)) { delete (OutObject); (OutObject) = nullptr; }

// Validate (a.k.a ASSERT)
#define VALIDATE(Condition) assert(Condition)

/*
* Forceinline
*/
#ifdef COMPILER_VISUAL_STUDIO
	#define FORCEINLINE __forceinline
#else
	#define FORCEINLINE __attribute__((always_inline)) inline
#endif

/*
* Bit-Mask helpers
*/
#define BIT(Bit)	(1 << Bit)
#define FLAG(Bit)	BIT(Bit)

/*
* Unused params
*/
#define UNREFERENCED_VARIABLE(Variable) (void)(Variable)

/*
* String preprocessor handling
*   There are two versions of PREPROCESS_CONCAT, this is so that you can use __LINE__, __FILE__ etc. within the macro,
*   therefore always use PREPROCESS_CONCAT
*/
#define _PREPROCESS_CONCAT(x, y) x##y
#define PREPROCESS_CONCAT(x, y) _PREPROCESS_CONCAT(x, y)