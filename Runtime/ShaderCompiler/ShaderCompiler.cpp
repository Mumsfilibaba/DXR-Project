#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Filesystem/File.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/Debug.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Threading/ScopedLock.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCompiler/ShaderCompilerBackend.h"
#include "ShaderCompiler/ShaderCompilerStats.h"
#include "ShaderCompiler/ShaderPreprocessor.h"
#include "ShaderCompiler/DXC/DXCShaderCompiler.h"
#include "ShaderCompiler/FXC/FXCShaderCompiler.h"
#include "ShaderCompiler/Spirv/SpirvTransforms.h"
#include "ShaderCore/ShaderCode.h"

IMPLEMENT_ENGINE_MODULE(IModule, ShaderCompiler);

/** Hashed into ComputeCompileHash, bump when a reflector's output changes */
static constexpr uint32 GShaderReflectionVersion = 1;

static TAutoConsoleVariable<bool> CVarShaderDebug(
    "RHI.ShaderCompiler.Debug",
    "Compile shaders with debug information, embedded reflection and binding names, for PIX and RenderDoc. Ignored in release builds.",
    false);

static TAutoConsoleVariable<bool> CVarVerboseLogging(
    "RHI.ShaderCompiler.VerboseLogging",
    "Enable verbose logging in the ShaderCompiler",
    false);

static TAutoConsoleVariable<bool> CVarMapMin16FloatToFloat(
    "RHI.ShaderCompiler.MapMin16FloatToFloat",
    "Map the min16float type family to full-precision float on non-HLSL backends (works around DXC's SPIR-V "
    "RelaxedPrecision codegen bug). Disable to keep native min-precision types (also sets MIN16FLOAT_AVAILABLE).",
    true);

static TAutoConsoleVariable<String> CVarDumpPreprocessedDir(
    "RHI.ShaderCompiler.DumpPreprocessedDir",
    "When set, every shader is written to this directory after preprocessing and translation, with the defines it was compiled with",
    "");

static FAutoConsoleCommand CCmdDumpShaderCompileStats(
    "RHI.DumpShaderCompileStats",
    "Logs how many shaders the compiler has built and how long they took",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        if (FShaderCompiler* Compiler = FShaderCompiler::TryGet())
        {
            Compiler->LogCompileStats();
        }
    }));

class FScopedCompileTimer
{
public:
    FScopedCompileTimer(AtomicInt64& InNumCompiles, AtomicInt64& InTotalTimeNS)
        : NumCompiles(InNumCompiles)
        , TotalTimeNS(InTotalTimeNS)
        , StartTime(FPlatformTime::QueryPerformanceCounter())
    {
    }

    ~FScopedCompileTimer()
    {
        const uint64 Elapsed = FPlatformTime::QueryPerformanceCounter() - StartTime;
        const double Seconds = static_cast<double>(Elapsed) / static_cast<double>(FPlatformTime::QueryPerformanceFrequency());

        NumCompiles.Add(1);
        TotalTimeNS.Add(static_cast<int64>(Seconds * 1000.0 * 1000.0 * 1000.0));
    }

private:
    AtomicInt64& NumCompiles;
    AtomicInt64& TotalTimeNS;
    uint64       StartTime;
};

FShaderCompiler* FShaderCompiler::ShaderCompiler = nullptr;

bool FShaderCompiler::IsDebugInfoEnabled()
{
#if RELEASE_BUILD
    return false;
#else
    return CVarShaderDebug.GetValue();
#endif
}

FShaderCompiler::FShaderCompiler(const String& InAssetPath)
    : Backends()
    , AssetPath(InAssetPath)
    , NumCompiles(0)
    , TotalCompileTimeNS(0)
{
}

FShaderCompiler::~FShaderCompiler()
{
    for (FShaderCompilerBackend* Backend : Backends)
    {
        delete Backend;
    }

    Backends.Clear();
}

