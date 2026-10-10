#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Tasks/ParallelFor.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Threading/Atomic.h"
#include "RendererCore/RenderSettings.h"
#include "ShaderCompiler/ShaderBytecodeCache.h"
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/Shaders/ShaderManifest.h"
#include "RHI/RHI.h"
#include "RHI/RHIPipelineStateCache.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCompiler/ShaderCompileJob.h"
#include "ShaderCompiler/ShaderJobFile.h"

FShaderCache* FShaderCache::ShaderCache = nullptr;

static TAutoConsoleVariable<bool> CVarPrewarm(
    "Renderer.ShaderCache.Prewarm",
    "Compiles the shader permutations the last run needed on worker threads during startup",
    true);

static TAutoConsoleVariable<bool> CVarSaveManifest(
    "Renderer.ShaderCache.SaveManifest",
    "Records which shader permutations were used, so the next run can pre-warm them",
    true);

static FAutoConsoleCommand CCmdRecompileShaders(
    "Renderer.RecompileShaders",
    "Drops every compiled shader and the pipelines built from them, so the next frame recompiles",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        if (FShaderCache* Cache = FShaderCache::TryGet())
        {
            Cache->FlushCompiledShaders();
        }
    }));

static void CompileAllShaders(StringView Arguments);

static FAutoConsoleCommand CCmdCompileAllShaders(
    "Shaders.CompileAll",
    "Compiles every registered shader type and permutation for an RHI (Null, D3D11, D3D12, Vulkan or Metal, defaults to the active one) and logs the failures",
    FConsoleCommandDelegate::CreateStatic(&CompileAllShaders));

static bool ParseRHIType(StringView Text, ERHIType& OutRHIType)
{
    constexpr ERHIType RHITypes[] =
    {
        ERHIType::Null,
        ERHIType::D3D11,
        ERHIType::D3D12,
        ERHIType::Vulkan,
        ERHIType::Metal,
    };

    const String Name(Text);
    for (ERHIType RHIType : RHITypes)
    {
        if (Name.Equals(ToString(RHIType), EStringCaseType::NoCase))
        {
            OutRHIType = RHIType;
            return true;
        }
    }

    return false;
}

#if EDITOR_BUILD
static void SaveShaderJobFile(StringView /* Arguments */)
{
    if (FShaderCache* Cache = FShaderCache::TryGet())
    {
        Cache->SaveJobFile();
    }
}

static FAutoConsoleCommand CCmdSaveShaderJobFile(
    "Shaders.SaveJobFile",
    "Writes every shader permutation used so far, for every RHI, to the job file the ShaderCompiler tool compiles",
    FConsoleCommandDelegate::CreateStatic(&SaveShaderJobFile));
#endif

FShaderPermutationDesc FShaderCache::CreateTargetPermutationDesc(ERHIType TargetRHI, int32 PermutationID)
{
    if (RHI::IsInitialized() && RHI::Device->GetRHIType() == TargetRHI)
    {
        return FShaderCache::CreatePermutationDesc(PermutationID);
    }

    // Without the target's device, assume the features its backend can have. SER depends on the driver, so it stays off.
    // Metal has bindless and inline ray tracing, but neither view instancing nor a ray tracing pipeline.
    const bool bSupportsModernFeatures = TargetRHI == ERHIType::D3D12 || TargetRHI == ERHIType::Vulkan;
    const bool bIsMetal                = TargetRHI == ERHIType::Metal;

    FShaderPermutationDesc Desc;
    Desc.PermutationID               = PermutationID;
    Desc.bSupportsBindless           = bSupportsModernFeatures || bIsMetal;
    Desc.bSupportsViewInstancing     = bSupportsModernFeatures;
    Desc.bSupportsRayTracing         = bSupportsModernFeatures || bIsMetal;
    Desc.bSupportsRayTracingPipeline = bSupportsModernFeatures;
    Desc.bSupportsInlineRayTracing   = bSupportsModernFeatures || bIsMetal;
    return Desc;
}

