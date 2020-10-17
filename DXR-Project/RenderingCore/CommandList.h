#pragma once
#include "Resource.h"
#include "Texture.h"
#include "Buffer.h"
#include "RayTracing.h"
#include "RenderCommand.h"

#include "Memory/StackAllocator.h"

class RenderTargetView;
class DepthStencilView;
class ShaderResourceView;
class UnorderedAccessView;
class Shader;

/*
* CommandList
*/

class CommandList
{
	friend class CommandListExecutor;

public:
	inline CommandList()
		: CmdAllocator()
		, First(nullptr)
		, Last(nullptr)
	{
	}

	inline ~CommandList()
	{
		Reset();
	}

	FORCEINLINE void CommandList::Begin()
	{
		InsertCommand<BeginRenderCommand>();
	}

	FORCEINLINE void CommandList::End()
	{
		InsertCommand<EndRenderCommand>();
	}

	FORCEINLINE void CommandList::ClearRenderTarget(
		RenderTargetView* RenderTargetView, 
		const ColorClearValue& ClearColor)
	{
		RenderTargetView->AddRef();
		InsertCommand<ClearRenderTargetRenderCommand>(RenderTargetView, ClearColor);
	}

	FORCEINLINE void CommandList::ClearDepthStencil(
		DepthStencilView* DepthStencilView, 
		const DepthStencilClearValue& ClearValue)
	{
		DepthStencilView->AddRef();
		InsertCommand<ClearDepthStencilRenderCommand>(DepthStencilView, ClearValue);
	}

	FORCEINLINE void CommandList::BeginRenderPass()
	{
		InsertCommand<BeginRenderPassRenderCommand>();
	}

	FORCEINLINE void CommandList::EndRenderPass()
	{
		InsertCommand<EndRenderPassRenderCommand>();
	}

	FORCEINLINE void CommandList::BindViewport(const Viewport& Viewport, Uint32 Slot)
	{
		InsertCommand<BindViewportRenderCommand>(Viewport, Slot);
	}

