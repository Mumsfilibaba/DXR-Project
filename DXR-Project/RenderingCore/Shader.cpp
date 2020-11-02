#include "Shader.h"
#include "RenderingAPI.h"

/*
* ShaderCompiler
*/

IShaderCompiler* ShaderCompiler::Instance = nullptr;

bool ShaderCompiler::Initialize()
{
	Instance = RenderingAPI::CreateShaderCompiler();
	if (Instance)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ShaderCompiler::Release()
{
	SAFEDELETE(Instance);
}