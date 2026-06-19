#pragma once
#include "Core/Core.h"

struct ENamedThread
{
    enum Type : uint8
    {
        AnyThread = 0,
        MainThread,
        RenderThread,
        RHIThread,
        Count
    };
};

struct ETaskPriority
{
    enum Type : uint8
    {
        High = 0,
        Normal,
        Low,
        Count
    };
};

class FTaskEvent;
class FGraphTask;
class FTaskWorker;
class FTaskGraph;
