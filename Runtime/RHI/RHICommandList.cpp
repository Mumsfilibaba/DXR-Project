#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Platform/PlatformThread.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Tasks/TaskGraph.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "RHI/RHICommandList.h"

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
    Memory::Memswap(this, &Other, sizeof(FRHICommandList));

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

FRHICommandListExecutor* FRHICommandListExecutor::GCommandListExecutor = nullptr;

FRHICommandListExecutor::FRHICommandListExecutor(IRHICommandContext* InDefaultCommandContext)
    : DeletedResources()
    , DeletedResourcesCS()
    , DefaultCommandContext(InDefaultCommandContext)
{
}

FRHICommandListExecutor::~FRHICommandListExecutor()
{
    DefaultCommandContext = nullptr;
}

bool FRHICommandListExecutor::Initialize()
{
    IRHICommandContext* Context = RHI::Device->ObtainCommandContext();
    if (!Context)
    {
        return false;
    }

    GCommandListExecutor = new FRHICommandListExecutor(Context);
    return true;
}

void FRHICommandListExecutor::Release()
{
    if (GCommandListExecutor)
    {
        GCommandListExecutor->WaitForCommands();

        delete GCommandListExecutor;
        GCommandListExecutor = nullptr;
    }
}

void FRHICommandListExecutor::Tick()
{
    STAT_SET(STAT_RHI_Commands,      0);
    STAT_SET(STAT_RHI_DispatchCalls, 0);
    STAT_SET(STAT_RHI_DrawCalls,     0);
    STAT_SET(STAT_RHI_AccelerationStructureBuilds, 0);
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
    while (true)
    {
        TArray<FRHIResource*> ResourcesToDelete;

        {
            TScopedLock Lock(DeletedResourcesCS);
            if (DeletedResources.IsEmpty())
            {
                break;
            }

            ResourcesToDelete = Move(DeletedResources);
        }

        for (FRHIResource* Resource : ResourcesToDelete)
        {
            RHI::Device->EnqueueResourceDeletion(Resource);
        }
    }
}

void FRHICommandListExecutor::ExecuteCommandList(FRHICommandList& CommandList)
{
    if (!CommandList.HasCommands())
    {
        return;
    }

    if (FTaskGraph::Get().IsRHIThreadEnabled())
    {
        FRHICommandList* NewCommandList = new FRHICommandList();
        NewCommandList->ExchangeState(CommandList);

        // Execute with the default command-context for now
        NewCommandList->SetCommandContext(DefaultCommandContext);

        Tasks::LaunchOnRHIThread("RHIExecuteCommandList",
            [NewCommandList]()
            {
                TRACE_FUNCTION_SCOPE();

                NewCommandList->Execute();
                delete NewCommandList;
            });
    }
    else
    {
        CommandList.SetCommandContext(DefaultCommandContext);
        CommandList.Execute();
    }
}

void FRHICommandListExecutor::WaitForCommands()
{
    if (FTaskGraph::Get().IsRHIThreadEnabled())
    {
        Tasks::LaunchOnRHIThread("RHIFlush", []() { }).Wait();
    }
}

void FRHICommandListExecutor::WaitForGPU()
{
    WaitForCommands();

    if (DefaultCommandContext)
    {
        DefaultCommandContext->Flush();
    }
}
