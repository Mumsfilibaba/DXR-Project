#pragma once

// TODO: Check if this could be avoided
#if PLATFORM_WINDOWS
    #include <Unknwn.h>
#endif

#include <dxc/dxcapi.h>

#include "RHIModule.h"
#include "RHIShader.h"

#include "Core/Containers/Optional.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderModel

enum class EShaderModel : uint8
{
    Unknown = 0,
    SM_5_0 = 1,
    SM_5_1 = 2,
    SM_6_0 = 3,
    SM_6_1 = 4,
    SM_6_2 = 5,
    SM_6_3 = 6,
    SM_6_4 = 7,
    SM_6_5 = 8,
    SM_6_6 = 9,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderOutputLanguage

enum class EShaderOutputLanguage : uint8
{
    Unknown = 0,
    HLSL    = 1, // DXIL for D3D12RHI
    MSL     = 2, // Metal Shading Language for MetalRHI
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FShaderDefine

struct FShaderDefine
{
    FShaderDefine(const FString& InDefine)
        : Define(InDefine)
        , Value()
    { }

    FShaderDefine(const FString& InDefine, const FString& InValue)
        : Define(InDefine)
        , Value(InValue)
    { }

    FString Define;
    FString Value;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIShaderCompileInfo

struct FRHIShaderCompileInfo
{
    FRHIShaderCompileInfo()
        : ShaderModel(EShaderModel::Unknown)
        , ShaderStage(EShaderStage::Unknown)
        , OutputLanguage(EShaderOutputLanguage::Unknown)
        , bOptimize(true)
        , Defines()
        , EntryPoint()
    { }
    
    FRHIShaderCompileInfo(
        const FString& InEntryPoint,
        EShaderModel InShaderModel,
        EShaderStage InShaderStage,
        const TArrayView<FShaderDefine>& InDefines = TArrayView<FShaderDefine>(),
#if PLATFORM_WINDOWS
        EShaderOutputLanguage InOutputLanguage = EShaderOutputLanguage::HLSL)
#elif PLATFORM_MACOS
        EShaderOutputLanguage InOutputLanguage = EShaderOutputLanguage::MSL)
#else
        EShaderOutputLanguage InOutputLanguage = EShaderOutputLanguage::Unknown)
#endif
        : ShaderModel(InShaderModel)
        , ShaderStage(InShaderStage)
        , OutputLanguage(InOutputLanguage)
        , bOptimize(true)
        , Defines(InDefines)
        , EntryPoint(InEntryPoint)
    { }
    
    EShaderModel              ShaderModel;
    EShaderStage              ShaderStage;
    EShaderOutputLanguage     OutputLanguage;
    
    bool                      bOptimize;

    TArrayView<FShaderDefine> Defines;

    FString                   EntryPoint;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIShaderCompiler

class RHI_API FRHIShaderCompiler
{
private:
    friend class TOptional<FRHIShaderCompiler>;

    FRHIShaderCompiler(const CHAR* InAssetPath);
    ~FRHIShaderCompiler();

public:
    static bool Initialize(const CHAR* AssetFolderPath);
    static void Release();
    
    static FRHIShaderCompiler& Get();

    bool CompileFromFile(const FString& Filename, const FRHIShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    bool CompileFromSource(const FString& ShaderSource, const FRHIShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);

private:
    static void ErrorCallback(void* Userdata, const CHAR* Error);

    bool ConvertSpirvToMetalShader(const FString& Entrypoint, TArray<uint8>& OutByteCode);
    bool DumpContentToFile(const TArray<uint8>& OutByteCode, const FString& Filename);

    FString CreateArgString(const TArrayView<LPCWSTR> Args);

public:
    void*                 DXCLib;
    DxcCreateInstanceProc DxcCreateInstanceFunc;

    FString               AssetPath;

    static TOptional<FRHIShaderCompiler> Instance;
};