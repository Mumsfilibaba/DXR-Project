#include "RHICommandList.h"

#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

RHI_API FRHICommandListExecutor GRHICommandListExecutor;

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
    TempCommandList->ExchangeState(CmdList);

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
        int32 NumCommands = 0;

        FRHICommand* CurrentCommand = CommandList.FirstCommand;
        while (CurrentCommand != nullptr)
        {
            FRHICommand* PreviousCommand = CurrentCommand;
            CurrentCommand = CurrentCommand->NextCommand;
            PreviousCommand->ExecuteAndRelease(GetContext());

            NumCommands++;
        }

        Statistics.NumDrawCalls     += CommandList.GetNumDrawCalls();
        Statistics.NumDispatchCalls += CommandList.GetNumDispatchCalls();

        // Ensure that all commands got executed
        Check(CommandList.NumCommands == NumCommands);

        CommandList.FirstCommand = nullptr;
        CommandList.Reset();
    }
}
