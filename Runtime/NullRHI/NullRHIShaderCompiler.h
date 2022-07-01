#pragma once
#include "RHI/RHIShaderCompiler.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIShaderCompiler

class CNullRHIShaderCompiler : public IRHIShaderCompiler
{
public:

    CNullRHIShaderCompiler()  = default;
    ~CNullRHIShaderCompiler() = default;

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

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif