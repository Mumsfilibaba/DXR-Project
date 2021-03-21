#pragma once
#include "RenderLayer/ShaderCompiler.h"

#include "D3D12Helpers.h"
#include "D3D12Shader.h"

#include <string>

#include <d3d12shader.h>

class D3D12ShaderCompiler : public IShaderCompiler
{
public:
    D3D12ShaderCompiler();
    ~D3D12ShaderCompiler();

    bool Init();
    
    virtual bool CompileFromFile(
        const std::string& FilePath,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code) override final;

    virtual bool CompileShader(
        const std::string& ShaderSource,
        const std::string& EntryPoint,
        const TArray<ShaderDefine>* Defines,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        TArray<uint8>& Code) override final;

    bool GetReflection(D3D12BaseShader* Shader, ID3D12ShaderReflection** Reflection);
    bool GetLibraryReflection(D3D12BaseShader* Shader, ID3D12LibraryReflection** Reflection);

    bool HasRootSignature(D3D12BaseShader* Shader);

private:
    bool InternalCompileFromSource(
        IDxcBlob* SourceBlob, 
        LPCWSTR FilePath, 
        LPCWSTR Entrypoint, 
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        const TArray<ShaderDefine>* Defines,
        TArray<uint8>& Code);

    bool InternalGetReflection(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject);

    bool ValidateRayTracingShader(const TComPtr<IDxcBlob>& ShaderBlob, LPCWSTR Entrypoint);

private:
    TComPtr<IDxcCompiler>       DxCompiler;
    TComPtr<IDxcLibrary>        DxLibrary;
    TComPtr<IDxcLinker>         DxLinker;
    TComPtr<IDxcIncludeHandler> DxIncludeHandler;
    TComPtr<IDxcContainerReflection> DxReflection;
    HMODULE DxCompilerDLL;
};

extern D3D12ShaderCompiler*  gD3D12ShaderCompiler;
extern DxcCreateInstanceProc DxcCreateInstanceFunc;