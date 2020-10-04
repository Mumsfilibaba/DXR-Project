#pragma once
#include "Resource.h"

/*
* CommandList
*/

class CommandList : public PipelineResource
{
public:
	virtual ~CommandList() = default;

	virtual bool Initialize() = 0;

	virtual void Begin() = 0;
	virtual void End() = 0;

	virtual void BeginRenderPass() = 0;
	virtual void EndRenderPass() = 0;

	virtual void BindViewPort() = 0;

	virtual void BindVertexBuffers() = 0;
	virtual void BindIndexBuffer() = 0;

	virtual void BindRenderTargetClearValues() = 0;
	virtual void BindRenderTargets() = 0;
	virtual void BindDepthStencil() = 0;

	virtual void BindShaderResourceView() = 0;
	virtual void BindUnorderedAccessView() = 0;
};