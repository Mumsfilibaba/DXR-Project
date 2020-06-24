#pragma once
#include <dxcapi.h>

#include <string>
#include <memory>

class D3D12ShaderCompiler
{
public:
	~D3D12ShaderCompiler();

	IDxcBlob* CompileFromFile(const std::string& Filepath, const std::string& Entrypoint, const std::string& TargetProfile);
	IDxcBlob* CompileFromSource(const std::string& Source, const std::string& Entrypoint, const std::string& TargetProfile);

	static D3D12ShaderCompiler* Make();
	static D3D12ShaderCompiler* Get();

private:
	D3D12ShaderCompiler();

	bool Initialize();

	IDxcBlob* InternalCompileFromSource(IDxcBlob* SourceBlob, LPCWSTR FilePath, LPCWSTR Entrypoint, LPCWSTR TargetProfile);

private:
	Microsoft::WRL::ComPtr<IDxcCompiler>		DxCompiler;
	Microsoft::WRL::ComPtr<IDxcLibrary>			DxLibrary;
	Microsoft::WRL::ComPtr<IDxcLinker>			DxLinker;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler>	DxIncludeHandler;

	static std::unique_ptr<D3D12ShaderCompiler> CompilerInstance;
};