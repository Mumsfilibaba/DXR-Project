#pragma once
#include "Core.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CPlatformAtomic
{
public:

    // Read: Perform a atomic read. All loads and stores are synched.

    static FORCEINLINE int8  Read( volatile const int8*  Source ) { return 0; }
    static FORCEINLINE int16 Read( volatile const int16* Source ) { return 0; }
    static FORCEINLINE int32 Read( volatile const int32* Source ) { return 0; }
    static FORCEINLINE int64 Read( volatile const int64* Source ) { return 0; }

    // RelaxedRead: Performs a releaxed atomic read. No barriers or synchronization takes place. Only guaranteed to be atomic.

    static FORCEINLINE int8  RelaxedRead( volatile const int8*  Source ) { return 0; }
    static FORCEINLINE int16 RelaxedRead( volatile const int16* Source ) { return 0; }
    static FORCEINLINE int32 RelaxedRead( volatile const int32* Source ) { return 0; }
    static FORCEINLINE int64 RelaxedRead( volatile const int64* Source ) { return 0; }

    // Store: Perform a store. All loads and stores are synched.

    static FORCEINLINE void Store( volatile const int8*  Dest, int8  Value ) {}
    static FORCEINLINE void Store( volatile const int16* Dest, int16 Value ) {}
    static FORCEINLINE void Store( volatile const int32* Dest, int32 Value ) {}
    static FORCEINLINE void Store( volatile const int64* Dest, int64 Value ) {}

    // RelaxedStore: Performs a releaxed atomic store. No barriers or synchronization takes place. Only guaranteed to be atomic.

    static FORCEINLINE void RelaxedStore( volatile const int8*  Dest, int8  Value ) {}
    static FORCEINLINE void RelaxedStore( volatile const int16* Dest, int16 Value ) {}
    static FORCEINLINE void RelaxedStore( volatile const int32* Dest, int32 Value ) {}
    static FORCEINLINE void RelaxedStore( volatile const int64* Dest, int64 Value ) {}
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif