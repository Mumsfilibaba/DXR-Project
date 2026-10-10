#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Filesystem/File.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/Debug.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Memory/Memory.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "ShaderCompiler/ShaderCompileJob.h"
#include "ShaderCompiler/ShaderCompilerBackend.h"
#include "ShaderCompiler/ShaderCompilerStats.h"
#include "ShaderCompiler/ShaderPreprocessor.h"
#include "ShaderCompiler/ShaderSourceHash.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerClient.h"
#include "ShaderCompiler/Remote/RemoteShaderCompilerProtocol.h"
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

static TAutoConsoleVariable<bool> CVarUseRemote(
    "RHI.ShaderCompiler.UseRemote",
    "Send shader compiles to the ShaderCompiler -server at RHI.ShaderCompiler.RemoteHost instead of compiling in the engine",
    false);

static TAutoConsoleVariable<String> CVarRemoteHost(
    "RHI.ShaderCompiler.RemoteHost",
    "Host name, IPv4 or IPv6 address of the ShaderCompiler -server",
    RemoteShaderCompilerProtocol::LoopbackAddress);

static TAutoConsoleVariable<int32> CVarRemotePort(
    "RHI.ShaderCompiler.RemotePort",
    "TCP port of the ShaderCompiler -server",
    RemoteShaderCompilerProtocol::DefaultPort,
    1, 65535);

static TAutoConsoleVariable<bool> CVarRemoteFallbackToLocal(
    "RHI.ShaderCompiler.RemoteFallbackToLocal",
    "Compile in the engine when the remote server is unreachable or cannot produce the output language",
    true);

static TAutoConsoleVariable<int32> CVarRemoteTimeout(
    "RHI.ShaderCompiler.RemoteTimeout",
    "Milliseconds to wait for one remote compile before treating the server as unreachable",
    60000,
    1000, 600000);

static constexpr double GRemoteReconnectIntervalSeconds   = 5.0;
static constexpr uint32 GRemoteConnectTimeoutMilliseconds = 2000;

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
    , RemoteClient(nullptr)
    , RemoteClientAddress()
    , NextRemoteConnectCycles(0)
{
}

FShaderCompiler::~FShaderCompiler()
{
    if (RemoteClient)
    {
        RemoteClient->Disconnect();
        delete RemoteClient;
        RemoteClient = nullptr;
    }

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
    const bool bIsSpirvBased         = CompileInfo.OutputLanguage == EShaderOutputLanguage::SPIRV || CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL;
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
    const EShaderCompileRoute Route = GetRoute(CompileInfo.OutputLanguage);
    if (Route == EShaderCompileRoute::Unavailable)
    {
        LOG_ERROR("[FShaderCompiler]: The remote shader compiler cannot produce %s for '%s'", ToString(CompileInfo.OutputLanguage), *Filename);
        return false;
    }

    if (Route == EShaderCompileRoute::Remote)
    {
        const ERemoteCompileStatus Status = CompileOnRemote(*GetRemoteClient(), Filename, CompileInfo, OutByteCode, OutDependencies);
        if (Status != ERemoteCompileStatus::Unreachable)
        {
            return Status == ERemoteCompileStatus::Succeeded;
        }

        if (!CVarRemoteFallbackToLocal.GetValue() || !IsOutputLanguageSupported(CompileInfo.OutputLanguage))
        {
            LOG_ERROR("[FShaderCompiler]: The remote shader compiler is unreachable and '%s' cannot be compiled locally", *Filename);
            return false;
        }

        LOG_WARNING("[FShaderCompiler]: The remote shader compiler is unreachable, compiling '%s' locally", *Filename);
    }

    const String FilePath = ResolveSourcePath(Filename);

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
    const String         Source(Text.Data(), Text.Size());
    const TArray<String> IncludeDirs = ResolveIncludeDirs(CompileInfo);
    return CompileLocal(Source, FilePath, AssetPath + "/Shaders", TArrayView<const String>(IncludeDirs), CompileInfo, nullptr, false, OutByteCode, OutDependencies, nullptr);
}

