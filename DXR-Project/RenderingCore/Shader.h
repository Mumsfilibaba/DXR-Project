#pragma once
#include "Resource.h"

class ComputeShader;
class VertexShader;
class PixelShader;

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