	FORCEINLINE void CommandList::BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot)
	{
		InsertCommand<BindScissorRectRenderCommand>(ScissorRect, Slot);
	}

	FORCEINLINE void CommandList::BindBlendFactor()
	{
		InsertCommand<BindBlendFactorRenderCommand>();
	}

	FORCEINLINE void CommandList::BindPrimitiveTopology(EPrimitveTopologyType PrimitveTopologyType)
	{
		InsertCommand<BindPrimitiveTopologyRenderCommand>(PrimitveTopologyType);
	}

	FORCEINLINE void CommandList::BindVertexBuffers(
		VertexBuffer* const* VertexBuffers, 
		Uint32 VertexBufferCount, 
		Uint32 BufferSlot)
	{
		VoidPtr BufferMemory	= CmdAllocator.Allocate(sizeof(VertexBuffer*) * VertexBufferCount, 1);
		VertexBuffer** Buffers	= reinterpret_cast<VertexBuffer**>(BufferMemory);
		for (Uint32 i = 0; i < VertexBufferCount; i++)
		{
			Buffers[i] = VertexBuffers[i];
			Buffers[i]->AddRef();
		}

		InsertCommand<BindVertexBuffersRenderCommand>(Buffers, VertexBufferCount, BufferSlot);
	}

	FORCEINLINE void CommandList::BindIndexBuffer(IndexBuffer* IndexBuffer, EFormat IndexFormat)
	{
		IndexBuffer->AddRef();
		InsertCommand<BindIndexBufferRenderCommand>(IndexBuffer, IndexFormat);
	}

	FORCEINLINE void CommandList::BindRayTracingScene(RayTracingScene* RayTracingScene)
	{
		RayTracingScene->AddRef();
		InsertCommand<BindRayTracingSceneRenderCommand>(RayTracingScene);
	}

	FORCEINLINE void CommandList::BindRenderTargets(
		RenderTargetView* const* RenderTargetViews, 
		Uint32 RenderTargetCount, 
		DepthStencilView* DepthStencilView)
	{
		VoidPtr RenderTargetMemory = CmdAllocator.Allocate(sizeof(RenderTargetView*) * RenderTargetCount, 1);
		RenderTargetView** RenderTargets = reinterpret_cast<RenderTargetView**>(RenderTargetMemory);
		for (Uint32 i = 0; i < RenderTargetCount; i++)
		{
			RenderTargets[i] = RenderTargetViews[i];
			RenderTargets[i]->AddRef();
		}

		InsertCommand<BindRenderTargetsRenderCommand>(RenderTargets, RenderTargetCount, DepthStencilView);
	}

	FORCEINLINE void CommandList::BindGraphicsPipelineState(GraphicsPipelineState* PipelineState)
	{
		PipelineState->AddRef();
		InsertCommand<BindGraphicsPipelineStateRenderCommand>(PipelineState);
	}

	FORCEINLINE void CommandList::BindComputePipelineState(ComputePipelineState* PipelineState)
	{
		PipelineState->AddRef();
		InsertCommand<BindComputePipelineStateRenderCommand>(PipelineState);
	}

	FORCEINLINE void CommandList::BindRayTracingPipelineState(RayTracingPipelineState* PipelineState)
	{
		PipelineState->AddRef();
		InsertCommand<BindRayTracingPipelineStateRenderCommand>(PipelineState);
	}

	FORCEINLINE void CommandList::BindConstantBuffers(
		Shader* Shader, 
		ConstantBuffer* const* ConstantBuffers, 
		Uint32 ConstantBufferCount, 
		Uint32 StartSlot)
	{
		InsertCommand<BindConstantBuffersRenderCommand>(
			Shader, 
			ConstantBuffers, 
			ConstantBufferCount, 
			StartSlot);
	}

	FORCEINLINE void CommandList::BindShaderResourceViews(
		Shader* Shader, 
		ShaderResourceView* const* ShaderResourceViews, 
		Uint32 ShaderResourceViewCount, 
		Uint32 StartSlot)
	{
		InsertCommand<BindShaderResourceViewsRenderCommand>(
			Shader, 
			ShaderResourceViews, 
			ShaderResourceViewCount, 
			StartSlot);
	}

	FORCEINLINE void CommandList::BindUnorderedAccessViews(
		Shader* Shader, 
		UnorderedAccessView* const* UnorderedAccessViews, 
		Uint32 UnorderedAccessViewCount, 
		Uint32 StartSlot)
	{
		InsertCommand<BindUnorderedAccessViewsRenderCommand>(
			Shader,
			UnorderedAccessViews,
			UnorderedAccessViewCount,
			StartSlot);
	}

	FORCEINLINE void CommandList::ResolveTexture(Texture* Destination, Texture* Source)
	{
		Destination->AddRef();
		Source->AddRef();
		InsertCommand<ResolveTextureRenderCommand>(Destination, Source);
	}

	FORCEINLINE void CommandList::UpdateBuffer(
		Buffer* Destination, 
		Uint64 DestinationOffsetInBytes, 
		Uint64 SizeInBytes, 
		const VoidPtr SourceData)
	{
		VoidPtr TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
		Memory::Memcpy(TempSourceData, SourceData, SizeInBytes);

		Destination->AddRef();
		InsertCommand<UpdateBufferRenderCommand>(
			Destination, 
			DestinationOffsetInBytes, 
			SizeInBytes, 
			SourceData);
	}

	FORCEINLINE void CommandList::CopyBuffer(
		Buffer* Destination, 
		Buffer* Source, 
		const CopyBufferInfo& CopyInfo)
	{
		Destination->AddRef();
		Source->AddRef();
		InsertCommand<CopyBufferRenderCommand>(Destination, Source, CopyInfo);
	}

	FORCEINLINE void CommandList::CopyTexture(
		Texture* Destination, 
		Texture* Source, 
		const CopyTextureInfo& CopyTextureInfo)
	{
		Destination->AddRef();
		Source->AddRef();
		InsertCommand<CopyTextureRenderCommand>(Destination, Source, CopyTextureInfo);
	}

	FORCEINLINE void CommandList::BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
	{
		RayTracingGeometry->AddRef();
		InsertCommand<BuildRayTracingGeometryRenderCommand>(RayTracingGeometry);
	}

	FORCEINLINE void CommandList::BuildRayTracingScene(RayTracingScene* RayTracingScene)
	{
		RayTracingScene->AddRef();
		InsertCommand<BuildRayTracingSceneRenderCommand>(RayTracingScene);
	}

	FORCEINLINE void CommandList::Draw(Uint32 VertexCount, Uint32 StartVertexLocation)
	{
		InsertCommand<DrawRenderCommand>(VertexCount, StartVertexLocation);
	}

	FORCEINLINE void CommandList::DrawIndexed(Uint32 IndexCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation)
	{
		InsertCommand< DrawIndexedRenderCommand>(IndexCount, StartIndexLocation, BaseVertexLocation);
	}

	FORCEINLINE void CommandList::DrawInstanced(
		Uint32 VertexCountPerInstance, 
		Uint32 InstanceCount, 
		Uint32 StartVertexLocation, 
		Uint32 StartInstanceLocation)
	{
		InsertCommand<DrawInstancedRenderCommand>(
			VertexCountPerInstance,
			InstanceCount,
			StartVertexLocation,
			StartInstanceLocation);
	}

	FORCEINLINE void CommandList::DrawIndexedInstanced(
		Uint32 IndexCountPerInstance, 
		Uint32 InstanceCount, 
		Uint32 StartIndexLocation, 
		Uint32 BaseVertexLocation, 
		Uint32 StartInstanceLocation)
	{
		InsertCommand<DrawIndexedInstancedRenderCommand>(
			IndexCountPerInstance,
			InstanceCount,
			StartIndexLocation,
			BaseVertexLocation,
			StartInstanceLocation);
	}

	FORCEINLINE void CommandList::Dispatch(Uint32 WorkGroupsX, Uint32 WorkGroupsY, Uint32 WorkGroupsZ)
	{
		InsertCommand<DispatchComputeRenderCommand>(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
	}

	FORCEINLINE void CommandList::DispatchRays(Uint32 Width, Uint32 Height, Uint32 Depth)
	{
		InsertCommand<DispatchRaysRenderCommand>(Width, Height, Depth);
	}

	FORCEINLINE void Reset()
	{
		if (First != nullptr)
		{
			RenderCommand* Cmd = First;
			while (Cmd != nullptr)
			{
				RenderCommand* Old = Cmd;
				Cmd = Cmd->NextCmd;
				Old->~RenderCommand();
			}
		}

		First = nullptr;
		Last = nullptr;

		CmdAllocator.Reset();
	}

private:
	template<typename TCommand, typename... TArgs>
	FORCEINLINE void InsertCommand(TArgs&&... Args)
	{
		VoidPtr Memory	= CmdAllocator.Allocate<TCommand>();
		TCommand* Cmd	= new(Memory) TCommand(Forward<TArgs>(Args)...);
		
		if (Last)
		{
			Last->NextCmd = Cmd;
			Last = Last->NextCmd;
		}
		else
		{
			First = Cmd;
			Last = First;
		}
	}

	StackAllocator CmdAllocator;
	RenderCommand* First;
	RenderCommand* Last;
};

/*
* CommandListExecutor
*/

class CommandListExecutor
{
public:
	CommandListExecutor();
	~CommandListExecutor();

	void ExecuteCommandList(CommandList& CmdList);

	FORCEINLINE void SetContext(ICommandContext* InCmdContext)
	{
		VALIDATE(InCmdContext != nullptr);
		CmdContext = InCmdContext;
	}

	FORCEINLINE ICommandContext& GetContext() const
	{
		VALIDATE(CmdContext != nullptr);
		return *CmdContext;
	}

private:
	ICommandContext* CmdContext;
};