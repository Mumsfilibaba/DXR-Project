#pragma once
#include "RenderLayer/ShaderCompiler.h"

#include "D3D12Helpers.h"

#include <string>

class D3D12ShaderCompiler : public IShaderCompiler
{
public:
    D3D12ShaderCompiler();
    ~D3D12ShaderCompiler();

    Bool Init();
    
    virtual Bool CompileFromFile(
        const std::string& FilePath,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<UInt8>& Code) const override final;

    virtual Bool CompileShader(
        const std::string& ShaderSource,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<UInt8>& Code) const override final;

private:
    Bool InternalCompileFromSource(
        IDxcBlob* SourceBlob, 
        LPCWSTR FilePath, 
        LPCWSTR Entrypoint, 
        LPCWSTR TargetProfile, 
        const TArray<ShaderDefine>* Defines,
        TArray<UInt8>& Code) const;

private:
    TComPtr<IDxcCompiler>       DxCompiler;
    TComPtr<IDxcLibrary>        DxLibrary;
    TComPtr<IDxcLinker>         DxLinker;
    TComPtr<IDxcIncludeHandler> DxIncludeHandler;
    HMODULE DxCompilerDLL;
};