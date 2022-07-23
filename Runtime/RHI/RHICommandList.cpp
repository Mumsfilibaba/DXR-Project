#include "RHICommandList.h"

#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

RHI_API FRHICommandListExecutor GRHICommandExecutor;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIThread

FRHIThread::FRHIThread()
    : Thread(nullptr)
    , WaitCS()
    , WaitCondition()
    , bIsRunning(false)
{ }

bool FRHIThread::Start()
{
    TFunction<void()> ThreadFunction = Bind(&FRHIThread::Worker, this);

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

void FRHIThread::StopExecution()
{
    bIsRunning = false;

    WaitCondition.NotifyAll();

    Check(Thread != nullptr);
    Thread->WaitForCompletion(kWaitForThreadInfinity);
}

void FRHIThread::Execute(const FRHIThreadTask& NewTask)
{
    {
        // Set the work to execute
        TScopedLock TaskLock(TasksCS);
        Tasks.Emplace(NewTask);
        
        NumSubmittedTasks++;
    }

    // Then notify worker
    WaitCondition.NotifyOne();
}

void FRHIThread::WaitForCompletion()
{
    while (NumCompletedTasks.Load() < NumSubmittedTasks.Load())
    {
        PauseInstruction();
    }
}

void FRHIThread::Worker()
{
    while(bIsRunning)
    {
        TScopedLock WaitLock(WaitCS);
        WaitCondition.Wait(WaitLock);

        FRHIThreadTask CurrentTask;

        {
            TScopedLock Lock(TasksCS);
            if (!Tasks.IsEmpty())
            {
                CurrentTask = Move(Tasks.FirstElement());
                Tasks.RemoveAt(0);
            }
        }
        
        if (CurrentTask)
        {
            TRACE_FUNCTION_SCOPE();
            CurrentTask.CommandList->Execute();
        }

        NumCompletedTasks++;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandListExecutor

FRHICommandListExecutor::FRHICommandListExecutor()
    : ExecutorThread()
    , Statistics()
    , CommandContext(nullptr)
{ }

bool FRHICommandListExecutor::Initialize()
{
#if ENABLE_RHI_EXECUTOR_THREAD
    if (!ExecutorThread.Start())
    {
        return false;
    }
#endif

    return true;
}

void FRHICommandListExecutor::Release()
{
#if ENABLE_RHI_EXECUTOR_THREAD
    ExecutorThread.StopExecution();
#endif
}

void FRHICommandListExecutor::ExecuteCommandList(FRHICommandList& CommandList)
{
    if (CommandList.HasCommands())
    {
        Statistics.NumDrawCalls     += CommandList.GetNumDrawCalls();
        Statistics.NumDispatchCalls += CommandList.GetNumDispatchCalls();

        FRHICommandList* NewCommandList = dbg_new FRHICommandList();
        NewCommandList->ExchangeState(CommandList);
        NewCommandList->SetCommandContext(CommandContext);

        ExecutorThread.Execute(FRHIThreadTask(NewCommandList));
    }
}

void FRHICommandListExecutor::WaitForOutstandingTasks()
{
    ExecutorThread.WaitForCompletion();
}

void FRHICommandListExecutor::WaitForGPU()
{
    WaitForOutstandingTasks();

    if (CommandContext)
    {
        CommandContext->Flush();
    }
}
