#pragma once
#include "RHI/RHIShaderCompiler.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalShaderCompiler

class CMetalShaderCompiler : public IRHIShaderCompiler
{
public:

    CMetalShaderCompiler()  = default;
    ~CMetalShaderCompiler() = default;

    virtual bool CompileFromFile( const String& FilePath
                                , const String& EntryPoint
                                , const TArray<FShaderDefine>* Defines
                                , EShaderStage ShaderStage
                                , EShaderModel ShaderModel
                                , TArray<uint8>& Code) override final
    {
        return true;
    }

    virtual bool CompileShader( const String& ShaderSource
                              , const String& EntryPoint
                              , const TArray<FShaderDefine>* Defines
                              , EShaderStage ShaderStage
                              , EShaderModel ShaderModel
                              , TArray<uint8>& Code) override final
    {
        return true;
    }
};

#pragma clang diagnostic pop