static void CompileAllShaders(StringView Arguments)
{
    StringView TargetName = Arguments;
    TargetName.TrimInline();

    ERHIType TargetRHI = RHI::IsInitialized() ? RHI::Device->GetRHIType() : ERHIType::Unknown;
    if (!TargetName.IsEmpty() && !ParseRHIType(TargetName, TargetRHI))
    {
        LOG_ERROR("Shaders.CompileAll: Unknown RHI '%s'", *String(TargetName));
        return;
    }

    FShaderCompiler* Compiler = FShaderCompiler::TryGet();

    const EShaderOutputLanguage OutputLanguage = RHI::GetShaderOutputLanguage(TargetRHI);
    const EShaderCompileRoute   Route          = Compiler ? Compiler->GetRoute(OutputLanguage) : EShaderCompileRoute::Unavailable;

    const bool bCanCompile = Route == EShaderCompileRoute::Remote || (Route == EShaderCompileRoute::Local && Compiler->CanCompileLocally(OutputLanguage));
    if (!bCanCompile)
    {
        LOG_ERROR("Shaders.CompileAll: %s (%s) cannot be compiled on this platform or by the remote shader compiler", ToString(TargetRHI), ToString(OutputLanguage));
        return;
    }

    struct FCompileWork
    {
        FShaderType*           Type;
        FShaderPermutationDesc Desc;
    };

    TArray<FCompileWork> Work;
    for (FShaderType* Type = FShaderType::GetTypeList(); Type; Type = Type->GetNext())
    {
        for (int32 PermutationID = 0; PermutationID < Type->GetPermutationCount(); ++PermutationID)
        {
            const FShaderPermutationDesc Desc = FShaderCache::CreateTargetPermutationDesc(TargetRHI, PermutationID);
            if (Type->ShouldCompilePermutation(Desc))
            {
                Work.Add({ Type, Desc });
            }
        }
    }

    LOG_INFO("Shaders.CompileAll: Compiling %d permutations for %s (%s)", Work.Size(), ToString(TargetRHI), ToString(OutputLanguage));

    AtomicInt32 NumFailed(0);
    Tasks::ParallelFor(Work.Size(), [&](int32 Index)
    {
        const FCompileWork& Item = Work[Index];

        FShaderCompilationEnvironment Environment;
        Item.Type->BuildCompilationEnvironment(Item.Desc, Environment);

        const FShaderCompileInfo CompileInfo(Item.Type->GetEntryPoint(), Environment.ShaderModel, Item.Type->GetStage(), OutputLanguage, Environment.Defines);

        TArray<uint8> ShaderCode;
        if (!FShaderBytecodeCache::CompileFromFile(Item.Type->GetSourceFile(), CompileInfo, ShaderCode, EShaderJobRecording::Skip))
        {
            LOG_ERROR("Shaders.CompileAll: %s permutation %d failed (%s, entry '%s')", Item.Type->GetName(), Item.Desc.PermutationID, Item.Type->GetSourceFile(), Item.Type->GetEntryPoint());
            NumFailed.Add(1);
        }
    });

    if (FShaderBytecodeCache* BytecodeCache = FShaderBytecodeCache::TryGet())
    {
        BytecodeCache->Save();
    }

    const int32 Failed = NumFailed.Load();
    if (Failed > 0)
    {
        LOG_ERROR("Shaders.CompileAll: %d of %d permutations failed for %s (%s)", Failed, Work.Size(), ToString(TargetRHI), ToString(OutputLanguage));
    }
    else
    {
        LOG_INFO("Shaders.CompileAll: All %d permutations compiled for %s (%s)", Work.Size(), ToString(TargetRHI), ToString(OutputLanguage));
    }
}

FShaderCache::FShaderCache()
    : Shaders()
    , RequestedPermutations()
    , bManifestDirty(false)
{
}

FShaderCache::~FShaderCache()
{
    Shaders.Clear();
}

bool FShaderCache::Initialize()
{
    ShaderCache = new FShaderCache();
    return true;
}

void FShaderCache::Release()
{
    if (ShaderCache)
    {
        // Startup never waits on the warm, but shutdown has to, or the workers outlive the cache they are filling.
        ShaderCache->PrewarmTask.Wait();
        ShaderCache->SaveManifest();

#if EDITOR_BUILD
        ShaderCache->SaveJobFile();
#endif

        delete ShaderCache;
        ShaderCache = nullptr;
    }
}

void FShaderCache::PrewarmAsync()
{
    if (!CVarPrewarm.GetValue())
    {
        return;
    }

    FShaderManifest Manifest;
    if (!Manifest.Load() || Manifest.IsEmpty())
    {
        return;
    }

    PrewarmTask = Tasks::Async([this, Manifest = ::Move(Manifest)]()
    {
        // One permutation per unit of work, so a type with a thousand of them does not sit on a single lane.
        TArray<TPair<FShaderType*, int32>> Work;
        for (const FShaderManifestEntry& Entry : Manifest.GetEntries())
        {
            for (int32 PermutationID : Entry.PermutationIDs)
            {
                Work.Emplace(Entry.Type, PermutationID);
            }
        }

        Tasks::ParallelFor(Work.Size(), [this, &Work](int32 Index)
        {
            (void)GetOrCompile(*Work[Index].First, Work[Index].Second);
        });

        LOG_INFO("[FShaderCache]: Pre-warmed %d shader permutations", Work.Size());
    });
}

