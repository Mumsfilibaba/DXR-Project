#pragma once
#include "Core.h"

#ifdef InterlockedAdd
#undef InterlockedAdd
#endif

#ifdef InterlockedIncrement
#undef InterlockedIncrement
#endif

#ifdef InterlockedDecrement
#undef InterlockedDecrement
#endif

#ifdef InterlockedCompareExchange
#undef InterlockedCompareExchange
#endif

#ifdef InterlockedExchange
#undef InterlockedExchange
#endif

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class GenericAtomic
{
public:
    FORCEINLINE static int32 InterlockedIncrement( volatile int32* Dest )
    {
        return 0;
    }
    FORCEINLINE static int64 InterlockedIncrement( volatile int64* Dest )
    {
        return 0;
    }

    FORCEINLINE static int32 InterlockedDecrement( volatile int32* Dest )
    {
        return 0;
    }
    FORCEINLINE static int64 InterlockedDecrement( volatile int64* Dest )
    {
        return 0;
    }

    FORCEINLINE static int32 InterlockedAdd( volatile int32* Dest, int32 Value )
    {
        return 0;
    }
    FORCEINLINE static int64 InterlockedAdd( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    FORCEINLINE static int32 InterlockedSub( volatile int32* Dest, int32 Value )
    {
        return 0;
    }
    FORCEINLINE static int64 InterlockedSub( volatile int64* Dest, int64 Value )
    {
        return 0;
    }

    FORCEINLINE static int32 InterlockedCompareExchange( volatile int32* Dest, int32 ExChange, int32 Comparand )
    {
        return 0;
    }
    FORCEINLINE static int64 InterlockedCompareExchange( volatile int64* Dest, int64 ExChange, int64 Comparand )
    {
        return 0;
    }

    FORCEINLINE static int32 InterlockedExchange( volatile int32* Dest, int32 ExChange )
    {
        return 0;
    }
    FORCEINLINE static int64 InterlockedExchange( volatile int64* Dest, int64 ExChange )
    {
        return 0;
    }
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif