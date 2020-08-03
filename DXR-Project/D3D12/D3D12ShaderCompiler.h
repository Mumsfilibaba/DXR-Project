#pragma once
#include <dxcapi.h>

#include <string>
#include <memory>

class D3D12ShaderCompiler
{
public:
	static IDxcBlob* CompileFromFile(const std::string& Filepath, const std::string& Entrypoint, const std::string& TargetProfile);
	static IDxcBlob* CompileFromSource(const std::string& Source, const std::string& Entrypoint, const std::string& TargetProfile);

private:
	static bool Initialize();
	static IDxcBlob* InternalCompileFromSource(IDxcBlob* SourceBlob, LPCWSTR FilePath, LPCWSTR Entrypoint, LPCWSTR TargetProfile);

	static Microsoft::WRL::ComPtr<IDxcCompiler> DxCompiler;
	static Microsoft::WRL::ComPtr<IDxcLibrary> DxLibrary;
	static Microsoft::WRL::ComPtr<IDxcLinker> DxLinker;
	static Microsoft::WRL::ComPtr<IDxcIncludeHandler> DxIncludeHandler;
	static HMODULE DxCompilerDLL;
};