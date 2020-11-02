#pragma once
#include "Resource.h"

class ComputeShader;
class VertexShader;
class PixelShader;

/*
* EShaderStage
*/

enum class EShaderStage
{
	ShaderStage_Vertex	= 1,
	ShaderStage_Pixel	= 2,
	ShaderStage_Compute	= 3,
};

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
	inline ShaderDefine(const std::string& InDefine)
		: Define(InDefine)
		, Value()
	{
	}

	inline ShaderDefine(const std::string& InDefine, const std::string& InValue)
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
	IShaderCompiler() = default;
	virtual ~IShaderCompiler() = default;

	virtual bool CompileFromFile(
		const std::string& FilePath, 
		const std::string& EntryPoint,
		const TArray<ShaderDefine>& Defines,
		EShaderStage ShaderStage, 
		EShaderModel ShaderModel, 
		TArray<Uint8>& Code) const = 0;

	virtual bool CompileShader(
		const std::string& ShaderSource, 
		const std::string& EntryPoint,
		const TArray<ShaderDefine>& Defines,
		EShaderStage ShaderStage, 
		EShaderModel ShaderModel, 
		TArray<Uint8>& Code) const = 0;
};

/*
* ShaderCompiler
*/

class ShaderCompiler
{
public:
	static bool Initialize();
	static void Release();

	FORCEINLINE static bool CompileFromFile(
		const std::string& FilePath,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>& Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<Uint8>& Code)
	{
		return Instance->CompileFromFile(
			FilePath,
			EntryPoint,
			Defines,
			ShaderStage,
			ShaderModel,
			Code);
	}

	FORCEINLINE static bool CompileShader(
		const std::string& ShaderSource,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>& Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<Uint8>& Code)
	{
		return Instance->CompileShader(
			ShaderSource,
			EntryPoint,
			Defines,
			ShaderStage,
			ShaderModel,
			Code);
	}

private:
	static IShaderCompiler* Instance;
};

/*
* Shader
*/

class Shader : public PipelineResource
{
public:
	Shader()	= default;
	~Shader()	= default;

	// Dynamic Casting
	virtual ComputeShader* AsComputeShader()
	{
		return nullptr;
	}

	virtual const ComputeShader* AsComputeShader() const
	{
		return nullptr;
	}

	virtual VertexShader* AsVertexShader()
	{
		return nullptr;
	}

	virtual const VertexShader* AsVertexShader() const
	{
		return nullptr;
	}

	virtual PixelShader* AsPixelShader()
	{
		return nullptr;
	}

	virtual const PixelShader* AsPixelShader() const
	{
		return nullptr;
	}
};

/*
* ComputeShader
*/

class ComputeShader : public Shader
{
};

/*
* VertexShader
*/

class VertexShader : public Shader
{
};

/*
* PixelShader
*/

class PixelShader : public Shader
{
};