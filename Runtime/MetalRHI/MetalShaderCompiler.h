#pragma once
#include "RHI/RHIShaderCompiler.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalShaderCompiler

class FMetalShaderCompiler : public IRHIShaderCompiler
{
public:

    FMetalShaderCompiler()  = default;
    ~FMetalShaderCompiler() = default;

    virtual bool CompileFromFile( const FString& FilePath
                                , const FString& EntryPoint
                                , const TArray<FShaderDefine>* Defines
                                , EShaderStage ShaderStage
                                , EShaderModel ShaderModel
                                , TArray<uint8>& Code) override final
    {
        return true;
    }

    virtual bool CompileShader( const FString& ShaderSource
                              , const FString& EntryPoint
                              , const TArray<FShaderDefine>* Defines
                              , EShaderStage ShaderStage
                              , EShaderModel ShaderModel
                              , TArray<uint8>& Code) override final
    {
        return true;
    }
};

#pragma clang diagnostic pop
