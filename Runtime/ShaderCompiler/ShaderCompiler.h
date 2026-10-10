#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/Optional.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic.h"
#include "Core/Time/Timespan.h"
#include "ShaderCompiler/ShaderCompilerIdentity.h"
#include "ShaderCore/ShaderTypes.h"

struct FShaderDefine
{
    FShaderDefine(const String& InDefine)
        : Define(InDefine)
        , Value()
    {
    }

    FShaderDefine(const String& InDefine, const String& InValue)
        : Define(InDefine)
        , Value(InValue)
    {
    }

    String Define;
    String Value;
};

struct FShaderCompileInfo;
struct FShaderCompileJob;
struct FShaderSourceFile;
class FShaderCompilerBackend;
class FShaderSourceRoots;
class FRemoteShaderCompilerClient;
class IShaderSourceProvider;
enum class ERemoteCompileStatus : uint8;

enum class EShaderCompileRoute : uint8
{
    /** Compiled in this process */
    Local,

    /** Sent to the server in RHI.ShaderCompiler.RemoteHost */
    Remote,

    /** RHI.ShaderCompiler.UseRemote is set, the server cannot produce the language and falling back is not allowed or not possible */
    Unavailable,
};

class SHADERCOMPILER_API FShaderCompiler
{
public:

    /** @return RHI.ShaderCompiler.Debug, always false in RELEASE_BUILD */
    NODISCARD static bool IsDebugInfoEnabled();

    static bool Initialize(const String& InAssetPath);
    static void Destroy();

    static FORCEINLINE FShaderCompiler& Get()
    {
        CHECK(ShaderCompiler != nullptr);
        return *ShaderCompiler;
    }

    static FORCEINLINE FShaderCompiler* TryGet()
    {
        return ShaderCompiler;
    }

public:

    /**
     * @brief OutByteCode receives an FShaderCode container, see ShaderCore/ShaderCode.h. Uses the remote compiler when RHI.ShaderCompiler.UseRemote is set.
     * @param Filename Absolute, or relative to the asset directory
     */
    bool CompileFromFile(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies = nullptr);

    /** Always compiles in this process, only the tests compile from source */
    bool CompileFromSource(const String& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies = nullptr);

    /**
     * @brief Compiles with the local backends only, which is what the remote compiler server runs
     * @param SourceRoot Stands in for the asset directory. Job.SourceFile, Job.IncludeDirs and every include resolve below it through Sources.
     * @param OutDependencies Relative to SourceRoot, the shader itself first
     * @param OutMessages Preprocessor and compiler output, with SourceRoot stripped from every path
     */
    bool CompileJob(FShaderCompileJob& Job, const String& SourceRoot, const IShaderSourceProvider& Sources, TArray<uint8>& OutShaderCode, TArray<String>& OutDependencies, String& OutMessages);

    /** @brief Sends the compile to Client. Used by CompileFromFile and by the batch tool, which has one client per remote host. */
    ERemoteCompileStatus CompileOnRemote(FRemoteShaderCompilerClient& Client, const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies);

    /**
     * @brief Preprocesses from disk without compiling and returns every file that was read, the shader itself first
     * @param OutFiles Named the way CompileOnRemote sends them: relative to the asset directory, "@Include<N>/..." below
     *        CompileInfo.IncludeDirs[N] or "@Source/..." below the shader's own folder, whichever folder contains the file and every other one
     */
    bool CollectSources(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<FShaderSourceFile>& OutFiles, String& OutErrors) const;

    /** @return True when a backend produces the language and this platform has an RHI for it. This decides what a server offers. */
    NODISCARD bool CanCompileLocally(EShaderOutputLanguage OutputLanguage) const;

    NODISCARD EShaderCompileRoute GetRoute(EShaderOutputLanguage OutputLanguage) const;

    /** @return One line for the editor, for example "Connected to 127.0.0.1:9371 (Windows): DXIL, DXBC, SPIRV local, MSL forwarded" */
    NODISCARD String DescribeRemoteStatus() const;

    NODISCARD const String& GetAssetPath() const
    {
        return AssetPath;
    }

    /** @return Path when it is absolute, otherwise Path below the asset directory. Used for shader files and include directories alike. */
    NODISCARD String ResolveSourcePath(const String& Path) const;

    /** Uses the identity of whichever compiler GetRoute picks */
    NODISCARD uint64 ComputeCompileHash(const String& SourceFile, const FShaderCompileInfo& CompileInfo) const;

