#pragma once
#include "RHI/RHIShaderCompiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanShaderCompiler

class CVulkanShaderCompiler : public IRHIShaderCompiler
{
public:

    CVulkanShaderCompiler()  = default;
    ~CVulkanShaderCompiler() = default;

    virtual bool CompileFromFile(const String& FilePath, const String& EntryPoint, const TArray<SShaderDefine>* Defines, ERHIShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code) override final
    {
        return true;
    }

    virtual bool CompileShader(const String& ShaderSource, const String& EntryPoint, const TArray<SShaderDefine>* Defines, ERHIShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code) override final
    {
        return true;
    }
};