bool FShaderCompiler::CompileFromSource(const String& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies)
{
    const TArray<String> IncludeDirs = ResolveIncludeDirs(CompileInfo);
    return CompileLocal(ShaderSource, "", AssetPath + "/Shaders", TArrayView<const String>(IncludeDirs), CompileInfo, nullptr, false, OutByteCode, OutDependencies, nullptr);
}

uint64 FShaderCompiler::ComputeCompileHash(const String& SourceFile, const FShaderCompileInfo& CompileInfo) const
{
    return ComputeCompileHash(SourceFile, CompileInfo, GetIdentityForRoute(CompileInfo.OutputLanguage));
}

uint64 FShaderCompiler::ComputeCompileHash(const String& SourceFile, const FShaderCompileInfo& CompileInfo, const FShaderCompilerIdentity& Identity) const
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

    for (const String& IncludeDir : CompileInfo.IncludeDirs)
    {
        HashCombine(Hash, IncludeDir);
    }

    HashCombine(Hash, FShaderPreprocessor::Version);
    HashCombine(Hash, CompileInfo.bDebugInfo);
    HashCombine(Hash, FShaderCodeHeader::CurrentVersion);
    HashCombine(Hash, FSpirvTransforms::Version);
    HashCombine(Hash, GShaderReflectionVersion);

    if (Identity.IsValid())
    {
        HashCombine(Hash, Identity.GetHash());
    }

    return Hash;
}

FShaderCompilerIdentity FShaderCompiler::GetLocalIdentity(EShaderOutputLanguage OutputLanguage) const
{
    const FShaderCompilerBackend* Backend = FindBackend(OutputLanguage);
    return Backend ? Backend->GetIdentity() : FShaderCompilerIdentity();
}

FShaderCompilerIdentity FShaderCompiler::GetIdentityForRoute(EShaderOutputLanguage OutputLanguage) const
{
    if (GetRoute(OutputLanguage) == EShaderCompileRoute::Remote)
    {
        if (TOptional<FShaderCompilerIdentity> RemoteIdentity = GetRemoteClient()->GetIdentity(OutputLanguage))
        {
            return *RemoteIdentity;
        }
    }

    return GetLocalIdentity(OutputLanguage);
}

class FRecordingSourceProvider final : public IShaderSourceProvider
{
public:
    NODISCARD virtual bool FileExists(const String& Path) const override final
    {
        return FDiskShaderSourceProvider::Get().FileExists(Path);
    }

    virtual bool ReadFile(const String& Path, TArray<CHAR>& OutText) const override final
    {
        if (!FDiskShaderSourceProvider::Get().ReadFile(Path, OutText))
        {
            return false;
        }

        if (!Files.Contains(Path))
        {
            Order.Add(Path);
            Files.Add(Path, OutText);
        }

        return true;
    }

    mutable TArray<String>             Order;
    mutable TMap<String, TArray<CHAR>> Files;
};

static String ReplaceAll(const String& Text, const String& From, const String& To)
{
    if (Text.IsEmpty() || From.IsEmpty())
    {
        return Text;
    }

    String Result;
    int32  Start = 0;
    while (Start < Text.Length())
    {
        const int32 Found = Text.Find(*From, Start);
        if (Found == String::InvalidIndex || Found < Start)
        {
            break;
        }

        Result.Append(Text.Data() + Start, Found - Start);
        Result.Append(To);
        Start = Found + From.Length();
    }

    if (Start < Text.Length())
    {
        Result.Append(Text.Data() + Start, Text.Length() - Start);
    }

    return Result;
}