void FShaderCache::RecordPermutation(FShaderType& Type, int32 PermutationID)
{
    TScopedLock Lock(RequestedPermutationsCS);

    bool bAlreadyRecorded = false;
    RequestedPermutations.FindOrAdd(&Type).Add(PermutationID, &bAlreadyRecorded);

    if (!bAlreadyRecorded)
    {
        bManifestDirty = true;
    }
}

void FShaderCache::SaveManifest()
{
    TScopedLock Lock(RequestedPermutationsCS);

    if (!bManifestDirty || !CVarSaveManifest.GetValue())
    {
        return;
    }

    FShaderManifest Manifest;
    RequestedPermutations.Foreach([&Manifest](FShaderType* Type, const TSet<int32>& PermutationIDs)
    {
        FShaderManifestEntry Entry;
        Entry.Type           = Type;
        Entry.PermutationIDs = PermutationIDs.GetValues();
        Manifest.AddEntry(::Move(Entry));
    });

    Manifest.Save();
    bManifestDirty = false;
}

#if EDITOR_BUILD
void FShaderCache::SaveJobFile()
{
    constexpr ERHIType TargetRHIs[] =
    {
        ERHIType::D3D11,
        ERHIType::D3D12,
        ERHIType::Vulkan,
        ERHIType::Metal,
    };

    const String FilePath = FShaderJobFile::GetDefaultFilePath();

    FShaderJobFile JobFile;
    String         LoadError;

    if (!JobFile.Load(FilePath, LoadError))
    {
        LOG_WARNING("[FShaderCache]: Rewriting '%s' from scratch: %s", *FilePath, *LoadError);
        JobFile = FShaderJobFile();
    }

    int32 NumAdded = 0;
    {
        TScopedLock Lock(RequestedPermutationsCS);

        RequestedPermutations.Foreach([&](FShaderType* Type, const TSet<int32>& PermutationIDs)
        {
            const TArray<int32> PermutationList = PermutationIDs.GetValues();

            for (ERHIType TargetRHI : TargetRHIs)
            {
                const EShaderOutputLanguage OutputLanguage = RHI::GetShaderOutputLanguage(TargetRHI);

                for (int32 PermutationID : PermutationList)
                {
                    const FShaderPermutationDesc Desc = CreateTargetPermutationDesc(TargetRHI, PermutationID);
                    if (!Type->ShouldCompilePermutation(Desc))
                    {
                        continue;
                    }

                    FShaderCompilationEnvironment Environment;
                    Type->BuildCompilationEnvironment(Desc, Environment);

                    const FShaderCompileInfo CompileInfo(Type->GetEntryPoint(), Environment.ShaderModel, Type->GetStage(), OutputLanguage, Environment.Defines);

                    FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo(Type->GetSourceFile(), CompileInfo);
                    Job.Name = String::Printf("%s#%d", Type->GetName(), PermutationID);
                    NumAdded += JobFile.Add(ToString(TargetRHI), ::Move(Job)) ? 1 : 0;
                }
            }
        });
    }

    // FXC lowers SM 6.0 to 6.2 requests to 5.0, so the standalone compiles only need their output language changed
    if (FShaderBytecodeCache* BytecodeCache = FShaderBytecodeCache::TryGet())
    {
        for (const FShaderCompileJob& Recorded : BytecodeCache->GetRecordedJobs())
        {
            for (ERHIType TargetRHI : TargetRHIs)
            {
                FShaderCompileJob Job = Recorded;
                Job.OutputLanguage = RHI::GetShaderOutputLanguage(TargetRHI);
                NumAdded += JobFile.Add(ToString(TargetRHI), ::Move(Job)) ? 1 : 0;
            }
        }
    }

    if (NumAdded > 0 && JobFile.Save(FilePath))
    {
        LOG_INFO("[FShaderCache]: Added %d shader jobs to '%s' (%d in total)", NumAdded, *FilePath, JobFile.GetEntries().Size());
    }
}
#endif

