#include "ShaderCompilerTool.h"
#include "ShaderHeader.h"
#include <LaunchProgram/ProgramEntry.h>
#include <Core/Filesystem/File.h>
#include <Core/Misc/Config.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Misc/OutputDeviceManager.h>
#include <Core/Misc/Paths.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformThreadMisc.h>
#include <Core/Tasks/ParallelFor.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Threading/Atomic.h>
#include <Core/Threading/ThreadManager.h>
#include <ShaderCompiler/ShaderBytecodeCache.h>
#include <ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompiler/ShaderJobFile.h>
#include <ShaderCompiler/Remote/RemoteShaderCompilerClient.h>
#include <ShaderCompiler/Remote/RemoteShaderCompilerServer.h>

#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();

static constexpr uint32 GServerPollMilliseconds = 100;

struct FRHIStats
{
    AtomicInt32 NumCompiled;
    AtomicInt32 NumCached;
    AtomicInt32 NumFailed;
    AtomicInt32 NumSkipped;
};

static void CompileEntry(const FShaderJobFileEntry& Entry, const FRouteEntry& Route, bool bForce, FRHIStats& Stats)
{
    FShaderCompiler&      Compiler = FShaderCompiler::Get();
    FShaderBytecodeCache& Cache    = FShaderBytecodeCache::Get();

    FShaderCompileJob        Job         = Entry.Job;
    const FShaderCompileInfo CompileInfo = Job.ToCompileInfo();

    const uint64 CompileHash = Compiler.ComputeCompileHash(Job.SourceFile, CompileInfo, Route.Identity);
    if (!bForce && Cache.Contains(CompileHash))
    {
        Stats.NumCached.Add(1);
        return;
    }

    TArray<uint8>  ShaderCode;
    TArray<String> Dependencies;

    bool bCompiled = false;
    if (Route.Route == EJobRoute::Local)
    {
        bCompiled = Compiler.CompileFromFile(Job.SourceFile, CompileInfo, ShaderCode, &Dependencies);
    }
    else
    {
        bCompiled = Compiler.CompileOnRemote(*Route.Client, Job.SourceFile, CompileInfo, ShaderCode, &Dependencies) == ERemoteCompileStatus::Succeeded;
    }

    if (!bCompiled)
    {
        LOG_ERROR("[ShaderCompiler] %s: %s FAILED", Route.Target->Name, *Job.Name);
        Stats.NumFailed.Add(1);
        return;
    }

    Cache.Add(CompileHash, ShaderCode, Dependencies);
    Stats.NumCompiled.Add(1);
}

int32 ShaderCompilerTool::RunBatch(const FToolOptions& Options)
{
    const String JobFilePath = Options.JobFilePath.IsEmpty() ? FShaderJobFile::GetDefaultFilePath() : Options.JobFilePath;

    FShaderJobFile JobFile;
    String         LoadError;
    if (!JobFile.Load(JobFilePath, LoadError) || JobFile.GetEntries().IsEmpty())
    {
        LOG_ERROR("[ShaderCompiler] No jobs in '%s' %s. Run SandboxEditor first, or Shaders.SaveJobFile in its console.", *JobFilePath, *LoadError);
        return ToExitCode(EShaderCompilerExitCode::InvalidUsage);
    }

    if (!Options.OutputPath.IsEmpty())
    {
        FShaderBytecodeCache::SetFilePathOverride(Options.OutputPath);
    }

    FShaderBytecodeCache::Initialize();

    TArray<TUniquePtr<FRemoteShaderCompilerClient>> Clients;
    bool bRemoteUnreachable = false;
    const TArray<FRouteEntry> Routes = BuildRoutes(Options, Clients, bRemoteUnreachable);

    struct FWorkItem
    {
        const FShaderJobFileEntry* Entry;
        int32                      RouteIndex;
    };

    FRHIStats Stats[ARRAY_COUNT(GRHITypes)];

    TArray<FWorkItem> Work;
    for (const FShaderJobFileEntry* Entry : JobFile.Filter(TArrayView<const String>(Options.RHINames)))
    {
        int32 RouteIndex = -1;
        for (int32 Index = 0; Index < Routes.Size(); ++Index)
        {
            if (Entry->RHIName.Equals(Routes[Index].Target->Name, EStringCaseType::NoCase))
            {
                RouteIndex = Index;
            }
        }

        if (RouteIndex < 0)
        {
            continue;
        }

        if (Routes[RouteIndex].Route == EJobRoute::Skipped)
        {
            Stats[RouteIndex].NumSkipped.Add(1);
            continue;
        }

        Work.Add({ Entry, RouteIndex });
    }

    LOG_INFO("[ShaderCompiler] Compiling %d jobs from '%s'", Work.Size(), *JobFilePath);

    Tasks::ParallelFor(Work.Size(), [&](int32 Index)
    {
        if (!FProgramLoop::IsExitRequested())
        {
            CompileEntry(*Work[Index].Entry, Routes[Work[Index].RouteIndex], Options.bForce, Stats[Work[Index].RouteIndex]);
        }
    });

    FShaderBytecodeCache::Get().Save();

    int32 TotalFailed = 0;
    for (int32 Index = 0; Index < Routes.Size(); ++Index)
    {
        const FRHIStats& RHIStats = Stats[Index];
        LOG_INFO("[ShaderCompiler] %-7s %d compiled, %d cached, %d failed, %d skipped", Routes[Index].Target->Name,
            RHIStats.NumCompiled.Load(), RHIStats.NumCached.Load(), RHIStats.NumFailed.Load(), RHIStats.NumSkipped.Load());

        TotalFailed += RHIStats.NumFailed.Load();
    }

    FShaderBytecodeCache::Release();

    for (TUniquePtr<FRemoteShaderCompilerClient>& Client : Clients)
    {
        Client->Disconnect();
    }

    if (TotalFailed > 0)
    {
        return ToExitCode(EShaderCompilerExitCode::JobsFailed);
    }

    return bRemoteUnreachable ? ToExitCode(EShaderCompilerExitCode::RemoteUnreachable) : ToExitCode(EShaderCompilerExitCode::Success);
}

