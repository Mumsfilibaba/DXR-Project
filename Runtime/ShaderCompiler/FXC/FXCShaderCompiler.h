#pragma once
#if PLATFORM_WINDOWS
#include <d3dcompiler.h>

#include "ShaderCompiler/ShaderCompilerBackend.h"

class FFXCShaderCompiler final : public FShaderCompilerBackend
{
public:
    FFXCShaderCompiler();
    virtual ~FFXCShaderCompiler();

    // FShaderCompilerBackend Interface
    virtual bool Initialize() override final;

    virtual const CHAR* GetName() const override final { return "FXC"; }
    virtual bool SupportsOutputLanguage(EShaderOutputLanguage OutputLanguage) const override final;
    virtual void HashCompileSettings(const FShaderCompileInfo& CompileInfo, const String& IncludeDir, bool bDebugInfo, uint64& InOutHash) const override final;
    virtual bool TranslateSource(FShaderPreprocessorOutput& InOutSource) const override final;
    virtual bool Compile(const FShaderCompileRequest& Request, FShaderCompileResult& OutResult) override final;

private:
    static UINT BuildCompileFlags(const FShaderCompileInfo& CompileInfo, bool bDebugInfo);

    void*       D3DCompilerLib;
    pD3DCompile D3DCompileFunc;
};

#endif
