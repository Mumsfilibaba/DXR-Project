#pragma once
#include "Resource.h"
#include "ShaderCompiler.h"

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
public:
	ComputeShader()		= default;
	~ComputeShader()	= default;
};

/*
* VertexShader
*/

class VertexShader : public Shader
{
public:
	VertexShader()	= default;
	~VertexShader()	= default;
};

/*
* HullShader
*/

class HullShader : public Shader
{
public:
	HullShader()	= default;
	~HullShader()	= default;
};

/*
* DomainShader
*/

class DomainShader : public Shader
{
public:
	DomainShader()	= default;
	~DomainShader()	= default;
};

/*
* GeometryShader
*/

class GeometryShader : public Shader
{
public:
	GeometryShader()	= default;
	~GeometryShader()	= default;
};

/*
* MeshShader
*/

class MeshShader : public Shader
{
public:
	MeshShader()	= default;
	~MeshShader()	= default;
};

/*
* AmplificationShader
*/

class AmplificationShader : public Shader
{
public:
	AmplificationShader()	= default;
	~AmplificationShader()	= default;
};

/*
* PixelShader
*/

class PixelShader : public Shader
{
public:
	PixelShader()	= default;
	~PixelShader()	= default;
};

/*
* RayGenShader
*/

class RayGenShader : public Shader
{
public:
	RayGenShader()	= default;
	~RayGenShader()	= default;
};

/*
* RayHitShader
*/

class RayHitShader : public Shader
{
public:
	RayHitShader()	= default;
	~RayHitShader()	= default;
};

/*
* RayMissShader
*/

class RayMissShader : public Shader
{
public:
	RayMissShader()		= default;
	~RayMissShader()	= default;
};