#pragma once
#include "Resource.h"

/*
* CommandList
*/

class CommandList
{
public:
	~CommandList() = default;

	bool Initialize();

	void Begin();
	void End();

	void ClearRenderTarget();
	void ClearDepthStencil();

	void BeginRenderPass();
	void EndRenderPass();

	void BindViewport(const Viewport& Viewport, Uint32 Slot);
	void BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot);

	void BindBlendFactor();

	void BindPrimitiveTopology();
	void BindVertexBuffers();
	void BindIndexBuffer();

	void BindRenderTargets();

	void BindGraphicsPipelineState();
	void BindComputePipelineState();
	void BindRayTracingPipelineState();

	void BindConstantBuffer();
	void BindShaderResourceView();
	void BindUnorderedAccessView();

	void ResolveTexture();

	void BuildRayTracingGeometry();
	void BuildRayTracingScene();

	void Draw(Uint32 VertexCount, Uint32 StartVertexLocation);
	void DrawIndexed(Uint32 IndexCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation);
	void DrawInstanced(Uint32 VertexCountPerInstance, Uint32 InstanceCount, Uint32 StartVertexLocation, Uint32 StartInstanceLocation);
	void DrawIndexedInstanced(Uint32 IndexCountPerInstance, Uint32 InstanceCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation, Uint32 StartInstanceLocation);

	void Dispatch(Uint32 WorkGroupsX, Uint32 WorkGroupsY, Uint32 WorkGroupsZ);
	void DispatchRays(Uint32 Width, Uint32 Height, Uint32 Depth);

private:
	VoidPtr AllocateStorage(Uint64 SizeInBytes);
};