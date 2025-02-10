#pragma once
#include "RHI/ShaderCompiler.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Shader.h"

#include <string>
#include <d3d12shader.h>

class FD3D12ShaderCompiler
{
public:
    FD3D12ShaderCompiler();
    ~FD3D12ShaderCompiler();

    bool Initialize();

    bool CompileFromFile(const FString& FilePath, const FString& EntryPoint, const TArray<FShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code);
    bool CompileShader(const FString& ShaderSource, const FString& EntryPoint, const TArray<FShaderDefine>* Defines, EShaderStage ShaderStage, EShaderModel ShaderModel, TArray<uint8>& Code);

    bool GetReflection(FD3D12Shader* Shader, ID3D12ShaderReflection** Reflection);
    bool GetLibraryReflection(FD3D12Shader* Shader, ID3D12LibraryReflection** Reflection);
    bool HasRootSignature(FD3D12Shader* Shader);

private:
    bool InternalCompileFromSource(
        IDxcBlob* SourceBlob,
        LPCWSTR FilePath,
        LPCWSTR Entrypoint,
        EShaderStage ShaderStage,
        EShaderModel ShaderModel,
        const TArray<FShaderDefine>* Defines,
        TArray<uint8>& Code);

    bool InternalGetReflection(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject);
    bool ValidateRayTracingShader(const TComPtr<IDxcBlob>& ShaderBlob, LPCWSTR Entrypoint);

    TComPtr<IDxcCompiler>            DxCompiler;
    TComPtr<IDxcLibrary>             DxLibrary;
    TComPtr<IDxcLinker>              DxLinker;
    TComPtr<IDxcIncludeHandler>      DxIncludeHandler;
    TComPtr<IDxcContainerReflection> DxReflection;
    HMODULE                          DxCompilerDLL;
};

extern FD3D12ShaderCompiler* GD3D12ShaderCompiler;
extern DxcCreateInstanceProc DxcCreateInstanceFunc;