static String GetCompileOutputPath(const FToolOptions& Options, const FRHITarget& Target, int32 NumTargets)
{
    if (NumTargets == 1)
    {
        return Options.OutputPath;
    }

    const String Extension = File::ExtractExtension(Options.OutputPath);
    const String Stem(Options.OutputPath.Data(), Options.OutputPath.Length() - Extension.Length());
    return String::Printf("%s_%s%s", *Stem, Target.Name, *Extension);
}

int32 ShaderCompilerTool::RunCompile(const FToolOptions& Options)
{
    if (!Options.HeaderPath.IsEmpty())
    {
        return ShaderHeader::RunCompile(Options);
    }

    FShaderCompiler& Compiler = FShaderCompiler::Get();

    FShaderCompileJob Job;
    Job.Name        = File::ExtractFilename(Options.CompileFile) + ':' + Options.EntryPoint;
    Job.SourceFile  = Options.CompileFile;
    Job.EntryPoint  = Options.EntryPoint;
    Job.ShaderModel = Options.ShaderModel;
    Job.ShaderStage = Options.ShaderStage;
    Job.bOptimize   = true;
    Job.bDebugInfo  = FShaderCompiler::IsDebugInfoEnabled();
    Job.Defines     = Options.Defines;
    Job.IncludeDirs = Options.IncludeDirs;

    TArray<TUniquePtr<FRemoteShaderCompilerClient>> Clients;
    bool bRemoteUnreachable = false;
    const TArray<FRouteEntry> Routes = BuildRoutes(Options, Clients, bRemoteUnreachable);

    int32 NumCompiled = 0;
    int32 NumFailed   = 0;
    for (const FRouteEntry& Route : Routes)
    {
        if (Route.Route == EJobRoute::Skipped)
        {
            continue;
        }

        Job.OutputLanguage = Route.Target->OutputLanguage;
        const FShaderCompileInfo CompileInfo = Job.ToCompileInfo();

        TArray<uint8> ShaderCode;
        bool bCompiled = false;
        if (Route.Route == EJobRoute::Local)
        {
            bCompiled = Compiler.CompileFromFile(Job.SourceFile, CompileInfo, ShaderCode);
        }
        else
        {
            bCompiled = Compiler.CompileOnRemote(*Route.Client, Job.SourceFile, CompileInfo, ShaderCode, nullptr) == ERemoteCompileStatus::Succeeded;
        }

        if (!bCompiled)
        {
            LOG_ERROR("[ShaderCompiler] %s: %s FAILED", Route.Target->Name, *Job.Name);
            NumFailed++;
            continue;
        }

        const String OutputPath = GetCompileOutputPath(Options, *Route.Target, Routes.Size());
        File::CreateDirectoryTree(File::GetDirectoryOf(OutputPath));

        TFileRef<IPlatformFile> Output = FPlatformFile::OpenForWrite(OutputPath);
        if (!Output || Output->Write(ShaderCode.Data(), ShaderCode.Size()) != ShaderCode.Size())
        {
            LOG_ERROR("[ShaderCompiler] %s: Cannot write '%s'", Route.Target->Name, *OutputPath);
            NumFailed++;
            continue;
        }

        LOG_INFO("[ShaderCompiler] %s: Wrote '%s' (%d bytes)", Route.Target->Name, *OutputPath, ShaderCode.Size());
        NumCompiled++;
    }

    for (TUniquePtr<FRemoteShaderCompilerClient>& Client : Clients)
    {
        Client->Disconnect();
    }

    if (NumFailed > 0)
    {
        return ToExitCode(EShaderCompilerExitCode::JobsFailed);
    }

    if (bRemoteUnreachable)
    {
        return ToExitCode(EShaderCompilerExitCode::RemoteUnreachable);
    }

    return NumCompiled > 0 ? ToExitCode(EShaderCompilerExitCode::Success) : ToExitCode(EShaderCompilerExitCode::JobsFailed);
}

