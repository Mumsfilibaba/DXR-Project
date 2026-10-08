#pragma once
#if PLATFORM_WINDOWS
#include <d3dcompiler.h>

#include "ShaderCompiler/ShaderCompilerBackend.h"
#include "ShaderCompiler/Reflection/DXBCShaderReflector.h"

typedef HRESULT(WINAPI* PFN_FXC_D3D_STRIP_SHADER)(LPCVOID pShaderBytecode, SIZE_T BytecodeLength, UINT uStripFlags, ID3DBlob** ppStrippedBlob);

class FFXCShaderCompiler final : public FShaderCompilerBackend
{
public:
    FFXCShaderCompiler();
    virtual ~FFXCShaderCompiler();

    // FShaderCompilerBackend Interface
    virtual bool Initialize() override final;

    virtual const CHAR* GetName() const override final { return "FXC"; }
    virtual bool SupportsOutputLanguage(EShaderOutputLanguage OutputLanguage) const override final;
    virtual void HashCompileSettings(const FShaderCompileInfo& CompileInfo, const String& IncludeDir, uint64& InOutHash) const override final;
    virtual bool TranslateSource(FShaderPreprocessorOutput& InOutSource) const override final;
    virtual bool Compile(const FShaderCompileRequest& Request, FShaderCompileResult& OutResult) override final;

private:
    static UINT BuildCompileFlags(const FShaderCompileInfo& CompileInfo);

    void*                    D3DCompilerLib;
    pD3DCompile              D3DCompileFunc;
    PFN_FXC_D3D_REFLECT      D3DReflectFunc;
    PFN_FXC_D3D_STRIP_SHADER D3DStripShaderFunc;
};

#endif
