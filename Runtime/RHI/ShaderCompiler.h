#pragma once
#include "Core/Containers/Optional.h"
#include "RHI/RHIShader.h"

// TODO: Check if this could be avoided
#if PLATFORM_WINDOWS
    #include <Unknwn.h>
#endif

#include <dxc/dxcapi.h>

enum class EShaderModel : uint8
{
    Unknown = 0,
    SM_6_0  = 3,
    SM_6_1  = 4,
    SM_6_2  = 5,
    SM_6_3  = 6,
    SM_6_4  = 7,
    SM_6_5  = 8,
    SM_6_6  = 9,
    SM_6_7  = 10,
};

enum class EShaderOutputLanguage : uint8
{
    Unknown = 0,
    HLSL    = 1, // DXIL for D3D12RHI
    MSL     = 2, // Metal Shading Language for MetalRHI
    SPIRV   = 3, // SPIR-V for VulkanRHI
};

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

class RHI_API FShaderCompiler
{
public:
    static bool Create(FStringView AssetFolderPath);
    static void Destroy();
    static EShaderOutputLanguage GetOutputLanguageBasedOnRHI();
    static FShaderCompiler& Get();

    bool CompileFromFile(const FString& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool CompileFromSource(const FString& ShaderSource, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);

private:
    FShaderCompiler(FStringView InAssetPath);
    ~FShaderCompiler();

    bool Initialize();
    bool Compile(const FString& ShaderSource, const FString& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool PatchHLSLForSpirv(const FString& Entrypoint, FString& OutSource);
    bool RecompileSpirv(const FString& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool ConvertSpirvToMetalShader(const FString& FilePath, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool DumpContentToFile(const TArray<uint8>& OutByteCode, const FString& Filename);
    FString CreateArgString(const TArrayView<LPCWSTR> Args);

    void*                 DXCLib;
    DxcCreateInstanceProc DxcCreateInstanceFunc;
    FString               AssetPath;

    static FShaderCompiler* GInstance;
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
    
    FShaderCompileInfo(
        const FString&                   InEntryPoint,
        EShaderModel                     InShaderModel,
        EShaderStage                     InShaderStage,
        const TArrayView<FShaderDefine>& InDefines        = TArrayView<FShaderDefine>(),
        EShaderOutputLanguage            InOutputLanguage = FShaderCompiler::GetOutputLanguageBasedOnRHI())
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
    FString                   EntryPoint;
};
