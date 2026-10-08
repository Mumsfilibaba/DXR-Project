#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/Optional.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic.h"
#include "Core/Time/Timespan.h"
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
class FShaderCompilerBackend;

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

    /** OutByteCode receives an FShaderCode container, see ShaderCore/ShaderCode.h */
    bool CompileFromFile(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies = nullptr);
    bool CompileFromSource(const String& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies = nullptr);

    NODISCARD uint64 ComputeCompileHash(const String& SourceFile, const FShaderCompileInfo& CompileInfo) const;

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
    void BuildCompileDefines(const FShaderCompileInfo& CompileInfo, TArray<FShaderDefine>& OutDefines) const;
    bool Compile(const String& ShaderSource, const String& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies);
    bool DumpContentToFile(const TArray<uint8>& ByteCode, const String& Filename);
    void DumpPreprocessedSource(const String& DumpDir, const String& FilePath, const FShaderCompileInfo& CompileInfo, const TArray<FShaderDefine>& Defines, const String& Source);

    TArray<FShaderCompilerBackend*> Backends;
    String                          AssetPath;
    AtomicInt64                     NumCompiles;
    AtomicInt64                     TotalCompileTimeNS;
    FCriticalSection                DumpCS;

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
        , EntryPoint(InEntryPoint)
    {
    }

    EShaderModel              ShaderModel;
    EShaderStage              ShaderStage;
    EShaderOutputLanguage     OutputLanguage;
    bool                      bOptimize;
    bool                      bDebugInfo;
    TArrayView<FShaderDefine> Defines;
    String                    EntryPoint;
};
