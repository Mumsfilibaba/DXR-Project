#include "Core/Tasks/TaskGraphStats.h"

STAT_DEFINE_COUNTER(STAT_TaskGraph_AnyThreadPending,       "AnyThread Pending",        "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_MainThreadPending,      "MainThread Pending",       "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_RenderThreadPending,    "RenderThread Pending",     "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_RHIThreadPending,       "RHIThread Pending",        "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_AnyThreadActiveWorkers, "AnyThread Active Workers", "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_TotalWorkers,           "Total Workers",            "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_TasksLaunched,          "Tasks Launched",           "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_TasksCompleted,         "Tasks Completed",          "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_StealAttempts,          "Steal Attempts",           "Task Graph");
STAT_DEFINE_COUNTER(STAT_TaskGraph_StealSuccesses,         "Steal Successes",          "Task Graph");
