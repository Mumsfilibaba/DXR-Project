#pragma once
#include "Core/Containers/Optional.h"
#include "RHI/RHIShader.h"
#include "RHI/ShaderCompilerInclude.h"

enum class EShaderOutputLanguage : uint8
{
    Unknown = 0,
    HLSL    = 1, // DXIL for D3D12RHI
    MSL     = 2, // Metal Shading Language for MetalRHI
    SPIRV   = 3, // SPIR-V for VulkanRHI
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

public:
    bool CompileFromFile(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool CompileFromSource(const String& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);

private:
    FShaderCompiler(const String& InAssetPath);
    ~FShaderCompiler();

    bool InitializeDXC();
    bool Compile(const String& ShaderSource, const String& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool ConvertSpirvToMetalShader(const String& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool DumpContentToFile(const TArray<uint8>& OutByteCode, const String& Filename);
    String CreateArgString(const TArrayView<LPCWSTR> Args);

    void*                 DXCLib;
    DxcCreateInstanceProc DxcCreateInstanceFunc;
    String                AssetPath;

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
