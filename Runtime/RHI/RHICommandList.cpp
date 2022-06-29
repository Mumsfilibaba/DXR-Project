#include "RHICommandList.h"

#include "Core/Debug/Profiler/FrameProfiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCommandAllocator

FCommandAllocator::FCommandAllocator(uint32 StartSize)
    : CurrentMemory(nullptr)
    , Size(StartSize)
    , Offset(0)
    , DiscardedMemory()
{
    CurrentMemory = reinterpret_cast<uint8*>(FMemory::Malloc(Size));
    Check(CurrentMemory != nullptr);

    AverageMemoryUsage = Size;
}

FCommandAllocator::~FCommandAllocator()
{
    ReleaseDiscardedMemory();

    SafeDelete(CurrentMemory);
}

void* FCommandAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment)
{
    Check(CurrentMemory != nullptr);

    const uint64 AlignedSize = NMath::AlignUp(SizeInBytes, Alignment);
    const uint64 NewOffset = Offset + AlignedSize;
    if (NewOffset <= Size)
    {
        void* Result = CurrentMemory + Offset;
        Offset = NewOffset;
        return Result;
    }
    else
    {
        // Discard the current pointer 
        DiscardedMemory.Emplace(CurrentMemory);

        // Allocate a new block of memory
        const uint64 NewSize = NMath::Max(Size + Size, AlignedSize);

        CurrentMemory = reinterpret_cast<uint8*>(FMemory::Malloc(NewSize));
        Check(CurrentMemory != nullptr);

        Size = NewSize;
        AverageMemoryUsage = Size;
        Offset = AlignedSize;

        // Return the newly allocated block
        return CurrentMemory;
    }
}

void FCommandAllocator::Reset()
{
    ReleaseDiscardedMemory();

    // Moving average for the memory usage
    const float Alpha = 0.2f;
    AverageMemoryUsage = uint64(Offset * Alpha) + uint64((1.0f - Alpha) * AverageMemoryUsage);

    // Resize if to much memory is used 
    const uint64 SlackSize = Size - AverageMemoryUsage;
    if (NMemoryUtils::BytesToMegaBytes(SlackSize) > 1)
    {
        SafeDelete(CurrentMemory);

        const uint64 NewSize = AverageMemoryUsage + NMemoryUtils::MegaBytesToBytes(1);

        CurrentMemory = reinterpret_cast<uint8*>(FMemory::Malloc(NewSize));
        Check(CurrentMemory != nullptr);

        Size = NewSize;
        AverageMemoryUsage = Size;
    }

    // Reset
    Offset = 0;
}

void FCommandAllocator::ReleaseDiscardedMemory()
{
    for (uint8* Memory : DiscardedMemory)
    {
        SafeDelete(Memory);
    }

    DiscardedMemory.MakeEmpty();
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandQueue

FRHICommandQueue FRHICommandQueue::Instance;

FRHICommandQueue& FRHICommandQueue::Get()
{
    return Instance;
}

FRHICommandQueue::FRHICommandQueue()
    : CmdContext(nullptr)
    , NumDrawCalls(0)
    , NumDispatchCalls(0)
    , NumCommands(0)
{
}

void FRHICommandQueue::ExecuteCommandList(FRHICommandList& CmdList)
{
    // Execute
    GetContext().StartContext();

    {
        TRACE_FUNCTION_SCOPE();

        ResetStatistics();

        InternalExecuteCommandList(CmdList);
    }

    GetContext().FinishContext();
}

void FRHICommandQueue::ExecuteCommandLists(FRHICommandList* const* CmdLists, uint32 NumCmdLists)
{
    // Execute
    GetContext().StartContext();

    {
        TRACE_FUNCTION_SCOPE();

        ResetStatistics();

        for (uint32 i = 0; i < NumCmdLists; i++)
        {
            FRHICommandList* CurrentCmdList = CmdLists[i];
            InternalExecuteCommandList(*CurrentCmdList);
        }
    }

    GetContext().FinishContext();
}

void FRHICommandQueue::WaitForGPU()
{
    if (CmdContext)
    {
        CmdContext->Flush();
    }
}

void FRHICommandQueue::InternalExecuteCommandList(FRHICommandList& CmdList)
{
    if (CmdList.FirstCommand)
    {
        FRHICommand* CurrentCommand = CmdList.FirstCommand;
        while (CurrentCommand != nullptr)
        {
            FRHICommand* PreviousCommand = CurrentCommand;
            CurrentCommand = CurrentCommand->NextCommand;
            PreviousCommand->ExecuteAndRelease(GetContext());
        }

        NumDrawCalls     += CmdList.GetNumDrawCalls();
        NumDispatchCalls += CmdList.GetNumDispatchCalls();
        NumCommands      += CmdList.GetNumCommands();

        CmdList.FirstCommand = nullptr;
        CmdList.LastCommand  = nullptr;
        CmdList.Reset();
    }
}
