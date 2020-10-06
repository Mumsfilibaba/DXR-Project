#pragma once
#include "RenderingCore/CommandContext.h"

#include "D3D12DeviceChild.h"

class D3D12CommandContext : public CommandContext, public D3D12DeviceChild
{
public:
    D3D12CommandContext(class D3D12Device* InDevice);
    ~D3D12CommandContext();

    virtual bool Initialize() override final;

	virtual void Begin() override final;
	virtual void End() override final;

	virtual void ClearRenderTarget(RenderTargetView* RenderTargetView, const ColorClearValue& ClearColor) override final;
	virtual void ClearDepthStencil(DepthStencilView* DepthStencilView, const DepthStencilClearValue& ClearValue) override final;

	virtual void BeginRenderPass() override final;
	virtual void EndRenderPass() override final;

	virtual void BindViewport(const Viewport& Viewport, Uint32 Slot) override final;
	virtual void BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot) override final;

	virtual void BindBlendFactor() override final;

	virtual void BindPrimitiveTopology(EPrimitveTopologyType PrimitveTopologyType) override final;
	virtual void BindVertexBuffers(Buffer* const * VertexBuffers, Uint32 BufferCount, Uint32 BufferSlot) override final;
	virtual void BindIndexBuffer(Buffer* IndexBuffer, EFormat IndexFormat) override final;
	virtual void BindRayTracingScene(RayTracingScene* RayTracingScene) override final;

	virtual void BindRenderTargets(RenderTargetView* const * RenderTargetViews, Uint32 RenderTargetCount, DepthStencilView* DepthStencilView) override final;

	virtual void BindGraphicsPipelineState(class GraphicsPipelineState* PipelineState) override final;
	virtual void BindComputePipelineState(class ComputePipelineState* PipelineState) override final;
	virtual void BindRayTracingPipelineState(class RayTracingPipelineState* PipelineState) override final;

	virtual void BindConstantBuffers(Shader* Shader, Buffer* const * ConstantBuffers, Uint32 ConstantBufferCount, Uint32 StartSlot) override final;
	virtual void BindShaderResourceViews(Shader* Shader, ShaderResourceView* const* ShaderResourceViews, Uint32 ShaderResourceViewCount, Uint32 StartSlot) override final;
	virtual void BindUnorderedAccessViews(Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, Uint32 UnorderedAccessViewCount, Uint32 StartSlot) override final;

	virtual void ResolveTexture(Texture* Destination, Texture* Source) override final;

	virtual void UpdateBuffer(Buffer* Destination, Uint64 OffsetInBytes, Uint64 SizeInBytes, const VoidPtr SourceData) override final;
	virtual void CopyBuffer(Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo) override final;

	virtual void CopyTexture(Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo) override final;

	virtual void BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry) override final;
	virtual void BuildRayTracingScene(RayTracingScene* RayTracingScene) override final;

	virtual void Draw(Uint32 VertexCount, Uint32 StartVertexLocation) override final;
	virtual void DrawIndexed(Uint32 IndexCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation) override final;
	virtual void DrawInstanced(Uint32 VertexCountPerInstance, Uint32 InstanceCount, Uint32 StartVertexLocation, Uint32 StartInstanceLocation) override final;
	virtual void DrawIndexedInstanced(Uint32 IndexCountPerInstance, Uint32 InstanceCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation, Uint32 StartInstanceLocation) override final;

	virtual void Dispatch(Uint32 WorkGroupsX, Uint32 WorkGroupsY, Uint32 WorkGroupsZ) override final;
	virtual void DispatchRays(Uint32 Width, Uint32 Height, Uint32 Depth) override final;

private:
    class D3D12CommandAllocator* CmdAllocator = nullptr;
    class D3D12CommandList* CmdList = nullptr; 
};