#pragma once
#include "RenderingCore/ICommandContext.h"

#include "D3D12DeviceChild.h"
#include "D3D12RootSignature.h"

class D3D12CommandQueue;
class D3D12CommandList;
class D3D12CommandAllocator;

/*
* D3D12GenerateMipsHelper
*/

struct D3D12GenerateMipsHelper
{
	TUniquePtr<class D3D12ComputePipelineState>		GenerateMipsTex2D_PSO;
	TUniquePtr<class D3D12ComputePipelineState>		GenerateMipsTexCube_PSO;
	TUniquePtr<class D3D12RootSignature>			GenerateMipsRootSignature;
	TUniquePtr<class D3D12DescriptorTable>			SRVDescriptorTable;
	TArray<TUniquePtr<class D3D12DescriptorTable>>	UAVDescriptorTables;
	TUniquePtr<class D3D12UnorderedAccessView>		NULLView;
};

/*
* D3D12CommandContext 
*/

class D3D12CommandContext : public ICommandContext, public D3D12DeviceChild
{
public:
	D3D12CommandContext(class D3D12Device* InDevice, D3D12CommandQueue* InCmdQueue, const D3D12DefaultRootSignatures& InDefaultRootSignatures);
	~D3D12CommandContext();

	bool Initialize();

	FORCEINLINE D3D12CommandList& GetCommandList() const
	{
		VALIDATE(CmdList != nullptr);
		return *CmdList;
	}

public:
	virtual void Begin() override final;
	virtual void End() override final;

	virtual void ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorClearValue& ClearColor) override final;
	virtual void ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilClearValue& ClearValue) override final;
	virtual void ClearUnorderedAccessView(UnorderedAccessView* UnorderedAccessView, const ColorClearValue& ClearColor) override final;

	virtual void BeginRenderPass() override final;
	virtual void EndRenderPass() override final;

	virtual void BindViewport(const Viewport& Viewport, UInt32 Slot) override final;
	virtual void BindScissorRect(const ScissorRect& ScissorRect, UInt32 Slot) override final;

	virtual void BindBlendFactor(const ColorClearValue& Color) override final;

	virtual void BindPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType) override final;
	virtual void BindVertexBuffers(
		VertexBuffer* const * VertexBuffers, 
		UInt32 BufferCount, 
		UInt32 BufferSlot) override final;

	virtual void BindIndexBuffer(IndexBuffer* IndexBuffer) override final;
	virtual void BindRayTracingScene(RayTracingScene* RayTracingScene) override final;

	virtual void BindRenderTargets(
		RenderTargetView* const * RenderTargetViews, 
		UInt32 RenderTargetCount, 
		DepthStencilView* DepthStencilView) override final;

	virtual void BindGraphicsPipelineState(class GraphicsPipelineState* PipelineState) override final;
	virtual void BindComputePipelineState(class ComputePipelineState* PipelineState) override final;
	virtual void BindRayTracingPipelineState(class RayTracingPipelineState* PipelineState) override final;

	virtual void BindConstantBuffers(
		Shader* Shader, 
		ConstantBuffer* const * ConstantBuffers, 
		UInt32 ConstantBufferCount, 
		UInt32 StartSlot) override final;

	virtual void BindShaderResourceViews(
		Shader* Shader, 
		ShaderResourceView* const* ShaderResourceViews, 
		UInt32 ShaderResourceViewCount, 
		UInt32 StartSlot) override final;

	virtual void BindUnorderedAccessViews(
		Shader* Shader, 
		UnorderedAccessView* const* UnorderedAccessViews, 
		UInt32 UnorderedAccessViewCount, 
		UInt32 StartSlot) override final;

	virtual void ResolveTexture(Texture* Destination, Texture* Source) override final;

	virtual void UpdateBuffer(
		Buffer* Destination, 
		UInt64 OffsetInBytes, 
		UInt64 SizeInBytes, 
		const Void* SourceData) override final;

	virtual void UpdateTexture2D(
		Texture2D* Destination,
		UInt32 Width,
		UInt32 Height,
		UInt32 MipLevel,
		const Void* SourceData) override final;

	virtual void CopyBuffer(
		Buffer* Destination, 
		Buffer* Source, 
		const CopyBufferInfo& CopyInfo) override final;

	virtual void CopyTexture(
		Texture* Destination, 
		Texture* Source, 
		const CopyTextureInfo& CopyTextureInfo) override final;

	virtual void BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry) 	override final;
	virtual void BuildRayTracingScene(RayTracingScene* RayTracingScene) 			override final;

	virtual void GenerateMips(Texture* Texture) override final;

	virtual void TransitionTexture(
		Texture* Texture,
		EResourceState BeforeState,
		EResourceState AfterState) override final;

	virtual void TransitionBuffer(
		Buffer* Buffer,
		EResourceState BeforeState,
		EResourceState AfterState) override final;

	virtual void UnorderedAccessTextureBarrier(Texture* Texture) override final;

	virtual void Draw(UInt32 VertexCount, UInt32 StartVertexLocation) override final;
	
	virtual void DrawIndexed(
		UInt32 IndexCount, 
		UInt32 StartIndexLocation, 
		UInt32 BaseVertexLocation) override final;
	
	virtual void DrawInstanced(
		UInt32 VertexCountPerInstance, 
		UInt32 InstanceCount, 
		UInt32 StartVertexLocation, 
		UInt32 StartInstanceLocation) override final;
	
	virtual void DrawIndexedInstanced(
		UInt32 IndexCountPerInstance, 
		UInt32 InstanceCount, 
		UInt32 StartIndexLocation, 
		UInt32 BaseVertexLocation, 
		UInt32 StartInstanceLocation) override final;

	virtual void Dispatch(
		UInt32 WorkGroupsX, 
		UInt32 WorkGroupsY, 
		UInt32 WorkGroupsZ) override final;

	virtual void DispatchRays(UInt32 Width, UInt32 Height, UInt32 Depth) override final;

	virtual void Flush() override final;

private:
	D3D12CommandQueue* CmdQueue;
	class D3D12CommandList* CmdList; 
	class D3D12Fence* Fence;
	UInt64 FenceValue = 0;

	TArray<D3D12CommandAllocator*> CmdAllocators;
	UInt32 NextCmdAllocator = 0;

	D3D12DefaultRootSignatures DefaultRootSignatures;
	
	Bool IsReady = false;
};