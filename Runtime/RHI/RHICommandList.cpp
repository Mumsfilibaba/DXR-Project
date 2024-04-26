#include "RHICommandList.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/PlatformThread.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

RHI_API FRHICommandExecutor GRHICommandExecutor;

static TAutoConsoleVariable<bool> CVarEnableRHIThread(
    "RHI.EnableRHIThread",
    "Enables the use of a separate Thread for executing RHI Commands",
    true);

FRHIThread::FRHIThread()
    : Thread(nullptr)
    , WaitCS()
    , WaitCondition()
    , bIsRunning(false)
{
}

FRHIThread::~FRHIThread()
{
    CHECK(Thread != nullptr);
    Thread->WaitForCompletion();
    delete Thread;
}

bool FRHIThread::Startup()
{
    Thread = FPlatformThread::Create(this, "RHIThread");
    if (!Thread)
    {
        return false;
    }

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
                CurrentTask = Move(Tasks.FirstElement());
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
    }
}

void FRHIThread::Execute(FRHIThreadTask&& NewTask)
{
    if (bIsRunning)
    {
        {
            SCOPED_LOCK(TasksCS);
            Tasks.Emplace(Move(NewTask));
            NumSubmittedTasks++;
        }

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


FRHICommandExecutor::FRHICommandExecutor()
    : Statistics()
    , CommandContext(nullptr)
{
}

bool FRHICommandExecutor::Initialize()
{
    if (!CVarEnableRHIThread.GetValue())
    {
        return true;
    }

    RHIThread = new FRHIThread();
    if (!RHIThread->Startup())
    {
        LOG_ERROR("Failed to startup RHIThread");
        return false;
    }

    return true;
}

void FRHICommandExecutor::Release()
{
    if (CVarEnableRHIThread.GetValue())
    {
        if (RHIThread)
        {
            RHIThread->Stop();
            delete RHIThread;
        }
    }
}

void FRHICommandExecutor::Tick()
{
    Statistics.NumDrawCalls     = 0;
    Statistics.NumDispatchCalls = 0;
    Statistics.NumCommands      = 0;
}

void FRHICommandExecutor::EnqueueResourceDeletion(FRHIResource* InResource)
{
    TScopedLock Lock(DeletedResourcesCS);

    if (InResource)
    {
        InResource->AddRef();
        DeletedResources.Add(InResource);
    }
}

void FRHICommandExecutor::ExecuteCommandList(FRHICommandList& CommandList)
{
    {
        TScopedLock Lock(DeletedResourcesCS);

        if (!DeletedResources.IsEmpty())
        {
            for (FRHIResource* Resource : DeletedResources)
            {
                CommandList.DestroyResource(Resource);
                Resource->Release();
            }

            DeletedResources.Clear();
        }
    }

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

            RHIThread->Execute(FRHIThreadTask(NewCommandList));
        }
        else
        {
            CommandList.SetCommandContext(CommandContext);
            CommandList.Execute();
        }
    }
}

void FRHICommandExecutor::WaitForOutstandingTasks()
{
    RHIThread->WaitForOutstandingTasks();
}

void FRHICommandExecutor::WaitForGPU()
{
    if (RHIThread)
    {
        WaitForOutstandingTasks();
    }

    if (CommandContext)
    {
        CommandContext->RHIFlush();
    }
}
