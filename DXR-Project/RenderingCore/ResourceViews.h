#pragma once
#include "Resource.h"

/*
* ShaderResourceView
*/

struct ShaderResourceViewInitializer
{
};

class ShaderResourceView : public PipelineResource
{
};

/*
* RenderTargetView
*/

struct RenderTargetViewInitializer
{
};

class RenderTargetView : public PipelineResource
{
};

/*
* DepthStencilView
*/

struct DepthStencilViewInitializer
{
};

class DepthStencilView : public PipelineResource
{
};

/*
* UnorderedAccessView
*/

struct UnorderedAccessViewInitializer
{
};

class UnorderedAccessView : public PipelineResource
{
};