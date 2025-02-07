#include "RHI/RHIStats.h"

RHI_API FAtomicUInt64 GRHINumDrawCalls     = 0;
RHI_API FAtomicUInt64 GRHINumDispatchCalls = 0;
RHI_API FAtomicUInt64 GRHINumCommands      = 0;