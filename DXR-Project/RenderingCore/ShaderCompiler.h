#pragma once
#include "Defines.h"
#include "Types.h"

/*
* EShaderStage
*/

enum class EShaderStage
{
	ShaderStage_Vertex			= 1,
	ShaderStage_Hull			= 2,
	ShaderStage_Domain			= 3,
	ShaderStage_Geometry		= 4,
	ShaderStage_Mesh			= 5,
	ShaderStage_Amplification	= 6,
	ShaderStage_Pixel			= 7,
	ShaderStage_Compute			= 8,
	ShaderStage_RayGen			= 9,
	ShaderStage_RayAnyHit		= 10,
	ShaderStage_RayClosestHit	= 11,
	ShaderStage_RayMiss			= 12,
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
		const TArray<ShaderDefine>* Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<UInt8>& Code) const = 0;

	virtual bool CompileShader(
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
	friend class RenderingAPI;

public:
	FORCEINLINE static bool CompileFromFile(
		const std::string& FilePath,
		const std::string& EntryPoint,
		const TArray<ShaderDefine>* Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<UInt8>& Code)
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
		const TArray<ShaderDefine>* Defines,
		EShaderStage ShaderStage,
		EShaderModel ShaderModel,
		TArray<UInt8>& Code)
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
	inline static TSharedPtr<IShaderCompiler> Instance = nullptr;
};