class FShaderSourceRoots
{
public:
    FShaderSourceRoots(const String& AssetPath, const String& InFilePath, TArrayView<const String> InIncludeDirs)
        : FilePath(InFilePath)
        , IncludeDirs()
    {
        AddRoot(String(), AssetPath);
     
        for (int32 Index = 0; Index < InIncludeDirs.Size(); ++Index)
        {
            IncludeDirs.Add(InIncludeDirs[Index]);
            AddRoot(String::Printf("@Include%d", Index), InIncludeDirs[Index]);
        }

        AddRoot("@Source", File::GetDirectoryOf(FilePath));

        MakePortable(FilePath, PortableFilePath);
        for (const String& IncludeDir : IncludeDirs)
        {
            String PortableDir;
            if (!MakePortable(IncludeDir, PortableDir) || PortableDir.IsEmpty())
            {
                Error = String::Printf("The include directory '%s' is the asset directory itself, pass its Shaders folder or a folder below it instead", *IncludeDir);
            }

            PortableIncludeDirs.Add(::Move(PortableDir));
        }
    }

    /** @return False when no folder holds Path, which happens for a relative include that climbs out of every folder */
    bool MakePortable(const String& Path, String& OutPath) const
    {
        const String CollapsedPath = RemoteShaderCompilerProtocol::CollapsePath(Path) + '/';

        const FRoot* Outermost = nullptr;
        for (const FRoot& Root : Roots)
        {
            if (CollapsedPath.StartsWith(Root.Prefix) && (!Outermost || Root.Prefix.Length() < Outermost->Prefix.Length()))
            {
                Outermost = &Root;
            }
        }

        if (!Outermost)
        {
            return false;
        }

        // The '/' added above is dropped again, a folder that is a root itself becomes the bare root name
        const int32  RelativeLength = CollapsedPath.Length() - Outermost->Prefix.Length() - 1;
        const String Relative       = RelativeLength > 0 ? String(CollapsedPath.Data() + Outermost->Prefix.Length(), RelativeLength) : String();

        if (Outermost->Name.IsEmpty())
        {
            OutPath = Relative;
        }
        else
        {
            OutPath = Relative.IsEmpty() ? Outermost->Name : (Outermost->Name + '/' + Relative);
        }

        return true;
    }

    /** @return The path on this machine of a name MakePortable gave, which is what a server reports dependencies with */
    NODISCARD String MakeLocal(const String& PortablePath) const
    {
        for (const FRoot& Root : Roots)
        {
            if (!Root.Name.IsEmpty() && PortablePath.StartsWith(Root.Name + '/'))
            {
                return Root.Prefix + String(PortablePath.Data() + Root.Name.Length() + 1, PortablePath.Length() - Root.Name.Length() - 1);
            }
        }

        return Roots[0].Prefix + PortablePath;
    }

    /** @return Text with the root names replaced by their folders, so errors from a server point at files on this machine */
    NODISCARD String LocalizeMessages(const String& Text) const
    {
        String Result = Text;
        for (const FRoot& Root : Roots)
        {
            if (!Root.Name.IsEmpty())
            {
                Result = ReplaceAll(Result, Root.Name + '/', Root.Prefix);
            }
        }

        return Result;
    }

    NODISCARD const String& GetFilePath() const
    {
        return FilePath;
    }

    NODISCARD const TArray<String>& GetIncludeDirs() const
    {
        return IncludeDirs;
    }

    NODISCARD const String& GetPortableFilePath() const
    {
        return PortableFilePath;
    }

    NODISCARD const TArray<String>& GetPortableIncludeDirs() const
    {
        return PortableIncludeDirs;
    }

    /** @return Empty when every include directory has a name */
    NODISCARD const String& GetError() const
    {
        return Error;
    }

private:
    struct FRoot
    {
        String Name;

        /** Collapsed, with a trailing '/' */
        String Prefix;
    };

    void AddRoot(const String& Name, const String& Directory)
    {
        FRoot& Root = Roots.Emplace();
        Root.Name   = Name;
        Root.Prefix = RemoteShaderCompilerProtocol::CollapsePath(Directory) + '/';
    }

