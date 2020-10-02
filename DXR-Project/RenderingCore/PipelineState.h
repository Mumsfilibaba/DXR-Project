#pragma once
#include "Core/RefCountedObject.h"

/*
* PipelineState
*/

class GraphicsPipelineState;
class ComputePipelineState;
class RayTracingPipelineState;

class PipelineState : public RefCountedObject
{
public:
	PipelineState() = default;
	~PipelineState() = default;

	virtual GraphicsPipelineState* AsGraphics()
	{
		return nullptr;
	}

	virtual const GraphicsPipelineState* AsGraphics() const
	{
		return nullptr;
	}

	virtual ComputePipelineState* AsCompute() 
	{
		return nullptr;
	}
	
	virtual const ComputePipelineState* AsCompute() const 
	{
		return nullptr;
	}

	virtual RayTracingPipelineState* AsRayTracing() 
	{
		return nullptr;
	}

	virtual const RayTracingPipelineState* AsRayTracing() const 
	{
		return nullptr;
	}
};

/*
* DepthStencilStateInitializer
*/

struct DepthStencilStateInitalizer
{

};

/*
* DepthStencilState
*/

class DepthStencilState
{
public:
	virtual Uint64 GetHash() = 0;
};

/*
* RasterizerStateInitializer
*/

struct RasterizerStateInitializer
{

};

/*
* RasterizerState
*/

class RasterizerState
{
public:
	virtual Uint64 GetHash() = 0;
};

/*
* BlendStateInitializer
*/

struct BlendStateInitializer
{

};

/*
* BlendState
*/

class BlendState
{
public:
	virtual Uint64 GetHash() = 0;
};

/*
* GraphicsPipelineState
*/

class GraphicsPipelineState : public PipelineState
{
public:
	GraphicsPipelineState() = default;
	~GraphicsPipelineState() = default;

	virtual GraphicsPipelineState* AsGraphics() override 
	{
		return this;
	}

	virtual const GraphicsPipelineState* AsGraphics() const override
	{
		return this;
	}

	virtual const RasterizerState* GetRasterizerState() const = 0;
	virtual const BlendState* GetBlendState() const = 0;
	virtual const DepthStencilState* GetDepthStencilState() const = 0;
};

/*
* ComputePipelineState
*/

class ComputePipelineState : public PipelineState
{
public:
	ComputePipelineState() = default;
	~ComputePipelineState() = default;

	virtual ComputePipelineState* AsCompute() override
	{
		return this;
	}

	virtual const ComputePipelineState* AsCompute() const override
	{
		return this;
	}
};

/*
* RayTracingPipelineState
*/

class RayTracingPipelineState : public PipelineState
{
public:
	RayTracingPipelineState() = default;
	~RayTracingPipelineState() = default;

	virtual RayTracingPipelineState* AsRayTracing() override
	{
		return this;
	}

	virtual const RayTracingPipelineState* AsRayTracing() const override
	{
		return this;
	}
};