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


FRHICommandList::FRHICommandList() noexcept
    : Memory()
    , CommandPointer(nullptr)
    , FirstCommand(nullptr)
    , CommandContext(nullptr)
    , FinishedEvent(nullptr)
    , Statistics()
    , NumCommands(0)
{
    CommandPointer = &FirstCommand;
}

FRHICommandList::~FRHICommandList() noexcept
{
    Reset();
}

void FRHICommandList::Execute() noexcept
{
    IRHICommandContext& CommandContextRef = GetCommandContext();
    CommandContextRef.RHIStartContext();
    ExecuteWithContext(CommandContextRef);
    CommandContextRef.RHIFinishContext();
}

void FRHICommandList::ExecuteWithContext(IRHICommandContext& InCommandContext) noexcept
{
    FRHICommand* CurrentCommand = FirstCommand;
    while (CurrentCommand != nullptr)
    {
        FRHICommand* PreviousCommand = CurrentCommand;
        CurrentCommand = CurrentCommand->NextCommand;
        PreviousCommand->ExecuteAndRelease(InCommandContext);
    }

    FirstCommand = nullptr;

    // Trigger event
    if (FinishedEvent)
    {
        FinishedEvent->Trigger();
        FinishedEvent = nullptr;
    }

    Reset();
}

void FRHICommandList::Reset() noexcept
{
    if (FirstCommand != nullptr)
    {
        // Call destructor on all commands that has not been executed
        FRHICommand* Command = FirstCommand;
        while (Command != nullptr)
        {
            FRHICommand* PreviousCommand = Command;
            Command = Command->NextCommand;
            PreviousCommand->~FRHICommand();
        }

        FirstCommand = nullptr;
    }

    CommandPointer = &FirstCommand;
    CommandContext = nullptr;
    NumCommands    = 0;

    Statistics.Reset();
    Memory.Reset();
}

void FRHICommandList::ExchangeState(FRHICommandList& Other) noexcept
{
    // This works fine in this case
    FMemory::Memswap(this, &Other, sizeof(FRHICommandList));

    if (CommandPointer == &Other.FirstCommand)
        CommandPointer = &FirstCommand;

    if (Other.CommandPointer == &FirstCommand)
        Other.CommandPointer = &Other.FirstCommand;
}

void FRHICommandList::FlushGarbageCollection() noexcept
{
    ExecuteLambda([]()
    {
        GRHICommandExecutor.FlushGarbageCollection();
    });
}


FRHIThread::FRHIThread()
    : Thread(nullptr)
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
        for(;;)
        {
            FRHICommandList* CommandList = nullptr;
            if (!Tasks.Dequeue(CommandList))
            {
                break;
            }

            if (CommandList)
            {
                TRACE_FUNCTION_SCOPE();

                CommandList->Execute();
                delete CommandList;

                NumCompletedTasks++;
            }
        }

        FPlatformThreadMisc::Pause();
    }

    return 0;
}

void FRHIThread::Stop()
{
    if (bIsRunning)
    {
        WaitForOutstandingTasks();
        bIsRunning = false;
    }
}

void FRHIThread::Execute(FRHICommandList* InCommandList)
{
    if (bIsRunning)
    {
        Tasks.Enqueue(InCommandList);
        NumSubmittedTasks++;
    }
}

void FRHIThread::WaitForOutstandingTasks()
{
    while (NumCompletedTasks.Load() < NumSubmittedTasks.Load())
    {
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
    FlushGarbageCollection();

    if (CVarEnableRHIThread.GetValue())
    {
        if (RHIThread)
        {
            RHIThread->Stop();
            delete RHIThread;
            RHIThread = nullptr;
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
        DeletedResources.Add(InResource);
    }
}

void FRHICommandExecutor::FlushGarbageCollection()
{
    TScopedLock Lock(DeletedResourcesCS);

    if (!DeletedResources.IsEmpty())
    {
        for (FRHIResource* Resource : DeletedResources)
        {
            GetRHI()->RHIEnqueueResourceDeletion(Resource);
        }

        DeletedResources.Clear();
    }
}

void FRHICommandExecutor::ExecuteCommandList(FRHICommandList& CommandList)
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
            RHIThread->Execute(NewCommandList);
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
