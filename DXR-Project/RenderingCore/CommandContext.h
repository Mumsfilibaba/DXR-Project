#pragma once
#include "RenderingCore.h"

class Buffer;
class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class StructuredBuffer;
class Shader;
class RenderTargetView;
class ShaderResourceView;
class DepthStencilView;
class UnorderedAccessView;
class Texture;
class RayTracingGeometry;
class RayTracingScene;
class GraphicsPipelineState;
class ComputePipelineState;
class RayTracingPipelineState;

/*
* ICommandContext
*/

class ICommandContext
{
public:
	virtual ~ICommandContext() = default;

	virtual bool Initialize() = 0;

	virtual void Begin() = 0;
	virtual void End() = 0;

	virtual void ClearRenderTarget(RenderTargetView* RenderTargetView, const ColorClearValue& ClearColor) = 0;
	virtual void ClearDepthStencil(DepthStencilView* DepthStencilView, const DepthStencilClearValue& ClearValue) = 0;

	virtual void BeginRenderPass() = 0;
	virtual void EndRenderPass() = 0;

	virtual void BindViewport(const Viewport& Viewport, Uint32 Slot) = 0;
	virtual void BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot) = 0;

	virtual void BindBlendFactor() = 0;

	virtual void BindVertexBuffers(
		VertexBuffer* const * VertexBuffers, 
		Uint32 BufferCount, 
		Uint32 BufferSlot) = 0;

	virtual void BindIndexBuffer(IndexBuffer* IndexBuffer) = 0;
	virtual void BindPrimitiveTopology(EPrimitveTopologyType PrimitveTopologyType) = 0;
	virtual void BindRayTracingScene(RayTracingScene* RayTracingScene) = 0;

	virtual void BindRenderTargets(
		RenderTargetView* const * RenderTargetViews, 
		Uint32 RenderTargetCount, 
		DepthStencilView* DepthStencilView) = 0;

	virtual void BindGraphicsPipelineState(class GraphicsPipelineState* PipelineState) = 0;
	virtual void BindComputePipelineState(class ComputePipelineState* PipelineState) = 0;
	virtual void BindRayTracingPipelineState(class RayTracingPipelineState* PipelineState) = 0;

	virtual void BindShaderResourceViews(
		Shader* Shader, 
		ShaderResourceView* const* ShaderResourceViews, 
		Uint32 ShaderResourceViewCount, 
		Uint32 StartSlot) = 0;

	virtual void BindUnorderedAccessViews(
		Shader* Shader, 
		UnorderedAccessView* const* UnorderedAccessViews, 
		Uint32 UnorderedAccessViewCount, 
		Uint32 StartSlot) = 0;

	virtual void BindConstantBuffers(
		Shader* Shader,
		ConstantBuffer* const * ConstantBuffers, 
		Uint32 ConstantBufferCount, 
		Uint32 StartSlot) = 0;

	virtual void ResolveTexture(Texture* Destination, Texture* Source) = 0;

	virtual void UpdateBuffer(
		Buffer* Destination, 
		Uint64 OffsetInBytes, 
		Uint64 SizeInBytes, 
		const VoidPtr SourceData) = 0;

	virtual void CopyBuffer(
		Buffer* Destination, 
		Buffer* Source, 
		const CopyBufferInfo& CopyInfo) = 0;

	virtual void CopyTexture(
		Texture* Destination, 
		Texture* Source, 
		const CopyTextureInfo& CopyTextureInfo) = 0;

	virtual void BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry) = 0;
	virtual void BuildRayTracingScene(RayTracingScene* RayTracingScene) = 0;

	virtual void Draw(
		Uint32 VertexCount, 
		Uint32 StartVertexLocation) = 0;

	virtual void DrawIndexed(
		Uint32 IndexCount, 
		Uint32 StartIndexLocation, 
		Uint32 BaseVertexLocation) = 0;

	virtual void DrawInstanced(
		Uint32 VertexCountPerInstance, 
		Uint32 InstanceCount, 
		Uint32 StartVertexLocation, 
		Uint32 StartInstanceLocation) = 0;

	virtual void DrawIndexedInstanced(
		Uint32 IndexCountPerInstance, 
		Uint32 InstanceCount, 
		Uint32 StartIndexLocation, 
		Uint32 BaseVertexLocation, 
		Uint32 StartInstanceLocation) = 0;

	virtual void Dispatch(
		Uint32 WorkGroupsX, 
		Uint32 WorkGroupsY, 
		Uint32 WorkGroupsZ) = 0;

	virtual void DispatchRays(
		Uint32 Width, 
		Uint32 Height, 
		Uint32 Depth) = 0;
};