#pragma once
#include "Core/Tasks/Tasks.h"
#include "Core/Tasks/TaskGraph.h"
#include "Core/Containers/Array.h"
#include "Core/Math/Math.h"

template<typename FuncType>
void Tasks::ParallelForRange(int32 Count, FuncType&& BodyRange, int32 MinPerJob, ETaskPriority::Type Priority)
{
    if (Count <= 0)
    {
        return;
    }

    const int32 NumWorkers    = FTaskGraph::IsInitialized() ? FTaskGraph::Get().GetNumAnyThreadWorkers() : 1;
    const int32 PerJob        = Math::Max(MinPerJob, 1);
    const int32 NumChunks     = Math::Clamp(Count / PerJob, 1, Math::Max(NumWorkers, 1));
    const int32 BaseChunkSize = Count / NumChunks;
    const int32 Remainder     = Count % NumChunks;

    // Launch all chunks except the first as tasks; the caller runs the first chunk itself so it is never idle while waiting.
    TArray<FTaskHandle> Handles;
    if (NumChunks > 1)
    {
        Handles.Reserve(NumChunks - 1);
    }

    int32 Begin = BaseChunkSize + (Remainder > 0 ? 1 : 0);
    for (int32 ChunkIndex = 1; ChunkIndex < NumChunks; ++ChunkIndex)
    {
        const int32 ChunkSize = BaseChunkSize + (ChunkIndex < Remainder ? 1 : 0);
        const int32 End       = Begin + ChunkSize;

        Handles.Add(Tasks::Launch("ParallelFor",
            [&BodyRange, Begin, End]()
            {
                BodyRange(Begin, End);
            }, ENamedThread::AnyThread, Priority));

        Begin = End;
    }

    // The caller runs the first chunk inline.
    const int32 FirstChunkSize = BaseChunkSize + (Remainder > 0 ? 1 : 0);
    BodyRange(0, FirstChunkSize);

    if (!Handles.IsEmpty())
    {
        Tasks::Wait(Handles);
    }
}

template<typename FuncType>
void Tasks::ParallelFor(int32 Count, FuncType&& Body, int32 MinPerJob, ETaskPriority::Type Priority)
{
    Tasks::ParallelForRange(Count,
        [&Body](int32 Begin, int32 End)
        {
            for (int32 Index = Begin; Index < End; ++Index)
            {
                Body(Index);
            }
        }, MinPerJob, Priority);
}
