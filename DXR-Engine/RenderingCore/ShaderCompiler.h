#pragma once
#include "Shader.h"

/*
* EShaderModel
*/

enum class EShaderModel
{
	ShaderModel_5_0 = 1,
	ShaderModel_5_1 = 2,
	ShaderModel_6_0 = 3,
};

/*
* ShaderDefine
*/

struct ShaderDefine
{
	ShaderDefine(const std::string& InDefine)
		: Define(InDefine)
		, Value()
	{
	}

	ShaderDefine(const std::string& InDefine, const std::string& InValue)
		: Define(InDefine)
		, Value(InValue)
	{
	}

	std::string Define;
	std::string Value;
};

/*
* IShaderCompiler - Interface for compiling HLSL source (HLSL is only supported)
*/

class IShaderCompiler
{
public:
	virtual ~IShaderCompiler() = default;

	virtual Bool CompileFromFile(
		const std::string& FilePath,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>* Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<UInt8>& Code) const = 0;

	virtual Bool CompileShader(
		const std::string& ShaderSource,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>* Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<UInt8>& Code) const = 0;
};

/*
* ShaderCompiler
*/

class ShaderCompiler
{
	friend class RenderLayer;

public:
	FORCEINLINE static Bool CompileFromFile(
		const std::string& FilePath,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>* Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<UInt8>& Code)
	{
		return GlobalShaderCompiler->CompileFromFile(
			FilePath,
			EntryPoint,
			Defines,
			ShaderStage,
			ShaderModel,
			Code);
	}

	FORCEINLINE static Bool CompileShader(
		const std::string& ShaderSource,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>* Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<UInt8>& Code)
	{
		return GlobalShaderCompiler->CompileShader(
			ShaderSource,
			EntryPoint,
			Defines,
			ShaderStage,
			ShaderModel,
			Code);
	}
};