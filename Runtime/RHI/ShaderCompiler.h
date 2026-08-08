#pragma once
#include "Core/Containers/Optional.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic.h"
#include "Core/Time/Timespan.h"
#include "RHI/RHIShader.h"
#include "RHI/ShaderCompilerInclude.h"

enum class EShaderOutputLanguage : uint8
{
    Unknown = 0,

    /** DXIL for D3D12RHI */
    HLSL = 1,

    /** Metal Shading Language for MetalRHI */
    MSL = 2,

    /** SPIR-V for VulkanRHI */
    SPIRV = 3,
};

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

class RHI_API FShaderCompiler
{
public:
    static EShaderOutputLanguage GetOutputLanguageBasedOnRHI();

    static bool Initialize(const String& InAssetPath);
    static void Destroy();

    static FORCEINLINE FShaderCompiler& Get()
    {
        CHECK(GShaderCompiler != nullptr);
        return *GShaderCompiler;
    }

    static FORCEINLINE FShaderCompiler* TryGet()
    {
        return GShaderCompiler;
    }

public:
    bool CompileFromFile(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies = nullptr);
    bool CompileFromSource(const String& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies = nullptr);

    NODISCARD uint64 ComputeCompileHash(const String& SourceFile, const FShaderCompileInfo& CompileInfo) const;

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

    bool InitializeDXC();
    bool Compile(const String& ShaderSource, const String& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode, TArray<String>* OutDependencies);
    bool ConvertSpirvToMetalShader(const String& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool DumpContentToFile(const TArray<uint8>& OutByteCode, const String& Filename);
    String CreateArgString(const TArrayView<LPCWSTR> Args);

    void*                 DXCLib;
    DxcCreateInstanceProc DxcCreateInstanceFunc;
    String                AssetPath;
    uint32                DXCVersionMajor;
    uint32                DXCVersionMinor;
    AtomicInt64           NumCompiles;
    AtomicInt64           TotalCompileTimeNS;
    FCriticalSection      DumpCS;

    static FShaderCompiler* GShaderCompiler;
};

struct FShaderCompileInfo
{
    FShaderCompileInfo()
        : ShaderModel(EShaderModel::Unknown)
        , ShaderStage(EShaderStage::Unknown)
        , OutputLanguage(EShaderOutputLanguage::Unknown)
        , bOptimize(true)
        , Defines()
        , EntryPoint()
    {
    }
    
    FShaderCompileInfo(const String& InEntryPoint, EShaderModel InShaderModel, EShaderStage InShaderStage, 
        const TArrayView<FShaderDefine>& InDefines = TArrayView<FShaderDefine>(), EShaderOutputLanguage InOutputLanguage = FShaderCompiler::GetOutputLanguageBasedOnRHI())
        : ShaderModel(InShaderModel)
        , ShaderStage(InShaderStage)
        , OutputLanguage(InOutputLanguage)
        , bOptimize(true)
        , Defines(InDefines)
        , EntryPoint(InEntryPoint)
    {
    }
    
    EShaderModel              ShaderModel;
    EShaderStage              ShaderStage;
    EShaderOutputLanguage     OutputLanguage;
    bool                      bOptimize;
    TArrayView<FShaderDefine> Defines;
    String                    EntryPoint;
};