bool FShaderCompiler::Initialize(const String& InAssetPath)
{
    CHECK(ShaderCompiler == nullptr);

    ShaderCompiler = new FShaderCompiler(InAssetPath);
    if (!ShaderCompiler->InitializeBackends())
    {
        delete ShaderCompiler;
        ShaderCompiler = nullptr;
        return false;
    }

    return true;
}

void FShaderCompiler::Destroy()
{
    if (ShaderCompiler)
    {
        delete ShaderCompiler;
        ShaderCompiler = nullptr;
    }
}

bool FShaderCompiler::InitializeBackends()
{
    // DXC is required on every platform
    FDXCShaderCompiler* DXCBackend = new FDXCShaderCompiler();
    if (!DXCBackend->Initialize())
    {
        delete DXCBackend;
        return false;
    }

    Backends.Add(DXCBackend);

#if PLATFORM_WINDOWS
    // FXC is optional, without it only DXBC (D3D11RHI) becomes unavailable
    FFXCShaderCompiler* FXCBackend = new FFXCShaderCompiler();
    if (FXCBackend->Initialize())
    {
        Backends.Add(FXCBackend);
    }
    else
    {
        LOG_WARNING("[FShaderCompiler]: 'd3dcompiler_47' is not available, DXBC output is disabled");
        delete FXCBackend;
    }
#endif

    return true;
}

FShaderCompilerBackend* FShaderCompiler::FindBackend(EShaderOutputLanguage OutputLanguage) const
{
    for (FShaderCompilerBackend* Backend : Backends)
    {
        if (Backend->SupportsOutputLanguage(OutputLanguage))
        {
            return Backend;
        }
    }

    return nullptr;
}

bool FShaderCompiler::IsOutputLanguageSupported(EShaderOutputLanguage OutputLanguage) const
{
    return FindBackend(OutputLanguage) != nullptr;
}

TArray<EShaderOutputLanguage> FShaderCompiler::GetSupportedOutputLanguages() const
{
    constexpr EShaderOutputLanguage OutputLanguages[] =
    {
        EShaderOutputLanguage::DXBC,
        EShaderOutputLanguage::DXIL,
        EShaderOutputLanguage::SPIRV,
        EShaderOutputLanguage::MSL,
    };

    TArray<EShaderOutputLanguage> Result;
    for (EShaderOutputLanguage OutputLanguage : OutputLanguages)
    {
        if (IsShaderOutputLanguageSupportedByPlatform(OutputLanguage) && IsOutputLanguageSupported(OutputLanguage))
        {
            Result.Add(OutputLanguage);
        }
    }

    return Result;
}

void FShaderCompiler::BuildCompileDefines(const FShaderCompileInfo& CompileInfo, TArray<FShaderDefine>& OutDefines) const
{
    // Add defines that identify the target shader backend
    OutDefines.Emplace("SHADER_BACKEND_D3D12", "(1)");
    OutDefines.Emplace("SHADER_BACKEND_VULKAN", "(2)");
    OutDefines.Emplace("SHADER_BACKEND_METAL", "(3)");
    OutDefines.Emplace("SHADER_BACKEND_D3D11", "(4)");

    switch (CompileInfo.OutputLanguage)
    {
        case EShaderOutputLanguage::DXIL:  OutDefines.Emplace("SHADER_BACKEND", "SHADER_BACKEND_D3D12");  break;
        case EShaderOutputLanguage::MSL:   OutDefines.Emplace("SHADER_BACKEND", "SHADER_BACKEND_METAL");  break;
        case EShaderOutputLanguage::SPIRV: OutDefines.Emplace("SHADER_BACKEND", "SHADER_BACKEND_VULKAN"); break;
        case EShaderOutputLanguage::DXBC:  OutDefines.Emplace("SHADER_BACKEND", "SHADER_BACKEND_D3D11");  break;
        default:                           OutDefines.Emplace("SHADER_BACKEND", "(0)");                   break;
    }

    // The mapping only applies to the SPIR-V based outputs
    const bool bIsSpirvBased = CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV || CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL;

    const bool bMapMin16FloatToFloat = bIsSpirvBased && CVarMapMin16FloatToFloat.GetValue();
    if (bMapMin16FloatToFloat)
    {
        OutDefines.Emplace("min16float", "float");
        OutDefines.Emplace("min16float2", "float2");
        OutDefines.Emplace("min16float3", "float3");
        OutDefines.Emplace("min16float4", "float4");
        OutDefines.Emplace("MIN16FLOAT_AVAILABLE", "(0)");
    }
    else
    {
        OutDefines.Emplace("MIN16FLOAT_AVAILABLE", "(1)");
    }

    for (const FShaderDefine& Define : CompileInfo.Defines)
    {
        OutDefines.Emplace(Define);
    }
}

