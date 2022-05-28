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
// SShaderDefine

struct SShaderDefine
{
    SShaderDefine(const String& InDefine)
        : Define(InDefine)
        , Value()
    { }

    SShaderDefine(const String& InDefine, const String& InValue)
        : Define(InDefine)
        , Value(InValue)
    { }

    String Define;
    String Value;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShaderCompileInfo

class CShaderCompileInfo
{
public:
    
    CShaderCompileInfo()
        : ShaderModel(EShaderModel::Unknown)
        , ShaderStage(EShaderStage::Unknown)
        , OutputLanguage(EShaderOutputLanguage::Unknown)
        , bOptimize(true)
        , Defines()
        , EntryPoint()
    { }
    
    CShaderCompileInfo( const String& InEntryPoint
                      , EShaderModel InShaderModel
                      , EShaderStage InShaderStage
                      , const TArrayView<SShaderDefine>& InDefines = TArrayView<SShaderDefine>()
#if PLATFORM_WINDOWS
                      , EShaderOutputLanguage InOutputLanguage = EShaderOutputLanguage::HLSL)
#elif PLATFORM_MAC
                      , EShaderOutputLanguage InOutputLanguage = EShaderOutputLanguage::MSL)
#else
                      , EShaderOutputLanguage InOutputLanguage = EShaderOutputLanguage::Unknown)
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

    TArrayView<SShaderDefine> Defines;

    String                    EntryPoint;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CShaderCompiler

class RHI_API CShaderCompiler
{
public:
    CShaderCompiler(const char* InAssestPath);
    ~CShaderCompiler();

public:
    
    static bool Initialize(const char* AssetFolderPath);
    
    static void Release();
    
    static CShaderCompiler& Get();

public:

    bool CompileFromFile(const String& Filename, const CShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    
    bool CompileFromSource(const String& ShaderSource, const CShaderCompileInfo& CompileInfo, TArray<uint8>& OutByteCode);
    
private:
    void*                 DXCLibrary;
    DxcCreateInstanceProc DxcCreateInstanceFunc;
    
    String                AssetPath;

    static TOptional<CShaderCompiler> Instance;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* DEPRECATED */

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHIShaderCompiler

class IRHIShaderCompiler
{
public:
    virtual ~IRHIShaderCompiler() = default;

    virtual bool CompileFromFile(const String& FilePath, const String& EntryPoint, const TArray<SShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code) = 0;
    virtual bool CompileShader(const String& ShaderSource, const String& EntryPoint, const TArray<SShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code) = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderCompiler

class CRHIShaderCompiler
{
public:
    static FORCEINLINE bool CompileFromFile(const String& FilePath, const String& EntryPoint, const TArray<SShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code)
    {
        return GShaderCompiler->CompileFromFile(FilePath, EntryPoint, Defines, ShaderStage, ShaderModel, Code);
    }

    static FORCEINLINE bool CompileShader(const String& ShaderSource, const String& EntryPoint, const TArray<SShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code)
    {
        return GShaderCompiler->CompileShader(ShaderSource, EntryPoint, Defines, ShaderStage, ShaderModel, Code);
    }
};

