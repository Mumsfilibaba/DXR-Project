#pragma once
#include <dxcapi.h>

#include <string>
#include <memory>

class D3D12ShaderCompiler
{
public:
	~D3D12ShaderCompiler();

	IDxcBlob* CompileFromFile(const std::string& InFilepath, const std::string& InEntrypoint, const std::string& InTargetProfile);
	IDxcBlob* CompileFromSource(const std::string& InSource, const std::string& InEntrypoint, const std::string& InTargetProfile);

	static D3D12ShaderCompiler* Create();
	static D3D12ShaderCompiler* Get();

private:
	D3D12ShaderCompiler();

	bool Initialize();

	IDxcBlob* InternalCompileFromSource(IDxcBlob* InSourceBlob, LPCWSTR InFilePath, LPCWSTR InEntrypoint, LPCWSTR InTargetProfile);

private:
	Microsoft::WRL::ComPtr<IDxcCompiler>		DxCompiler;
	Microsoft::WRL::ComPtr<IDxcLibrary>			DxLibrary;
	Microsoft::WRL::ComPtr<IDxcLinker>			DxLinker;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler>	DxIncludeHandler;

	static std::unique_ptr<D3D12ShaderCompiler> CompilerInstance;
};