    /** For a job compiled elsewhere, with Identity taken from that compiler's Hello */
    NODISCARD uint64 ComputeCompileHash(const String& SourceFile, const FShaderCompileInfo& CompileInfo, const FShaderCompilerIdentity& Identity) const;

    /** @return Invalid when no local backend produces the language */
    NODISCARD FShaderCompilerIdentity GetLocalIdentity(EShaderOutputLanguage OutputLanguage) const;

    /** @return Returns true if one of the compiler backends can produce the output language */
    NODISCARD bool IsOutputLanguageSupported(EShaderOutputLanguage OutputLanguage) const;

    /** @return Returns the output languages the current platform has an RHI for, and that a backend can produce */
    NODISCARD TArray<EShaderOutputLanguage> GetSupportedOutputLanguages() const;

    void LogCompileStats() const;

    NODISCARD int64 GetNumCompiles() const
    {
        return NumCompiles.Load();
    }

    NODISCARD FTimespan GetTotalCompileTime() const
    {
        return FTimespan(static_cast<uint64>(TotalCompileTimeNS.Load()));
    }

private:
    FShaderCompiler(const String& InAssetPath);
    ~FShaderCompiler();

    bool InitializeBackends();
    FShaderCompilerBackend* FindBackend(EShaderOutputLanguage OutputLanguage) const;
    NODISCARD FShaderCompilerIdentity GetIdentityForRoute(EShaderOutputLanguage OutputLanguage) const;
    void BuildCompileDefines(const FShaderCompileInfo& CompileInfo, TArray<FShaderDefine>& OutDefines) const;

    bool CompileLocal(
        const String&                ShaderSource,
        const String&                FilePath,
        const String&                ShaderDir,
        TArrayView<const String>     IncludeDirs,
        const FShaderCompileInfo&    CompileInfo,
        const IShaderSourceProvider* Sources,
        bool                         bDefinesAreFinal,
        TArray<uint8>&               OutByteCode,
        TArray<String>*              OutDependencies,
        String*                      OutMessages);

    NODISCARD TArray<String> ResolveIncludeDirs(const FShaderCompileInfo& CompileInfo) const;
    bool CollectSources(const FShaderSourceRoots& Roots, const FShaderCompileInfo& CompileInfo, TArray<FShaderSourceFile>& OutFiles, String& OutErrors) const;
    FRemoteShaderCompilerClient* GetRemoteClient() const;
    bool DumpContentToFile(const TArray<uint8>& ByteCode, const String& Filename);
    void DumpPreprocessedSource(const String& DumpDir, const String& FilePath, const FShaderCompileInfo& CompileInfo, const TArray<FShaderDefine>& Defines, const String& Source);

    TArray<FShaderCompilerBackend*>      Backends;
    String                               AssetPath;
    AtomicInt64                          NumCompiles;
    AtomicInt64                          TotalCompileTimeNS;
    FCriticalSection                     DumpCS;
    mutable FRemoteShaderCompilerClient* RemoteClient;
    mutable FCriticalSection             RemoteClientCS;
    mutable String                       RemoteClientAddress;
    mutable uint64                       NextRemoteConnectCycles;

    static FShaderCompiler* ShaderCompiler;
};

struct FShaderCompileInfo
{
    FShaderCompileInfo()
        : ShaderModel(EShaderModel::Unknown)
        , ShaderStage(EShaderStage::Unknown)
        , OutputLanguage(EShaderOutputLanguage::Unknown)
        , bOptimize(true)
        , bDebugInfo(FShaderCompiler::IsDebugInfoEnabled())
        , Defines()
        , IncludeDirs()
        , EntryPoint()
    {
    }

    FShaderCompileInfo(
        const String&                    InEntryPoint,
        EShaderModel                     InShaderModel,
        EShaderStage                     InShaderStage,
        EShaderOutputLanguage            InOutputLanguage,
        const TArrayView<FShaderDefine>& InDefines = TArrayView<FShaderDefine>())
        : ShaderModel(InShaderModel)
        , ShaderStage(InShaderStage)
        , OutputLanguage(InOutputLanguage)
        , bOptimize(true)
        , bDebugInfo(FShaderCompiler::IsDebugInfoEnabled())
        , Defines(InDefines)
        , IncludeDirs()
        , EntryPoint(InEntryPoint)
    {
    }

    EShaderModel              ShaderModel;
    EShaderStage              ShaderStage;
    EShaderOutputLanguage     OutputLanguage;
    bool                      bOptimize;
    bool                      bDebugInfo;
    TArrayView<FShaderDefine> Defines;
    TArrayView<const String>  IncludeDirs;
    String                    EntryPoint;
};
