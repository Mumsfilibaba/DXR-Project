#pragma once
#include "Resources.h"

enum class EShaderModel
{
    _5_0 = 1,
    _5_1 = 2,
    _6_0 = 3,
};

struct ShaderDefine
{
    ShaderDefine(const std::string& InDefine)
        : Define(InDefine)
        , Value()
    {
    }

    ShaderDefine(const std::string& InDefine, const std::string& InValue)
        : Define(InDefine)
        , Value(InValue)
    {
    }

    std::string Define;
    std::string Value;
};

class IShaderCompiler
{
public:
    virtual ~IShaderCompiler() = default;

    virtual Bool CompileFromFile(
        const std::string& FilePath,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<UInt8>& Code) const = 0;

    virtual Bool CompileShader(
        const std::string& ShaderSource,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<UInt8>& Code) const = 0;
};

class ShaderCompiler
{
    friend class RenderLayer;

public:
    FORCEINLINE static Bool CompileFromFile(
        const std::string& FilePath,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<UInt8>& Code)
    {
        return gShaderCompiler->CompileFromFile(FilePath, EntryPoint, Defines, ShaderStage, ShaderModel, Code);
    }

    FORCEINLINE static Bool CompileShader(
        const std::string& ShaderSource,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<UInt8>& Code)
    {
        return gShaderCompiler->CompileShader(ShaderSource, EntryPoint, Defines, ShaderStage, ShaderModel, Code);
    }
};