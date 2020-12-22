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
* Shader
*/

class Shader : public PipelineResource
{
public:
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
* HullShader
*/

class HullShader : public Shader
{
};

/*
* DomainShader
*/

class DomainShader : public Shader
{
};

/*
* GeometryShader
*/

class GeometryShader : public Shader
{
};

/*
* MeshShader
*/

class MeshShader : public Shader
{
};

/*
* AmplificationShader
*/

class AmplificationShader : public Shader
{
};

/*
* PixelShader
*/

class PixelShader : public Shader
{
};

/*
* RayGenShader
*/

class RayGenShader : public Shader
{
};

/*
* RayHitShader
*/

class RayHitShader : public Shader
{
};

/*
* RayMissShader
*/

class RayMissShader : public Shader
{
};