int32 ShaderCompilerTool::RunServer(const FToolOptions& Options)
{
    FRemoteShaderCompilerServerSettings Settings;
    Settings.bAllowRemoteConnections = Options.bAllowRemote;
    Settings.Port                    = Options.Port;

    if (!Options.BindAddress.IsEmpty())
    {
        Settings.BindAddress = Options.BindAddress;
    }

    for (const FRemoteTarget& Remote : Options.RemoteTargets)
    {
        Settings.ForwardTargets.Add({ Remote.Target->OutputLanguage, Remote.Host, Remote.Port });
    }

    FRemoteShaderCompilerServer Server(Settings);
    if (!Server.Launch())
    {
        return ToExitCode(EShaderCompilerExitCode::ServerFailed);
    }

    LOG_INFO("[ShaderCompiler] Serving until the window is closed or Ctrl+C is pressed");

    while (!FProgramLoop::IsExitRequested())
    {
        FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(GServerPollMilliseconds));
    }

    Server.Shutdown();
    return ToExitCode(EShaderCompilerExitCode::Success);
}

static int32 ShaderCompilerMain()
{
    FToolOptions Options;
    String       Error;
    if (!ShaderCompilerTool::ParseOptions(Options, Error))
    {
        LOG_ERROR("[ShaderCompiler] %s", *Error);
        ShaderCompilerTool::PrintUsage();
        return ToExitCode(EShaderCompilerExitCode::InvalidUsage);
    }

    if (Options.bShowHelp)
    {
        ShaderCompilerTool::PrintUsage();
        return ToExitCode(EShaderCompilerExitCode::Success);
    }

    if (!FConfig::Initialize())
    {
        LOG_WARNING("[ShaderCompiler] Failed to read the config, using the CVar defaults");
    }
    else
    {
        GConfig->LoadConsoleVariables();
    }

    FConsoleManager::Get().LoadFromCommandLine();

    if (IConsoleVariable* UseRemote = FConsoleManager::Get().FindConsoleVariable("RHI.ShaderCompiler.UseRemote"))
    {
        UseRemote->SetAsBool(false, EConsoleVariableFlags::SetByCode);
    }

    if (!FThreadManager::Initialize() || !FTaskGraph::Initialize() || !FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        LOG_ERROR("[ShaderCompiler] Failed to initialize the shader compiler");
        return ToExitCode(EShaderCompilerExitCode::ServerFailed);
    }

    int32 Result = 0;
    if (Options.bServer)
    {
        Result = ShaderCompilerTool::RunServer(Options);
    }
    else if (!Options.CompileFile.IsEmpty())
    {
        Result = ShaderCompilerTool::RunCompile(Options);
    }
    else
    {
        Result = ShaderCompilerTool::RunBatch(Options);
    }

    FShaderCompiler::Destroy();
    FTaskGraph::Release();
    FThreadManager::Release();
    FConfig::Release();
    return Result;
}

IMPLEMENT_PROGRAM_MAIN("ShaderCompiler", &ShaderCompilerMain);
