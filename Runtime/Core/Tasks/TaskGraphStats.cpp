#include "Core/Tasks/TaskGraphStats.h"

STAT_DEFINE_COUNTER(STAT_TaskGraph_AnyThreadPending,       TEXT("AnyThread Pending"),        TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_MainThreadPending,      TEXT("MainThread Pending"),       TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_RenderThreadPending,    TEXT("RenderThread Pending"),     TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_RHIThreadPending,       TEXT("RHIThread Pending"),        TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_AnyThreadActiveWorkers, TEXT("AnyThread Active Workers"), TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_TotalWorkers,           TEXT("Total Workers"),            TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_TasksLaunched,          TEXT("Tasks Launched"),           TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_TasksCompleted,         TEXT("Tasks Completed"),          TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_StealAttempts,          TEXT("Steal Attempts"),           TEXT("Task Graph"));
STAT_DEFINE_COUNTER(STAT_TaskGraph_StealSuccesses,         TEXT("Steal Successes"),          TEXT("Task Graph"));
