#pragma once
#include <dxcapi.h>

#include <string>

class D3D12ShaderCompiler
{
public:
	static IDxcBlob* CompileFromFile(const std::string& Filepath, const std::string& Entrypoint, const std::string& TargetProfile, const DxcDefine* Defines = nullptr, const Uint32 NumDefines = 0);
	static IDxcBlob* CompileFromSource(const std::string& Source, const std::string& Entrypoint, const std::string& TargetProfile, const DxcDefine* Defines = nullptr, const Uint32 NumDefines = 0);

private:
	static bool Initialize();
	static IDxcBlob* InternalCompileFromSource(IDxcBlob* SourceBlob, LPCWSTR FilePath, LPCWSTR Entrypoint, LPCWSTR TargetProfile, const DxcDefine* Defines = nullptr, const Uint32 NumDefines = 0);

	static Microsoft::WRL::ComPtr<IDxcCompiler> DxCompiler;
	static Microsoft::WRL::ComPtr<IDxcLibrary> DxLibrary;
	static Microsoft::WRL::ComPtr<IDxcLinker> DxLinker;
	static Microsoft::WRL::ComPtr<IDxcIncludeHandler> DxIncludeHandler;
	static HMODULE DxCompilerDLL;
};