#include "RHICommandList.h"

#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformThreadMisc.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

RHI_API FRHICommandListExecutor GRHICommandExecutor;

TAutoConsoleVariable<bool> CVarEnableRHIThread("RHI.EnableRHIThread", true);

FRHIThread* FRHIThread::GInstance = nullptr;

FRHIThread::FRHIThread()
    : Thread(nullptr)
    , WaitCS()
    , WaitCondition()
    , bIsRunning(false)
{ }

bool FRHIThread::Startup()
{
    if (!GInstance)
    {
        GInstance = new FRHIThread();
        if (!CVarEnableRHIThread.GetValue())
        {
            return true;
        }
        
        if (!GInstance->Create())
        {
            return false;
        }
    }

    return true;
}

void FRHIThread::Shutdown()
{
    if (GInstance)
    {
        if (CVarEnableRHIThread.GetValue())
        {
            GInstance->Stop();
        }

        delete GInstance;
        GInstance = nullptr;
    }
}

FRHIThread& FRHIThread::Get()
{
    CHECK(GInstance != nullptr);
    return *GInstance;
}

bool FRHIThread::Create()
{
    Thread = FPlatformThreadMisc::CreateThread(this);
    if (!Thread)
    {
        return false;
    }

    Thread->SetName("RHI-CommandList Thread");

    if (!Thread->Start())
    {
        return false;
    }

    return true;
}

bool FRHIThread::Start()
{
    bIsRunning = true;
    return true;
}

int32 FRHIThread::Run()
{
    while (bIsRunning)
    {
        TScopedLock WaitLock(WaitCS);
        WaitCondition.Wait(WaitLock);

        FRHIThreadTask CurrentTask;
        {
            SCOPED_LOCK(TasksCS);
            
            if (!Tasks.IsEmpty())
            {
                CurrentTask = ::Move(Tasks.FirstElement());
                Tasks.RemoveAt(0);
            }
        }

        if (CurrentTask)
        {
            TRACE_FUNCTION_SCOPE();
            CurrentTask.CommandList->Execute();
            NumCompletedTasks++;
        }
    }

    return 0;
}

void FRHIThread::Stop()
{
    if (bIsRunning)
    {
        WaitForOutstandingTasks();

        bIsRunning = false;
        WaitCondition.NotifyAll();

        CHECK(Thread != nullptr);
        Thread->WaitForCompletion();

        Thread.Reset();
    }
}

void FRHIThread::Execute(FRHIThreadTask&& NewTask)
{
    if (bIsRunning)
    {
        // Set the work to execute
        {
            SCOPED_LOCK(TasksCS);

            Tasks.Emplace(Move(NewTask));
            NumSubmittedTasks++;
        }

        // Then notify worker
        WaitCondition.NotifyAll();
    }
}

void FRHIThread::WaitForOutstandingTasks()
{
    while (NumCompletedTasks.Load() < NumSubmittedTasks.Load())
    {
        WaitCondition.NotifyAll();
        FPlatformThreadMisc::Pause();
    }
}


FRHICommandListExecutor::FRHICommandListExecutor()
    : Statistics()
    , CommandContext(nullptr)
{ }

bool FRHICommandListExecutor::Initialize()
{
    return true;
}

void FRHICommandListExecutor::Release()
{
    if (FRHIThread::IsRunning())
    {
        FRHIThread::Get().Stop();
    }
}

void FRHICommandListExecutor::Tick()
{
    Statistics.NumDrawCalls     = 0;
    Statistics.NumDispatchCalls = 0;
    Statistics.NumCommands      = 0;
}

void FRHICommandListExecutor::ExecuteCommandList(FRHICommandList& CommandList)
{
    if (CommandList.HasCommands())
    {
        Statistics.NumDrawCalls     += CommandList.GetNumDrawCalls();
        Statistics.NumDispatchCalls += CommandList.GetNumDispatchCalls();
        Statistics.NumCommands      += CommandList.GetNumCommands();

        if (CVarEnableRHIThread.GetValue())
        {
            FRHICommandList* NewCommandList = new FRHICommandList();
            NewCommandList->ExchangeState(CommandList);
            NewCommandList->SetCommandContext(CommandContext);

            FRHIThread::Get().Execute(FRHIThreadTask(NewCommandList));
        }
        else
        {
            CommandList.SetCommandContext(CommandContext);
            CommandList.Execute();
        }
    }
}

void FRHICommandListExecutor::WaitForOutstandingTasks()
{
    FRHIThread::Get().WaitForOutstandingTasks();
}

void FRHICommandListExecutor::WaitForGPU()
{
    if (FRHIThread::IsRunning())
    {
        WaitForOutstandingTasks();
    }

    if (CommandContext)
    {
        CommandContext->Flush();
    }
}