void FShaderCache::FlushCompiledShaders()
{
    {
        TScopedLock Lock(ShadersCS);
        Shaders.Clear();
    }

    if (FRHIPipelineStateCache* PipelineCache = FRHIPipelineStateCache::TryGet())
    {
        PipelineCache->FlushPipelineStates();
    }
}

void FShaderCache::EvictUnsupportedPermutations()
{
    TScopedLock Lock(ShadersCS);

    TArray<FShaderCacheKey> UnsupportedKeys;
    Shaders.Foreach([&UnsupportedKeys](const FShaderCacheKey& Key, const FRHIShaderRef& /* Shader */)
    {
        if (!Key.Type->ShouldCompilePermutation(CreatePermutationDesc(Key.PermutationID)))
        {
            UnsupportedKeys.Add(Key);
        }
    });

    for (const FShaderCacheKey& Key : UnsupportedKeys)
    {
        Shaders.Remove(Key);
    }
}

FShaderPermutationDesc FShaderCache::CreatePermutationDesc(int32 PermutationID)
{
    FShaderPermutationDesc Desc;
    Desc.PermutationID                      = PermutationID;
    Desc.bSupportsBindless                  = RHI::bSupportsBindless;
    Desc.bSupportsViewInstancing            = RHI::bSupportsViewInstancing;
    Desc.bSupportsRayTracing                = RenderSettings::IsRayTracingEnabled();
    Desc.bSupportsRayTracingPipeline        = RenderSettings::IsRayTracingEnabled() && RHI::bSupportsRayTracingPipeline;
    Desc.bSupportsInlineRayTracing          = RenderSettings::IsRayTracingEnabled() && RHI::bSupportsInlineRayTracing;
    Desc.bSupportsShaderExecutionReordering = RenderSettings::IsRayTracingEnabled() && RHI::bSupportsShaderExecutionReordering;
    return Desc;
}

FRHIShaderRef FShaderCache::GetOrCompile(FShaderType& Type, int32 PermutationID)
{
    RecordPermutation(Type, PermutationID);

    FShaderCacheKey Key;
    Key.Type          = &Type;
    Key.PermutationID = PermutationID;

    {
        TScopedLock Lock(ShadersCS);
        if (FRHIShaderRef* Existing = Shaders.Find(Key))
        {
            return *Existing;
        }
    }

    const FShaderPermutationDesc Desc = CreatePermutationDesc(PermutationID);
    if (!Type.ShouldCompilePermutation(Desc))
    {
        return FRHIShaderRef();
    }

    FShaderCompilationEnvironment Environment;
    Type.BuildCompilationEnvironment(Desc, Environment);

    const EShaderOutputLanguage OutputLanguage = RHI::GetShaderOutputLanguage();
    const FShaderCompileInfo    CompileInfo    = FShaderCompileInfo(Type.GetEntryPoint(), Environment.ShaderModel, Type.GetStage(), OutputLanguage, Environment.Defines);

    // DXBC is always SM 5.0 and FXC rejects the requests it cannot lower itself.
    if (OutputLanguage != EShaderOutputLanguage::DXBC && RHI::MaxShaderModel != EShaderModel::Unknown && Environment.ShaderModel > RHI::MaxShaderModel)
    {
        LOG_ERROR("'%s' requests Shader Model %s but the device supports at most %s", Type.GetSourceFile(), ToString(Environment.ShaderModel), ToString(RHI::MaxShaderModel));
    }

    TArray<uint8> ShaderCode;
    if (!FShaderBytecodeCache::CompileFromFile(Type.GetSourceFile(), CompileInfo, ShaderCode, EShaderJobRecording::Skip))
    {
        LOG_ERROR("Failed to compile %s permutation %d (%s, entry '%s')", Type.GetName(), PermutationID, Type.GetSourceFile(), Type.GetEntryPoint());
        return FRHIShaderRef();
    }

    FRHIShaderRef Shader = RHI::CreateShader(Type.GetStage(), ShaderCode);
    if (!Shader)
    {
        LOG_ERROR("Failed to create %s permutation %d for stage %s", Type.GetName(), PermutationID, ToString(Type.GetStage()));
        return FRHIShaderRef();
    }

    {
        TScopedLock Lock(ShadersCS);

        // Another thread may have compiled the same permutation while this one was unlocked, in which case the loser discards its result.
        if (FRHIShaderRef* Existing = Shaders.Find(Key))
        {
            return *Existing;
        }

        Shaders.Add(Key, Shader);
    }

    return Shader;
}