    String         FilePath;
    TArray<String> IncludeDirs;
    TArray<FRoot>  Roots;
    String         PortableFilePath;
    TArray<String> PortableIncludeDirs;
    String         Error;
};

String FShaderCompiler::ResolveSourcePath(const String& Path) const
{
    return FPlatformFile::IsPathRelative(*Path) ? (AssetPath + '/' + Path) : Path;
}

TArray<String> FShaderCompiler::ResolveIncludeDirs(const FShaderCompileInfo& CompileInfo) const
{
    TArray<String> IncludeDirs;
    IncludeDirs.Reserve(CompileInfo.IncludeDirs.Size());
    for (const String& IncludeDir : CompileInfo.IncludeDirs)
    {
        IncludeDirs.Add(ResolveSourcePath(IncludeDir));
    }

    return IncludeDirs;
}

bool FShaderCompiler::CollectSources(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<FShaderSourceFile>& OutFiles, String& OutErrors) const
{
    const TArray<String>     IncludeDirs = ResolveIncludeDirs(CompileInfo);
    const FShaderSourceRoots Roots(AssetPath, ResolveSourcePath(Filename), TArrayView<const String>(IncludeDirs));
    return CollectSources(Roots, CompileInfo, OutFiles, OutErrors);
}

bool FShaderCompiler::CollectSources(const FShaderSourceRoots& Roots, const FShaderCompileInfo& CompileInfo, TArray<FShaderSourceFile>& OutFiles, String& OutErrors) const
{
    if (!Roots.GetError().IsEmpty())
    {
        OutErrors = Roots.GetError();
        return false;
    }

    const String& FilePath = Roots.GetFilePath();

    FRecordingSourceProvider Recorder;

    TArray<CHAR> RootText;
    if (!Recorder.ReadFile(FilePath, RootText))
    {
        OutErrors = String::Printf("Failed to open '%s'", *FilePath);
        return false;
    }

    TArray<FShaderDefine> Defines;
    BuildCompileDefines(CompileInfo, Defines);

    FShaderPreprocessor Preprocessor(AssetPath + "/Shaders", &Recorder);
    for (const String& IncludeDir : Roots.GetIncludeDirs())
    {
        Preprocessor.AddIncludeDir(IncludeDir);
    }

    for (const FShaderDefine& Define : Defines)
    {
        Preprocessor.AddDefine(Define.Define, Define.Value);
    }

    FShaderPreprocessorOutput Preprocessed;
    if (!Preprocessor.Preprocess(FilePath, StringView(RootText.Data(), RootText.Size()), Preprocessed))
    {
        OutErrors = Preprocessed.Errors;
        return false;
    }

    for (const String& Path : Recorder.Order)
    {
        String SourcePath;
        if (!Roots.MakePortable(Path, SourcePath))
        {
            OutErrors = String::Printf("'%s' is outside the asset directory, the include directories and the shader's folder, so it cannot be sent to a remote compiler", *Path);
            return false;
        }

        const TArray<CHAR>& Text = *Recorder.Files.Find(Path);

        // File::ReadTextFile appends a terminator, which the receiving provider adds back
        int32 TextLength = Text.Size();
        while (TextLength > 0 && Text[TextLength - 1] == '\0')
        {
            TextLength--;
        }

        FShaderSourceFile SourceFile;
        SourceFile.Path      = SourcePath;
        SourceFile.LocalPath = Path;
        SourceFile.Contents.Resize(TextLength);
        if (TextLength > 0)
        {
            Memory::Memcpy(SourceFile.Contents.Data(), Text.Data(), TextLength);
        }

        ShaderSourceHash::NormalizeLineEndings(SourceFile.Contents);
        SourceFile.Hash = ShaderSourceHash::Compute(SourceFile.Contents);

        bool bAlreadyCollected = false;
        for (const FShaderSourceFile& Existing : OutFiles)
        {
            bAlreadyCollected |= Existing.Path == SourceFile.Path;
        }

        if (!bAlreadyCollected)
        {
            OutFiles.Emplace(::Move(SourceFile));
        }
    }

    return true;
}

