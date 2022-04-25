#pragma once
#include "RHIModule.h"
#include "RHIShader.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderModel

enum class EShaderModel
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
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SShaderDefine

struct SShaderDefine
{
    FORCEINLINE SShaderDefine(const String& InDefine)
        : Define(InDefine)
        , Value()
    { }

    FORCEINLINE SShaderDefine(const String& InDefine, const String& InValue)
        : Define(InDefine)
        , Value(InValue)
    { }

    String Define;
    String Value;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHIShaderCompiler

class IRHIShaderCompiler
{
public:
    virtual ~IRHIShaderCompiler() = default;

    virtual bool CompileFromFile(const String& FilePath, const String& EntryPoint, const TArray<SShaderDefine>* Defines, ERHIShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code) = 0;
    virtual bool CompileShader(const String& ShaderSource, const String& EntryPoint, const TArray<SShaderDefine>* Defines, ERHIShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code) = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderCompiler

class CRHIShaderCompiler
{
public:
    static FORCEINLINE bool CompileFromFile(const String& FilePath, const String& EntryPoint, const TArray<SShaderDefine>* Defines, ERHIShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code)
    {
        return GShaderCompiler->CompileFromFile(FilePath, EntryPoint, Defines, ShaderStage, ShaderModel, Code);
    }

    static FORCEINLINE bool CompileShader(const String& ShaderSource, const String& EntryPoint, const TArray<SShaderDefine>* Defines, ERHIShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code)
    {
        return GShaderCompiler->CompileShader(ShaderSource, EntryPoint, Defines, ShaderStage, ShaderModel, Code);
    }
};