bool FShaderCompiler::CompileFromFile(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies)
{
    // Add asset-path to the filename
    const String FilePath = AssetPath + '/' + Filename;

    // Store the ShaderFile in this array
    TArray<CHAR> Text;

    {
        // Open the file
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(FilePath);
        if (!FileHandle)
        {
            LOG_ERROR("[FShaderCompiler]: Failed to open file '%s'", *Filename);
            return false;
        }

        // Read the full file as a text-file
        if (!File::ReadTextFile(FileHandle.Get(), Text))
        {
            LOG_ERROR("[FShaderCompiler]: Failed to read file '%s'", *Filename);
            return false;
        }
    }

    // The pre-processor only reports the files it includes, so the shader itself is added here
    if (OutDependencies)
    {
        OutDependencies->Emplace(FilePath);
    }

    // Compile the source
    const String Source(Text.Data(), Text.Size());
    return Compile(Source, FilePath, CompileInfo, OutByteCode, OutDependencies);
}

bool FShaderCompiler::CompileFromSource(const String& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies)
{
    return Compile(ShaderSource, "", CompileInfo, OutByteCode, OutDependencies);
}

uint64 FShaderCompiler::ComputeCompileHash(const String& SourceFile, const FShaderCompileInfo& CompileInfo) const
{
    uint64 Hash = THash<String>::GetHash(SourceFile);

    HashCombine(Hash, CompileInfo.EntryPoint);
    HashCombine(Hash, CompileInfo.ShaderModel);
    HashCombine(Hash, CompileInfo.ShaderStage);
    HashCombine(Hash, CompileInfo.OutputLanguage);

    TArray<FShaderDefine> Defines;
    BuildCompileDefines(CompileInfo, Defines);

    for (const FShaderDefine& Define : Defines)
    {
        HashCombine(Hash, Define.Define);
        HashCombine(Hash, Define.Value);
    }

    HashCombine(Hash, FShaderPreprocessor::Version);
    HashCombine(Hash, CompileInfo.bDebugInfo);
    HashCombine(Hash, FShaderCodeHeader::CurrentVersion);
    HashCombine(Hash, FSpirvTransforms::Version);
    HashCombine(Hash, GShaderReflectionVersion);

    if (const FShaderCompilerBackend* Backend = FindBackend(CompileInfo.OutputLanguage))
    {
        HashCombine(Hash, THash<String>::GetHash(String(Backend->GetName())));
        Backend->HashCompileSettings(CompileInfo, AssetPath + "/Shaders", Hash);
    }

    return Hash;
}

void FShaderCompiler::LogCompileStats() const
{
    const int64     Count = NumCompiles.Load();
    const FTimespan Total = FTimespan(static_cast<uint64>(TotalCompileTimeNS.Load()));

    if (Count <= 0)
    {
        LOG_INFO("[FShaderCompiler]: No shaders compiled");
        return;
    }

    LOG_INFO("[FShaderCompiler]: Compiled %lld shaders in %.2f seconds (%.1f ms average)",
        Count, Total.AsSeconds(), Total.AsMilliseconds() / static_cast<double>(Count));
}

