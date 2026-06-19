#pragma once
#include "Core/Stats/Stats.h"

STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_AnyThreadPending);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_MainThreadPending);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_RenderThreadPending);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_RHIThreadPending);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_AnyThreadActiveWorkers);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_TotalWorkers);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_TasksLaunched);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_TasksCompleted);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_StealAttempts);
STAT_DECLARE_EXTERN(CORE_API, STAT_TaskGraph_StealSuccesses);
