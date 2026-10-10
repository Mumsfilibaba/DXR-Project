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
    FShaderDefine(const FString& InDefine)
        : Define(InDefine)
        , Value()
    {
    }

    FShaderDefine(const FString& InDefine, const FString& InValue)
        : Define(InDefine)
        , Value(InValue)
    {
    }

    FString Define;
    FString Value;
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

    static bool Initialize(const FString& InAssetPath);
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
    bool CompileFromFile(const FString& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<FString>* OutDependencies = nullptr);

    /** Always compiles in this process, only the tests compile from source */
    bool CompileFromSource(const FString& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<FString>* OutDependencies = nullptr);

    /**
     * @brief Compiles with the local backends only, which is what the remote compiler server runs
     * @param SourceRoot Stands in for the asset directory. Job.SourceFile, Job.IncludeDirs and every include resolve below it through Sources.
     * @param OutDependencies Relative to SourceRoot, the shader itself first
     * @param OutMessages Preprocessor and compiler output, with SourceRoot stripped from every path
     */
    bool CompileJob(FShaderCompileJob& Job, const FString& SourceRoot, const IShaderSourceProvider& Sources, TArray<uint8>& OutShaderCode, TArray<FString>& OutDependencies, FString& OutMessages);

    /** @brief Sends the compile to Client. Used by CompileFromFile and by the batch tool, which has one client per remote host. */
    ERemoteCompileStatus CompileOnRemote(FRemoteShaderCompilerClient& Client, const FString& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<FString>* OutDependencies);

    /**
     * @brief Preprocesses from disk without compiling and returns every file that was read, the shader itself first
     * @param OutFiles Named the way CompileOnRemote sends them: relative to the asset directory, "@Include<N>/..." below
     *        CompileInfo.IncludeDirs[N] or "@Source/..." below the shader's own folder, whichever folder contains the file and every other one
     */
    bool CollectSources(const FString& Filename, const FShaderCompileInfo& CompileInfo, TArray<FShaderSourceFile>& OutFiles, FString& OutErrors) const;

    /** @return True when a backend produces the language and this platform has an RHI for it. This decides what a server offers. */
    NODISCARD bool CanCompileLocally(EShaderOutputLanguage OutputLanguage) const;

    NODISCARD EShaderCompileRoute GetRoute(EShaderOutputLanguage OutputLanguage) const;

    /** @return One line for the editor, for example "Connected to 127.0.0.1:9371 (Windows): DXIL, DXBC, SPIRV local, MSL forwarded" */
    NODISCARD FString DescribeRemoteStatus() const;

    NODISCARD const FString& GetAssetPath() const
    {
        return AssetPath;
    }

    /** @return Path when it is absolute, otherwise Path below the asset directory. Used for shader files and include directories alike. */
    NODISCARD FString ResolveSourcePath(const FString& Path) const;

    /** Uses the identity of whichever compiler GetRoute picks */
    NODISCARD uint64 ComputeCompileHash(const FString& SourceFile, const FShaderCompileInfo& CompileInfo) const;

    /** For a job compiled elsewhere, with Identity taken from that compiler's Hello */
    NODISCARD uint64 ComputeCompileHash(const FString& SourceFile, const FShaderCompileInfo& CompileInfo, const FShaderCompilerIdentity& Identity) const;

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
    FShaderCompiler(const FString& InAssetPath);
    ~FShaderCompiler();

    bool InitializeBackends();
    FShaderCompilerBackend* FindBackend(EShaderOutputLanguage OutputLanguage) const;
    NODISCARD FShaderCompilerIdentity GetIdentityForRoute(EShaderOutputLanguage OutputLanguage) const;
    void BuildCompileDefines(const FShaderCompileInfo& CompileInfo, TArray<FShaderDefine>& OutDefines) const;

    bool CompileLocal(
        const FString&               ShaderSource,
        const FString&               FilePath,
        const FString&               ShaderDir,
        TArrayView<const FString>    IncludeDirs,
        const FShaderCompileInfo&    CompileInfo,
        const IShaderSourceProvider* Sources,
        bool                         bDefinesAreFinal,
        TArray<uint8>&               OutByteCode,
        TArray<FString>*             OutDependencies,
        FString*                     OutMessages);

    NODISCARD TArray<FString> ResolveIncludeDirs(const FShaderCompileInfo& CompileInfo) const;
    bool CollectSources(const FShaderSourceRoots& Roots, const FShaderCompileInfo& CompileInfo, TArray<FShaderSourceFile>& OutFiles, FString& OutErrors) const;
    FRemoteShaderCompilerClient* GetRemoteClient() const;
    bool DumpContentToFile(const TArray<uint8>& ByteCode, const FString& Filename);
    void DumpPreprocessedSource(const FString& DumpDir, const FString& FilePath, const FShaderCompileInfo& CompileInfo, const TArray<FShaderDefine>& Defines, const FString& Source);

    TArray<FShaderCompilerBackend*>      Backends;
    FString                              AssetPath;
    FAtomicInt64                         NumCompiles;
    FAtomicInt64                         TotalCompileTimeNS;
    FCriticalSection                     DumpCS;
    mutable FRemoteShaderCompilerClient* RemoteClient;
    mutable FCriticalSection             RemoteClientCS;
    mutable FString                      RemoteClientAddress;
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
        const FString&                   InEntryPoint,
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
    TArrayView<const FString> IncludeDirs;
    FString                   EntryPoint;
};