bool FShaderCompiler::Compile(const String& ShaderSource, const String& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies)
{
    FScopedCompileTimer CompileTimer(NumCompiles, TotalCompileTimeNS);

    STAT_ADD(STAT_Shader_CompileCount, 1);
    OutByteCode.Clear();

    FShaderCompilerBackend* Backend = FindBackend(CompileInfo.OutputLanguage);
    if (!Backend)
    {
        LOG_ERROR("[FShaderCompiler]: No compiler backend can produce %s", ToString(CompileInfo.OutputLanguage));
        return false;
    }

    TArray<FShaderDefine> Defines;
    BuildCompileDefines(CompileInfo, Defines);

    // Log all the defines that are used for this shader-compilation
    const bool bVerboseLogging = CVarVerboseLogging.GetValue();
    if (bVerboseLogging)
    {
        if (!FilePath.IsEmpty())
        {
            LOG_INFO("[FShaderCompiler]: Compiling shader '%s' with %s, using the following defines:", *FilePath, Backend->GetName());
        }
        else
        {
            LOG_INFO("[FShaderCompiler]: Compiling shader with %s, using the following defines:", Backend->GetName());
        }

        for (const FShaderDefine& Define : Defines)
        {
            LOG_INFO("    %s = %s", *Define.Define, *Define.Value);
        }
    }

    const String IncludeDir = AssetPath + "/Shaders";

    // Both compilers get source the engine has preprocessed, so macros and includes behave the same for every backend
    FShaderPreprocessor Preprocessor(IncludeDir);
    for (const FShaderDefine& Define : Defines)
    {
        Preprocessor.AddDefine(Define.Define, Define.Value);
    }

    FShaderPreprocessorOutput Preprocessed;
    const bool bPreprocessed = Preprocessor.Preprocess(FilePath, StringView(ShaderSource), Preprocessed) && Backend->TranslateSource(Preprocessed);

    if (OutDependencies)
    {
        for (const String& Dependency : Preprocessed.Dependencies)
        {
            if (!OutDependencies->Contains(Dependency))
            {
                OutDependencies->Emplace(Dependency);
            }
        }
    }

    if (!bPreprocessed)
    {
        LOG_ERROR("[FShaderCompiler]: FAILED to preprocess for %s with error: %s", Backend->GetName(), *Preprocessed.Errors);

        if (Debug::IsDebuggerPresent())
        {
            DEBUG_BREAK();
        }

        return false;
    }

    const String PreprocessedSource = Preprocessed.Render();

    const String DumpDir = CVarDumpPreprocessedDir.GetValue();
    if (!DumpDir.IsEmpty())
    {
        DumpPreprocessedSource(DumpDir, FilePath, CompileInfo, Defines, PreprocessedSource);
    }

    FShaderCompileRequest Request;
    Request.CompileInfo     = &CompileInfo;
    Request.Source          = StringView(PreprocessedSource);
    Request.FilePath        = FilePath;
    Request.IncludeDir      = IncludeDir;
    Request.bVerboseLogging = bVerboseLogging;

    FShaderCompileResult Result;
    const bool bCompiled = Backend->Compile(Request, Result);

    if (!bCompiled)
    {
        if (!Result.Messages.IsEmpty())
        {
            LOG_ERROR("[FShaderCompiler]: %s FAILED to compile with error: %s", Backend->GetName(), *Result.Messages);
        }
        else
        {
            LOG_ERROR("[FShaderCompiler]: %s FAILED to compile with. Unknown ERROR.", Backend->GetName());
        }

        // Callers handle the failure, so only stop when someone is there to look at the error
        if (Debug::IsDebuggerPresent())
        {
            DEBUG_BREAK();
        }

        return false;
    }

    if (bVerboseLogging)
    {
        if (!Result.Messages.IsEmpty())
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader with the following output: %s", *Result.Messages);
        }
        else
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader.");
        }
    }

    // Dump the metal file to disk
    if (CompileInfo.bDebugInfo && CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL && !FilePath.IsEmpty())
    {
        if (!DumpContentToFile(Result.ByteCode, FilePath + "_" + ToString(CompileInfo.ShaderStage) + ".metal"))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    const EShaderCodeFlags CodeFlags = CompileInfo.bDebugInfo ? EShaderCodeFlags::DebugInfo : EShaderCodeFlags::None;

    String WriteError;
    if (!FShaderCodeWriter::Write(CompileInfo.OutputLanguage, CompileInfo.ShaderStage, CodeFlags, Result.Reflection, Result.ByteCode, OutByteCode, &WriteError))
    {
        LOG_ERROR("[FShaderCompiler]: Failed to write the shader code container for '%s': %s", FilePath.IsEmpty() ? *CompileInfo.EntryPoint : *FilePath, *WriteError);
        return false;
    }

    if (bVerboseLogging)
    {
        LOG_INFO("[FShaderCompiler]: Compiled Size: %u Bytes", OutByteCode.Size());
    }

    if (OutByteCode.IsEmpty())
    {
        LOG_WARNING("[FShaderCompiler]: Resulting bytecode is empty");
    }

    if (bVerboseLogging && OutDependencies)
    {
        LOG_INFO("[FShaderCompiler]: Compiled from the following files:");

        for (const String& Dependency : *OutDependencies)
        {
            LOG_INFO("    %s", *Dependency);
        }
    }

    // If verbose logging is turned off, atleast log that we successfully compiled the shader
    if (!bVerboseLogging)
    {
        if (!FilePath.IsEmpty())
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader '%s'", *FilePath);
        }
        else
        {
            LOG_INFO("[FShaderCompiler]: Successfully compiled shader");
        }
    }

    return true;
}

