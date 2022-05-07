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
// CMetalShaderCompiler

class CMetalShaderCompiler : public IRHIShaderCompiler
{
public:

    CMetalShaderCompiler()  = default;
    ~CMetalShaderCompiler() = default;

    virtual bool CompileFromFile( const String& FilePath
                                , const String& EntryPoint
                                , const TArray<SShaderDefine>* Defines
                                , EShaderStage ShaderStage
                                , EShaderModel ShaderModel
                                , TArray<uint8>& Code) override final
    {
        return true;
    }

    virtual bool CompileShader( const String& ShaderSource
                              , const String& EntryPoint
                              , const TArray<SShaderDefine>* Defines
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