bool FShaderCompiler::CompileJob(FShaderCompileJob& Job, const String& SourceRoot, const IShaderSourceProvider& Sources, TArray<uint8>& OutShaderCode, TArray<String>& OutDependencies, String& OutMessages)
{
    const String FilePath = SourceRoot + '/' + Job.SourceFile;

    TArray<CHAR> Text;
    if (!Sources.ReadFile(FilePath, Text))
    {
        OutMessages = String::Printf("'%s' was not sent with the request", *Job.SourceFile);
        return false;
    }

    TArray<String> Dependencies;
    Dependencies.Emplace(FilePath);

    TArray<String> IncludeDirs;
    for (const String& IncludeDir : Job.IncludeDirs)
    {
        IncludeDirs.Add(SourceRoot + '/' + IncludeDir);
    }

    const FShaderCompileInfo CompileInfo = Job.ToCompileInfo();
    const bool bCompiled = CompileLocal(String(Text.Data(), Text.Size()), FilePath, SourceRoot + "/Shaders", TArrayView<const String>(IncludeDirs), CompileInfo,
        &Sources, Job.bHasEngineDefines, OutShaderCode, &Dependencies, &OutMessages);

    const String RootPrefix = SourceRoot + '/';
    for (const String& Dependency : Dependencies)
    {
        const String RelativePath = Dependency.StartsWith(RootPrefix) ? String(Dependency.Data() + RootPrefix.Length(), Dependency.Length() - RootPrefix.Length()) : Dependency;
        if (!OutDependencies.Contains(RelativePath))
        {
            OutDependencies.Emplace(RelativePath);
        }
    }

    OutMessages = ReplaceAll(OutMessages, RootPrefix, "");
    return bCompiled;
}

ERemoteCompileStatus FShaderCompiler::CompileOnRemote(FRemoteShaderCompilerClient& Client, const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies)
{
    FScopedCompileTimer CompileTimer(NumCompiles, TotalCompileTimeNS);
    OutByteCode.Clear();

    const TArray<String>     IncludeDirs = ResolveIncludeDirs(CompileInfo);
    const FShaderSourceRoots Roots(AssetPath, ResolveSourcePath(Filename), TArrayView<const String>(IncludeDirs));

    TArray<FShaderSourceFile> Sources;
    String                    Errors;
    if (!CollectSources(Roots, CompileInfo, Sources, Errors))
    {
        LOG_ERROR("[FShaderCompiler]: FAILED to preprocess '%s' for the remote compiler with error: %s", *Filename, *Errors);
        return ERemoteCompileStatus::CompileFailed;
    }

    FShaderCompileJob Job = FShaderCompileJob::FromCompileInfo(Roots.GetPortableFilePath(), CompileInfo);
    Job.IncludeDirs = Roots.GetPortableIncludeDirs();
    Job.Defines.Clear();

    BuildCompileDefines(CompileInfo, Job.Defines);
    Job.bHasEngineDefines = true;

    FRemoteCompileResult Result;

    const ERemoteCompileStatus Status = Client.Compile(Job, Sources, Result, static_cast<uint32>(CVarRemoteTimeout.GetValue()));
    if (Status == ERemoteCompileStatus::CompileFailed)
    {
        LOG_ERROR("[FShaderCompiler]: %s FAILED to compile '%s' with error: %s", *Client.GetPeerName(), *Filename, *Roots.LocalizeMessages(Result.Messages));
    }

    if (Status != ERemoteCompileStatus::Succeeded)
    {
        return Status;
    }

    FShaderCodeView CodeView;
    String          ReadError;

    if (!FShaderCodeReader::Read(Result.ShaderCode, CodeView, &ReadError) || CodeView.GetOutputLanguage() != CompileInfo.OutputLanguage || CodeView.GetStage() != CompileInfo.ShaderStage)
    {
        LOG_ERROR("[FShaderCompiler]: %s returned an invalid container for '%s': %s", *Client.GetPeerName(), *Filename, *ReadError);
        return ERemoteCompileStatus::CompileFailed;
    }

    if (OutDependencies)
    {
        for (const String& Dependency : Result.Dependencies)
        {
            const String DependencyPath = Roots.MakeLocal(Dependency);
            if (!OutDependencies->Contains(DependencyPath))
            {
                OutDependencies->Emplace(DependencyPath);
            }
        }
    }

    // Matches the local path, which writes the MSL next to the shader when compiling with debug info.
    if (CompileInfo.bDebugInfo && CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL)
    {
        const TArrayView<const uint8> NativeCode = CodeView.GetNativeCode();

        TArray<uint8> MSLSource;
        MSLSource.Resize(NativeCode.Size());
        if (NativeCode.Size() > 0)
        {
            Memory::Memcpy(MSLSource.Data(), NativeCode.Data(), NativeCode.Size());
        }

        DumpContentToFile(MSLSource, Roots.GetFilePath() + "_" + ToString(CompileInfo.ShaderStage) + ".metal");
    }

    OutByteCode = ::Move(Result.ShaderCode);
    LOG_INFO("[FShaderCompiler]: Successfully compiled shader '%s' on %s", *Filename, *Client.GetPeerName());
    return ERemoteCompileStatus::Succeeded;
}

bool FShaderCompiler::CanCompileLocally(EShaderOutputLanguage OutputLanguage) const
{
    return IsShaderOutputLanguageSupportedByPlatform(OutputLanguage) && IsOutputLanguageSupported(OutputLanguage);
}

EShaderCompileRoute FShaderCompiler::GetRoute(EShaderOutputLanguage OutputLanguage) const
{
    // Without the remote compiler nothing changes, a missing backend is still reported by the compile itself.
    if (!CVarUseRemote.GetValue())
    {
        return EShaderCompileRoute::Local;
    }

    FRemoteShaderCompilerClient* Client = GetRemoteClient();
    if (Client && Client->CanCompile(OutputLanguage))
    {
        return EShaderCompileRoute::Remote;
    }

    return CVarRemoteFallbackToLocal.GetValue() && IsOutputLanguageSupported(OutputLanguage) ? EShaderCompileRoute::Local : EShaderCompileRoute::Unavailable;
}

FRemoteShaderCompilerClient* FShaderCompiler::GetRemoteClient() const
{
    TScopedLock Lock(RemoteClientCS);

    if (!RemoteClient)
    {
        RemoteClient = new FRemoteShaderCompilerClient();
    }

    const String Host    = CVarRemoteHost.GetValue();
    const uint16 Port    = static_cast<uint16>(CVarRemotePort.GetValue());
    const String Address = FSocketAddress::FormatHostAndPort(Host, Port);

    const bool bAddressChanged = Address != RemoteClientAddress;
    if (bAddressChanged)
    {
        RemoteClient->Disconnect();
        RemoteClientAddress     = Address;
        NextRemoteConnectCycles = 0;
    }

    if (!RemoteClient->IsConnected())
    {
        const uint64 Now = FPlatformTime::QueryPerformanceCounter();
        if (Now >= NextRemoteConnectCycles)
        {
            const uint64 Interval = static_cast<uint64>(GRemoteReconnectIntervalSeconds * static_cast<double>(FPlatformTime::QueryPerformanceFrequency()));
            NextRemoteConnectCycles = Now + Interval;

            if (RemoteClient->Connect(Host, Port, GRemoteConnectTimeoutMilliseconds))
            {
                LOG_INFO("[FShaderCompiler]: Connected to the remote shader compiler %s", *RemoteClient->GetPeerName());
            }
            else
            {
                LOG_WARNING("[FShaderCompiler]: Could not reach the remote shader compiler at %s, retrying in %.0f seconds", *Address, GRemoteReconnectIntervalSeconds);
            }
        }
    }

    return RemoteClient;
}

