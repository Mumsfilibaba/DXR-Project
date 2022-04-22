#pragma once
#include "RHI/RHIShaderCompiler.h"

#include "D3D12Core.h"
#include "D3D12Shader.h"

#include <string>

#include <d3d12shader.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ShaderCompiler

class CD3D12ShaderCompiler : public IRHIShaderCompiler
{
public:
    CD3D12ShaderCompiler();
    ~CD3D12ShaderCompiler();

    bool Initialize();

    virtual bool CompileFromFile(
        const String& FilePath,
        const String& EntryPoint,
        const TArray<SShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code) override final;

    virtual bool CompileShader(
        const String& ShaderSource,
        const String& EntryPoint,
        const TArray<SShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code) override final;

    bool GetReflection(CD3D12BaseShader* Shader, ID3D12ShaderReflection** Reflection);
    bool GetLibraryReflection(CD3D12BaseShader* Shader, ID3D12LibraryReflection** Reflection);

    bool HasRootSignature(CD3D12BaseShader* Shader);

private:
    bool InternalCompileFromSource(
        IDxcBlob* SourceBlob,
        LPCWSTR FilePath,
        LPCWSTR Entrypoint,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        const TArray<SShaderDefine>* Defines,
        TArray<uint8>& Code);

    bool InternalGetReflection(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject);

    bool ValidateRayTracingShader(const TComPtr<IDxcBlob>& ShaderBlob, LPCWSTR Entrypoint);

private:
    TComPtr<IDxcCompiler>       DxCompiler;
    TComPtr<IDxcLibrary>        DxLibrary;
    TComPtr<IDxcLinker>         DxLinker;
    TComPtr<IDxcIncludeHandler>      DxIncludeHandler;
    TComPtr<IDxcContainerReflection> DxReflection;

    HMODULE DxCompilerDLL;
};

extern CD3D12ShaderCompiler* GD3D12ShaderCompiler;
extern DxcCreateInstanceProc DxcCreateInstanceFunc;