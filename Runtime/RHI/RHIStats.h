#pragma once
#include "Core/Threading/Atomic/AtomicInt.h"

struct FRHIStats
{
    static RHI_API FAtomicUInt64 NumDrawCalls;
    static RHI_API FAtomicUInt64 NumDispatchCalls;
    static RHI_API FAtomicUInt64 NumCommands;
};

