#include "D3D12CommandContext.h"

/*
* D3D12CommandContext
*/

D3D12CommandContext::D3D12CommandContext(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, CmdList(nullptr)
	, CmdAllocator(nullptr)
{
}

D3D12CommandContext::~D3D12CommandContext()
{
}

bool D3D12CommandContext::Initialize()
{
	return false;
}

void D3D12CommandContext::Begin()
{
}

void D3D12CommandContext::End()
{
}

void D3D12CommandContext::ClearRenderTarget(RenderTargetView* RenderTargetView, const ColorClearValue& ClearColor)
{
	
}

void D3D12CommandContext::ClearDepthStencil(DepthStencilView* DepthStencilView, const DepthStencilClearValue& ClearValue) 
{
}

void D3D12CommandContext::BeginRenderPass()
{
}

void D3D12CommandContext::EndRenderPass()
{
}

void D3D12CommandContext::BindViewport(const Viewport& Viewport, Uint32 Slot)
{
}

void D3D12CommandContext::BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot)
{
}

void D3D12CommandContext::BindBlendFactor()
{
}

void D3D12CommandContext::BindPrimitiveTopology(EPrimitveTopologyType PrimitveTopologyType)
{
}

void D3D12CommandContext::BindVertexBuffers(VertexBuffer* const * VertexBuffers, Uint32 BufferCount, Uint32 BufferSlot)
{
}

void D3D12CommandContext::BindIndexBuffer(IndexBuffer* IndexBuffer, EFormat IndexFormat)
{
}

void D3D12CommandContext::BindRayTracingScene(RayTracingScene* RayTracingScene)
{
}

void D3D12CommandContext::BindRenderTargets(RenderTargetView* const * RenderTargetViews, Uint32 RenderTargetCount, DepthStencilView* DepthStencilView)
{
}

void D3D12CommandContext::BindGraphicsPipelineState(class GraphicsPipelineState* PipelineState)
{
}

void D3D12CommandContext::BindComputePipelineState(class ComputePipelineState* PipelineState)
{
}

void D3D12CommandContext::BindRayTracingPipelineState(class RayTracingPipelineState* PipelineState)
{
}

void D3D12CommandContext::BindConstantBuffers(Shader* Shader, ConstantBuffer* const * ConstantBuffers, Uint32 ConstantBufferCount, Uint32 StartSlot)
{
}

void D3D12CommandContext::BindShaderResourceViews(Shader* Shader, ShaderResourceView* const* ShaderResourceViews, Uint32 ShaderResourceViewCount, Uint32 StartSlot)
{
}

void D3D12CommandContext::BindUnorderedAccessViews(Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, Uint32 UnorderedAccessViewCount, Uint32 StartSlot)
{
}

void D3D12CommandContext::ResolveTexture(Texture* Destination, Texture* Source)
{
}

void D3D12CommandContext::UpdateBuffer(Buffer* Destination, Uint64 OffsetInBytes, Uint64 SizeInBytes, const VoidPtr SourceData)
{
}

void D3D12CommandContext::CopyBuffer(Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo)
{
}

void D3D12CommandContext::CopyTexture(Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo)
{
}

void D3D12CommandContext::BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
{
}

void D3D12CommandContext::BuildRayTracingScene(RayTracingScene* RayTracingScene)
{
}

void D3D12CommandContext::Draw(Uint32 VertexCount, Uint32 StartVertexLocation)
{
}

void D3D12CommandContext::DrawIndexed(Uint32 IndexCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation)
{
}

void D3D12CommandContext::DrawInstanced(Uint32 VertexCountPerInstance, Uint32 InstanceCount, Uint32 StartVertexLocation, Uint32 StartInstanceLocation)
{
}

void D3D12CommandContext::DrawIndexedInstanced(Uint32 IndexCountPerInstance, Uint32 InstanceCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation, Uint32 StartInstanceLocation)
{
}

void D3D12CommandContext::Dispatch(Uint32 WorkGroupsX, Uint32 WorkGroupsY, Uint32 WorkGroupsZ)
{
}

void D3D12CommandContext::DispatchRays(Uint32 Width, Uint32 Height, Uint32 Depth)
{
}