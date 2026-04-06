#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/PlatformThread.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "RHI/RHICommandList.h"

static TAutoConsoleVariable<bool> CVarEnableRHIThread(
    "RHI.EnableRHIThread",
    "Enables the use of a separate Thread for executing RHI Commands",
    true);

bool GRHIVerboseEventOutput = false;

static TAutoConsoleVariable<bool> CVarVerboseEventOutput(
    "RHI.VerboseEventOutput",
    "When true, PushEvent names are sent to OutputDebugString (visible in debugger output)",
    GRHIVerboseEventOutput);

FRHICommandList::FRHICommandList() noexcept
    : Memory()
    , CommandPointer(nullptr)
    , FirstCommand(nullptr)
    , CommandContext(nullptr)
    , FinishedEvent(nullptr)
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
    STAT_ADD(STAT_RHI_Commands, NumCommands);

    // Then execute all commands on the assigned context
    IRHICommandContext& CommandContextRef = GetCommandContext();
    CommandContextRef.StartContext();

    ExecuteWithContext(CommandContextRef);

    CommandContextRef.FinishContext();
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

    Memory.Reset();
}

void FRHICommandList::ExchangeState(FRHICommandList& Other) noexcept
{
    FMemory::Memswap(this, &Other, sizeof(FRHICommandList));

    if (CommandPointer == &Other.FirstCommand)
    {
        CommandPointer = &FirstCommand;
    }

    if (Other.CommandPointer == &FirstCommand)
    {
        Other.CommandPointer = &Other.FirstCommand;
    }
}

void FRHICommandList::FlushDeletedResources() noexcept
{
    ExecuteLambda([]()
    {
        FRHICommandListExecutor::Get().FlushDeletedResources();
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

FRHICommandListExecutor* FRHICommandListExecutor::GCommandListExecutor = nullptr;

FRHICommandListExecutor::FRHICommandListExecutor(IRHICommandContext* InDefaultCommandContext)
    : DeletedResources()
    , DeletedResourcesCS()
    , DefaultCommandContext(InDefaultCommandContext)
    , RHIThread(nullptr)
{
}

FRHICommandListExecutor::~FRHICommandListExecutor()
{
    DefaultCommandContext = nullptr;
}

bool FRHICommandListExecutor::InitializeRHIThread()
{
    RHIThread = new FRHIThread();
    if (!RHIThread->Startup())
    {
        LOG_ERROR("Failed to startup RHIThread");
        return false;
    }

    return true;
}

void FRHICommandListExecutor::ReleaseRHIThread()
{
	if (RHIThread)
	{
		RHIThread->Stop();
		delete RHIThread;
		RHIThread = nullptr;
	}
}

bool FRHICommandListExecutor::Initialize()
{
    IRHICommandContext* Context = FRHI::Get()->ObtainCommandContext();
    if (!Context)
    {
        return false;
    }

    // Create the executor
    FRHICommandListExecutor* LocalExecutor = new FRHICommandListExecutor(Context);
    GCommandListExecutor = LocalExecutor;

    // Initialize the RHI-Thread
    if (CVarEnableRHIThread.GetValue())
    {
        return LocalExecutor->InitializeRHIThread();
    }
    else
    {
        return true;
    }
}

void FRHICommandListExecutor::Release()
{
    if (GCommandListExecutor)
    {
        // Release the RHI-Thread
        GCommandListExecutor->ReleaseRHIThread();

        // Delete the instance
        delete GCommandListExecutor;
        GCommandListExecutor = nullptr;
    }
}

void FRHICommandListExecutor::Tick()
{
    STAT_SET(STAT_RHI_Commands,      0);
    STAT_SET(STAT_RHI_DispatchCalls, 0);
    STAT_SET(STAT_RHI_DrawCalls,     0);
}

void FRHICommandListExecutor::EnqueueResourceDeletion(FRHIResource* InResource)
{
    TScopedLock Lock(DeletedResourcesCS);

    if (InResource)
    {
        DeletedResources.Add(InResource);
    }
}

void FRHICommandListExecutor::FlushDeletedResources()
{
    TScopedLock Lock(DeletedResourcesCS);

    if (!DeletedResources.IsEmpty())
    {
        for (FRHIResource* Resource : DeletedResources)
        {
            FRHI::Get()->EnqueueResourceDeletion(Resource);
        }

        DeletedResources.Clear();
    }
}

void FRHICommandListExecutor::ExecuteCommandList(FRHICommandList& CommandList)
{
    if (CommandList.HasCommands())
    {
        if (RHIThread)
        {
            FRHICommandList* NewCommandList = new FRHICommandList();
            NewCommandList->ExchangeState(CommandList);

            // Execute with the default command-context for now
            NewCommandList->SetCommandContext(DefaultCommandContext);
            
            RHIThread->Execute(NewCommandList);
        }
        else
        {
            CommandList.SetCommandContext(DefaultCommandContext);
            CommandList.Execute();
        }
    }
}

void FRHICommandListExecutor::WaitForCommands()
{
    if (RHIThread)
    {
        RHIThread->WaitForOutstandingTasks();
    }
}

void FRHICommandListExecutor::WaitForGPU()
{
    if (RHIThread)
    {
        WaitForCommands();
    }

    if (DefaultCommandContext)
    {
        DefaultCommandContext->Flush();
    }
}
