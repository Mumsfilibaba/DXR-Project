#include "RHICommandList.h"

#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

RHI_API FRHICommandListExecutor GRHICommandListExecutor;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandAllocator

FRHICommandAllocator::FRHICommandAllocator(uint32 StartSize)
    : CurrentMemory(nullptr)
    , Size(StartSize)
    , Offset(0)
    , DiscardedMemory()
{
    CurrentMemory = reinterpret_cast<uint8*>(FMemory::Malloc(Size));
    Check(CurrentMemory != nullptr);

    AverageMemoryUsage = Size;
}

FRHICommandAllocator::~FRHICommandAllocator()
{
    ReleaseDiscardedMemory();

    SafeDelete(CurrentMemory);
}

void* FRHICommandAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment)
{
    Check(CurrentMemory != nullptr);

    const uint64 AlignedSize = NMath::AlignUp(SizeInBytes, Alignment);
    const uint64 NewOffset   = Offset + AlignedSize;
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

        Size               = NewSize;
        AverageMemoryUsage = Size;
        Offset             = AlignedSize;

        // Return the newly allocated block
        return CurrentMemory;
    }
}

void FRHICommandAllocator::Reset()
{
    ReleaseDiscardedMemory();

    // Moving average for the memory usage
    const float Alpha  = 0.2f;
    AverageMemoryUsage = uint64(Offset * Alpha) + uint64((1.0f - Alpha) * AverageMemoryUsage);

    // Resize if to much memory is used 
    const uint64 SlackSize = Size - AverageMemoryUsage;
    if (NMemoryUtils::BytesToMegaBytes(SlackSize) > 1)
    {
        SafeDelete(CurrentMemory);

        const uint64 NewSize = AverageMemoryUsage + NMemoryUtils::MegaBytesToBytes(1);

        CurrentMemory = reinterpret_cast<uint8*>(FMemory::Malloc(NewSize));
        Check(CurrentMemory != nullptr);

        Size               = NewSize;
        AverageMemoryUsage = Size;
    }

    // Reset
    Offset = 0;
}

void FRHICommandAllocator::ReleaseDiscardedMemory()
{
    for (uint8* Memory : DiscardedMemory)
    {
        SafeDelete(Memory);
    }

    DiscardedMemory.MakeEmpty();
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIExecutorThread

FRHIExecutorThread::FRHIExecutorThread()
    : Thread(nullptr)
    , WaitCS()
    , WaitCondition()
    , bIsRunning(false)
    , bIsExecuting(false)
{ }

bool FRHIExecutorThread::Start()
{
    TFunction<void()> ThreadFunction = Bind(&FRHIExecutorThread::Worker, this);

    Thread = FThreadManager::Get().CreateNamedThread(ThreadFunction, GetStaticThreadName());
    if (!Thread)
    {
        return false;
    }

    bIsRunning = true;

    if (!Thread->Start())
    {
        return false;
    }

    return true;
}

void FRHIExecutorThread::StopExecution()
{
    bIsRunning = false;

    WaitCondition.NotifyOne();

    Check(Thread != nullptr);
    Thread->WaitForCompletion(kWaitForThreadInfinity);
}

void FRHIExecutorThread::Execute(const FRHIExecutorTask& ExecutionTask)
{
    {
        // Set the work to execute
        TScopedLock TaskLock(CurrentTaskCS);
        CurrentTask = ExecutionTask;

        // Then notify worker
        WaitCondition.NotifyOne();
    }

    // Wait for the worker to start
    while (!bIsExecuting) { }
}

void FRHIExecutorThread::Worker()
{
    while(bIsRunning)
    {
        TScopedLock WaitLock(WaitCS);
        WaitCondition.Wait(WaitLock);

        {
            TScopedLock TaskLock(CurrentTaskCS);
            bIsExecuting = true;

            CurrentTask();

            bIsExecuting = false;
        }
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandListExecutor

FRHICommandListExecutor& FRHICommandListExecutor::Get()
{
    return GRHICommandListExecutor;
}

FRHICommandListExecutor::FRHICommandListExecutor()
    : ExecutorThread()
    , Statistics()
    , CommandContext(nullptr)
{ }

bool FRHICommandListExecutor::Initialize()
{
#if ENABLE_RHI_EXECUTOR_THREAD
    if (!GRHICommandListExecutor.ExecutorThread.Start())
    {
        return false;
    }
#endif

    return true;
}

void FRHICommandListExecutor::Release()
{
#if ENABLE_RHI_EXECUTOR_THREAD
    GRHICommandListExecutor.ExecutorThread.StopExecution();
#endif
}

void FRHICommandListExecutor::ExecuteCommandList(FRHICommandList& CmdList)
{
    FRHICommandList* TempCommandList = dbg_new FRHICommandList();
    TempCommandList->Swap(CmdList);

    FRHIExecutorTask ExecutorTask = FRHIExecutorTask([=]()
    {
        // Execute CommandList
        GetContext().StartContext();

        {
            TRACE_FUNCTION_SCOPE();

            Statistics.Reset();

            InternalExecuteCommandList(*TempCommandList);
        }

        GetContext().FinishContext();

        delete TempCommandList;
    });

#if ENABLE_RHI_EXECUTOR_THREAD
    ExecutorThread.Execute(ExecutorTask);
#else
    ExecutorTask();
#endif
}

void FRHICommandListExecutor::ExecuteCommandLists(FRHICommandList* const* CmdLists, uint32 NumCmdLists)
{
    // TODO: We need to present on the CommandContext before we can continue here

    // Execute multiple CommandList
    GetContext().StartContext();

    {
        TRACE_FUNCTION_SCOPE();

        Statistics.Reset();

        for (uint32 Index = 0; Index < NumCmdLists; ++Index)
        {
            FRHICommandList* CurrentCmdList = CmdLists[Index];
            InternalExecuteCommandList(*CurrentCmdList);
        }
    }

    GetContext().FinishContext();
}

void FRHICommandListExecutor::WaitForGPU()
{
    if (CommandContext)
    {
        CommandContext->Flush();
    }
}

void FRHICommandListExecutor::InternalExecuteCommandList(FRHICommandList& CommandList)
{
    if (CommandList.FirstCommand)
    {
        FRHICommand* CurrentCommand = CommandList.FirstCommand;
        while (CurrentCommand != nullptr)
        {
            FRHICommand* PreviousCommand = CurrentCommand;
            CurrentCommand = CurrentCommand->NextCommand;
            PreviousCommand->ExecuteAndRelease(GetContext());
        }

        Statistics.NumDrawCalls     += CommandList.GetNumDrawCalls();
        Statistics.NumDispatchCalls += CommandList.GetNumDispatchCalls();
        Statistics.NumCommands      += CommandList.GetNumCommands();

        CommandList.FirstCommand = nullptr;
        CommandList.LastCommand  = nullptr;
        CommandList.Reset();
    }
}
