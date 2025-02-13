#include "RHI/RHIStats.h"

RHI_API FAtomicUInt64 FRHIStats::NumDrawCalls     = 0;
RHI_API FAtomicUInt64 FRHIStats::NumDispatchCalls = 0;
RHI_API FAtomicUInt64 FRHIStats::NumCommands      = 0;