void FShaderCompiler::DumpPreprocessedSource(const String& DumpDir, const String& FilePath, const FShaderCompileInfo& CompileInfo, const TArray<FShaderDefine>& Defines, const String& Source)
{
    uint64 DefinesHash = 0;
    for (const FShaderDefine& Define : Defines)
    {
        HashCombine(DefinesHash, Define.Define);
        HashCombine(DefinesHash, Define.Value);
    }

    const String BaseName = FilePath.IsEmpty() ? String("ShaderSource") : File::ExtractFilenameWithoutExtension(FilePath);
    const String Filename = String::Printf("%s/%s_%s_%s_%016llx.hlsl", *DumpDir, *BaseName, *CompileInfo.EntryPoint, ToString(CompileInfo.OutputLanguage), DefinesHash);

    String Contents = String::Printf("// Source: %s\n// Entry: %s\n// Stage: %s\n// Model: %s\n", *FilePath, *CompileInfo.EntryPoint, ToString(CompileInfo.ShaderStage), ToString(CompileInfo.ShaderModel));
    for (const FShaderDefine& Define : Defines)
    {
        Contents += String::Printf("// Define: %s=%s\n", *Define.Define, *Define.Value);
    }

    Contents += Source;

    TScopedLock Lock(DumpCS);

    if (!File::CreateDirectoryTree(DumpDir))
    {
        LOG_ERROR("[FShaderCompiler]: Failed to create '%s'", *DumpDir);
        return;
    }

    TFileRef<IPlatformFile> Output = FPlatformFile::OpenForWrite(Filename);
    if (!Output || !File::WriteTextFile(Output.Get(), Contents))
    {
        LOG_ERROR("[FShaderCompiler]: Failed to write '%s'", *Filename);
    }
}

bool FShaderCompiler::DumpContentToFile(const TArray<uint8>& ByteCode, const String& Filename)
{
    // Permutations of one shader all dump to the same path, so concurrent compiles would otherwise interleave in the file.
    TScopedLock Lock(DumpCS);

    TFileRef<IPlatformFile> Output = FPlatformFile::OpenForWrite(Filename);
    if (!Output)
    {
        LOG_ERROR("[FShaderCompiler]: Failed to open file '%s'", *Filename);
        return false;
    }

    Output->Write(ByteCode.Data(), ByteCode.Size());
    return true;
}
