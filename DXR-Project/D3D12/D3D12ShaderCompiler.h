#pragma once
#include <dxcapi.h>

#include <string>
#include <memory>

class D3D12ShaderCompiler
{
	D3D12ShaderCompiler(D3D12ShaderCompiler&& Other)		= delete;
	D3D12ShaderCompiler(const D3D12ShaderCompiler& Other)	= delete;

	D3D12ShaderCompiler& operator=(D3D12ShaderCompiler&& Other)			= delete;
	D3D12ShaderCompiler& operator=(const D3D12ShaderCompiler& Other)	= delete;

public:
	~D3D12ShaderCompiler();

	IDxcBlob* CompileFromFile(const std::string& Filepath, const std::string& Entrypoint, const std::string& TargetProfile);

	static D3D12ShaderCompiler* Create();
	static D3D12ShaderCompiler* Get();

private:
	D3D12ShaderCompiler();

	bool Init();

private:
	Microsoft::WRL::ComPtr<IDxcCompiler>		DxCompiler;
	Microsoft::WRL::ComPtr<IDxcLibrary>			DxLibrary;
	Microsoft::WRL::ComPtr<IDxcLinker>			DxLinker;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler>	DxIncludeHandler;

	static std::unique_ptr<D3D12ShaderCompiler> CompilerInstance;
};