String FShaderCompiler::DescribeRemoteStatus() const
{
    if (!CVarUseRemote.GetValue())
    {
        return "Off, compiling in the engine";
    }

    FRemoteShaderCompilerClient* Client = GetRemoteClient();
    if (!Client || !Client->IsConnected())
    {
        return String::Printf("Not connected to %s, %s", *FSocketAddress::FormatHostAndPort(CVarRemoteHost.GetValue(), static_cast<uint16>(CVarRemotePort.GetValue())),
            CVarRemoteFallbackToLocal.GetValue() ? "compiling in the engine" : "shaders cannot compile");
    }

    String Local;
    String Forwarded;
    for (const FRemoteLanguageInfo& Language : Client->GetLanguages())
    {
        String& List = Language.bForwarded ? Forwarded : Local;
        if (!List.IsEmpty())
        {
            List += ", ";
        }

        List += ToString(Language.OutputLanguage);
    }

    String Description = String::Printf("Connected to %s", *Client->GetPeerName());
    if (!Local.IsEmpty())
    {
        Description += String::Printf(": %s local", *Local);
    }

    if (!Forwarded.IsEmpty())
    {
        Description += String::Printf("%s%s forwarded", Local.IsEmpty() ? ": " : ", ", *Forwarded);
    }

    return Description;
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

bool FShaderCompiler::CompileLocal(const String& ShaderSource, const String& FilePath, const String& ShaderDir, TArrayView<const String> IncludeDirs, const FShaderCompileInfo& CompileInfo,
    const IShaderSourceProvider* Sources, bool bDefinesAreFinal, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies, String* OutMessages)
{
    FScopedCompileTimer CompileTimer(NumCompiles, TotalCompileTimeNS);

    STAT_ADD(STAT_Shader_CompileCount, 1);
    OutByteCode.Clear();

    FShaderCompilerBackend* Backend = FindBackend(CompileInfo.OutputLanguage);
    if (!Backend)
    {
        const String Error = String::Printf("No compiler backend can produce %s", ToString(CompileInfo.OutputLanguage));
        LOG_ERROR("[FShaderCompiler]: %s", *Error);

        if (OutMessages)
        {
            *OutMessages += Error;
        }

        return false;
    }

    TArray<FShaderDefine> Defines;
    if (bDefinesAreFinal)
    {
        for (const FShaderDefine& Define : CompileInfo.Defines)
        {
            Defines.Emplace(Define);
        }
    }
    else
    {
        BuildCompileDefines(CompileInfo, Defines);
    }

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

    // Both compilers get source the engine has preprocessed, so macros and includes behave the same for every backend
    FShaderPreprocessor Preprocessor(ShaderDir, Sources);
    for (const String& IncludeDir : IncludeDirs)
    {
        Preprocessor.AddIncludeDir(IncludeDir);
    }

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

        if (OutMessages)
        {
            *OutMessages += Preprocessed.Errors;
        }

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
    Request.IncludeDir      = ShaderDir;
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

        if (OutMessages)
        {
            *OutMessages += Result.Messages.IsEmpty() ? String::Printf("%s failed without reporting an error", Backend->GetName()) : Result.Messages;
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

    // Dump the metal file to disk, which only exists when the sources came from it
    if (CompileInfo.bDebugInfo && CompileInfo.OutputLanguage == EShaderOutputLanguage::MSL && !FilePath.IsEmpty() && !Sources)
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

        if (OutMessages)
        {
            *OutMessages += WriteError;
        }

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
