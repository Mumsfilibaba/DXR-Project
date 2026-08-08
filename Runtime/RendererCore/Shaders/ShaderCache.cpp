#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Tasks/ParallelFor.h"
#include "Core/Tasks/Tasks.h"
#include "RendererCore/Shaders/ShaderBytecodeCache.h"
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/Shaders/ShaderManifest.h"
#include "RHI/RHI.h"
#include "RHI/RHIPipelineStateCache.h"
#include "RHI/ShaderCompiler.h"

FShaderCache* FShaderCache::GShaderCache = nullptr;

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
    GShaderCache = new FShaderCache();
    return true;
}

void FShaderCache::Release()
{
    if (GShaderCache)
    {
        // Startup never waits on the warm, but shutdown has to, or the workers outlive the cache they are filling.
        GShaderCache->PrewarmTask.Wait();
        GShaderCache->SaveManifest();

        delete GShaderCache;
        GShaderCache = nullptr;
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

FShaderPermutationDesc FShaderCache::MakePermutationDesc(int32 PermutationID)
{
    FShaderPermutationDesc Desc;
    Desc.PermutationID                      = PermutationID;
    Desc.bSupportsBindless                  = RHI::bSupportsBindless;
    Desc.bSupportsRayTracing                = RHI::bSupportsRayTracing;
    Desc.bSupportsShaderExecutionReordering = RHI::bSupportsShaderExecutionReordering;
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

    const FShaderPermutationDesc Desc = MakePermutationDesc(PermutationID);
    if (!Type.ShouldCompilePermutation(Desc))
    {
        return FRHIShaderRef();
    }

    FShaderCompilationEnvironment Environment;
    Type.BuildCompilationEnvironment(Desc, Environment);

    const FShaderCompileInfo CompileInfo(Type.GetEntryPoint(), Environment.ShaderModel, Type.GetStage(), Environment.Defines);
    const uint64             CompileHash = FShaderCompiler::Get().ComputeCompileHash(Type.GetSourceFile(), CompileInfo);

    TArray<uint8> ShaderCode;
    if (!FShaderBytecodeCache::Get().Find(CompileHash, ShaderCode))
    {
        TArray<String> Dependencies;
        if (!FShaderCompiler::Get().CompileFromFile(Type.GetSourceFile(), CompileInfo, ShaderCode, &Dependencies))
        {
            LOG_ERROR("Failed to compile %s permutation %d (%s, entry '%s')", Type.GetName(), PermutationID, Type.GetSourceFile(), Type.GetEntryPoint());
            return FRHIShaderRef();
        }

        FShaderBytecodeCache::Get().Add(CompileHash, ShaderCode, Dependencies);
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
