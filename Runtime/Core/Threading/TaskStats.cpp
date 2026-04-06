#include "Core/Threading/TaskStats.h"

STAT_DEFINE_COUNTER(STAT_Task_PendingTasks,  "Pending Tasks",  "Task System");
STAT_DEFINE_COUNTER(STAT_Task_ActiveWorkers, "Active Workers", "Task System");
STAT_DEFINE_COUNTER(STAT_Task_TotalWorkers,  "Total Workers",  "Task System");
