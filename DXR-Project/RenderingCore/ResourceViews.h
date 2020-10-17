#pragma once
#include "Resource.h"

/*
* ShaderResourceView
*/

class ShaderResourceView : public PipelineResource
{
public:
	ShaderResourceView()	= default;
	~ShaderResourceView()	= default;
};

/*
* UnorderedAccessView
*/

class UnorderedAccessView : public PipelineResource
{
public:
	UnorderedAccessView()	= default;
	~UnorderedAccessView()	= default;
};

/*
* DepthStencilView
*/

class DepthStencilView : public PipelineResource
{
public:
	DepthStencilView()	= default;
	~DepthStencilView()	= default;
};

/*
* RenderTargetView
*/

class RenderTargetView : public PipelineResource
{
public:
	RenderTargetView()	= default;
	~RenderTargetView()	= default;
};