#pragma once
#include "Resources.h"

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

struct ShaderDefine
{
    ShaderDefine( const std::string& InDefine )
        : Define( InDefine )
        , Value()
    {
    }

    ShaderDefine( const std::string& InDefine, const std::string& InValue )
        : Define( InDefine )
        , Value( InValue )
    {
    }

    std::string Define;
    std::string Value;
};

class IShaderCompiler
{
public:
    virtual ~IShaderCompiler() = default;

    virtual bool CompileFromFile(
        const std::string& FilePath,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code ) = 0;

    virtual bool CompileShader(
        const std::string& ShaderSource,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code ) = 0;
};

class ShaderCompiler
{
public:
    static FORCEINLINE bool CompileFromFile(
        const std::string& FilePath,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code )
    {
        return GShaderCompiler->CompileFromFile( FilePath, EntryPoint, Defines, ShaderStage, ShaderModel, Code );
    }

    static FORCEINLINE bool CompileShader(
        const std::string& ShaderSource,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code )
    {
        return GShaderCompiler->CompileShader( ShaderSource, EntryPoint, Defines, ShaderStage, ShaderModel, Code );
    }
};