#pragma once
#include "RenderingCore/Shader.h"

#include <dxcapi.h>

#include <string>

/*
* D3D12ShaderCompiler
*/

class D3D12ShaderCompiler : public IShaderCompiler
{
	friend class D3D12RenderingAPI;

public:
	D3D12ShaderCompiler();
	~D3D12ShaderCompiler();

	virtual bool CompileFromFile(
		const std::string& FilePath,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>& Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<Uint8>& Code) const override final;

	virtual bool CompileShader(
		const std::string& ShaderSource,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>& Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<Uint8>& Code) const override final;

private:
	bool Initialize();

	bool InternalCompileFromSource(
		IDxcBlob* SourceBlob, 
		LPCWSTR FilePath, 
		LPCWSTR Entrypoint, 
		LPCWSTR TargetProfile, 
		const TArray<ShaderDefine>& Defines,
		TArray<Uint8>& Code) const;

private:
	Microsoft::WRL::ComPtr<IDxcCompiler>	DxCompiler;
	Microsoft::WRL::ComPtr<IDxcLibrary>		DxLibrary;
	Microsoft::WRL::ComPtr<IDxcLinker>		DxLinker;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> DxIncludeHandler;
	HMODULE DxCompilerDLL;
};