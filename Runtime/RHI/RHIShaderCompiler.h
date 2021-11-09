#pragma once
#include "RHIModule.h"
#include "RHIShader.h"

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

struct SShaderDefine
{
    FORCEINLINE SShaderDefine( const CString& InDefine )
        : Define( InDefine )
        , Value()
    {
    }

    FORCEINLINE SShaderDefine( const CString& InDefine, const CString& InValue )
        : Define( InDefine )
        , Value( InValue )
    {
    }

    CString Define;
    CString Value;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class IRHIShaderCompiler
{
public:
    virtual ~IRHIShaderCompiler() = default;

    virtual bool CompileFromFile(
        const CString& FilePath,
        const CString& EntryPoint,
        const TArray<SShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code ) = 0;

    virtual bool CompileShader(
        const CString& ShaderSource,
        const CString& EntryPoint,
        const TArray<SShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code ) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CRHIShaderCompiler
{
public:
    static FORCEINLINE bool CompileFromFile(
        const CString& FilePath,
        const CString& EntryPoint,
        const TArray<SShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code )
    {
        return GShaderCompiler->CompileFromFile( FilePath, EntryPoint, Defines, ShaderStage, ShaderModel, Code );
    }

    static FORCEINLINE bool CompileShader(
        const CString& ShaderSource,
        const CString& EntryPoint,
        const TArray<SShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code )
    {
        return GShaderCompiler->CompileShader( ShaderSource, EntryPoint, Defines, ShaderStage, ShaderModel, Code );
    }
};