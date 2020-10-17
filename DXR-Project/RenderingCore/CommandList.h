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
		InsertCommand<BeginCommand>();
	}

	FORCEINLINE void CommandList::End()
	{
		InsertCommand<EndCommand>();
	}

	FORCEINLINE void CommandList::ClearRenderTargetView(
		RenderTargetView* RenderTargetView, 
		const ColorClearValue& ClearColor)
	{
		RenderTargetView->AddRef();
		InsertCommand<ClearRenderTargetCommand>(RenderTargetView, ClearColor);
	}

	FORCEINLINE void CommandList::ClearDepthStencilView(
		DepthStencilView* DepthStencilView, 
		const DepthStencilClearValue& ClearValue)
	{
		DepthStencilView->AddRef();
		InsertCommand<ClearDepthStencilCommand>(DepthStencilView, ClearValue);
	}

	FORCEINLINE void CommandList::BeginRenderPass()
	{
		InsertCommand<BeginRenderPassCommand>();
	}

	FORCEINLINE void CommandList::EndRenderPass()
	{
		InsertCommand<EndRenderPassCommand>();
	}

	FORCEINLINE void CommandList::BindViewport(const Viewport& Viewport, Uint32 Slot)
	{
		InsertCommand<BindViewportCommand>(Viewport, Slot);
	}

	FORCEINLINE void CommandList::BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot)
	{
		InsertCommand<BindScissorRectCommand>(ScissorRect, Slot);
	}

	FORCEINLINE void CommandList::BindBlendFactor()
	{
		InsertCommand<BindBlendFactorCommand>();
	}

	FORCEINLINE void CommandList::BindPrimitiveTopology(EPrimitveTopologyType PrimitveTopologyType)
	{
		InsertCommand<BindPrimitiveTopologyCommand>(PrimitveTopologyType);
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

		InsertCommand<BindVertexBuffersCommand>(Buffers, VertexBufferCount, BufferSlot);
	}

	FORCEINLINE void CommandList::BindIndexBuffer(IndexBuffer* IndexBuffer)
	{
		IndexBuffer->AddRef();
		InsertCommand<BindIndexBufferCommand>(IndexBuffer);
	}

	FORCEINLINE void CommandList::BindRayTracingScene(RayTracingScene* RayTracingScene)
	{
		RayTracingScene->AddRef();
		InsertCommand<BindRayTracingSceneCommand>(RayTracingScene);
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

		InsertCommand<BindRenderTargetsCommand>(RenderTargets, RenderTargetCount, DepthStencilView);
	}

	FORCEINLINE void CommandList::BindGraphicsPipelineState(GraphicsPipelineState* PipelineState)
	{
		PipelineState->AddRef();
		InsertCommand<BindGraphicsPipelineStateCommand>(PipelineState);
	}

	FORCEINLINE void CommandList::BindComputePipelineState(ComputePipelineState* PipelineState)
	{
		PipelineState->AddRef();
		InsertCommand<BindComputePipelineStateCommand>(PipelineState);
	}

	FORCEINLINE void CommandList::BindRayTracingPipelineState(RayTracingPipelineState* PipelineState)
	{
		PipelineState->AddRef();
		InsertCommand<BindRayTracingPipelineStateCommand>(PipelineState);
	}

	FORCEINLINE void CommandList::BindConstantBuffers(
		Shader* Shader, 
		ConstantBuffer* const* ConstantBuffers, 
		Uint32 ConstantBufferCount, 
		Uint32 StartSlot)
	{
		InsertCommand<BindConstantBuffersCommand>(
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
		InsertCommand<BindShaderResourceViewsCommand>(
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
		InsertCommand<BindUnorderedAccessViewsCommand>(
			Shader,
			UnorderedAccessViews,
			UnorderedAccessViewCount,
			StartSlot);
	}

	FORCEINLINE void CommandList::ResolveTexture(Texture* Destination, Texture* Source)
	{
		Destination->AddRef();
		Source->AddRef();
		InsertCommand<ResolveTextureCommand>(Destination, Source);
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
		InsertCommand<UpdateBufferCommand>(
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
		InsertCommand<CopyBufferCommand>(Destination, Source, CopyInfo);
	}

	FORCEINLINE void CommandList::CopyTexture(
		Texture* Destination, 
		Texture* Source, 
		const CopyTextureInfo& CopyTextureInfo)
	{
		Destination->AddRef();
		Source->AddRef();
		InsertCommand<CopyTextureCommand>(Destination, Source, CopyTextureInfo);
	}

	FORCEINLINE void CommandList::BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
	{
		RayTracingGeometry->AddRef();
		InsertCommand<BuildRayTracingGeometryCommand>(RayTracingGeometry);
	}

	FORCEINLINE void CommandList::BuildRayTracingScene(RayTracingScene* RayTracingScene)
	{
		RayTracingScene->AddRef();
		InsertCommand<BuildRayTracingSceneCommand>(RayTracingScene);
	}

	FORCEINLINE void CommandList::Draw(Uint32 VertexCount, Uint32 StartVertexLocation)
	{
		InsertCommand<DrawCommand>(VertexCount, StartVertexLocation);
	}

	FORCEINLINE void CommandList::DrawIndexed(Uint32 IndexCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation)
	{
		InsertCommand< DrawIndexedCommand>(IndexCount, StartIndexLocation, BaseVertexLocation);
	}

	FORCEINLINE void CommandList::DrawInstanced(
		Uint32 VertexCountPerInstance, 
		Uint32 InstanceCount, 
		Uint32 StartVertexLocation, 
		Uint32 StartInstanceLocation)
	{
		InsertCommand<DrawInstancedCommand>(
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
		InsertCommand<DrawIndexedInstancedCommand>(
			IndexCountPerInstance,
			InstanceCount,
			StartIndexLocation,
			BaseVertexLocation,
			StartInstanceLocation);
	}

	FORCEINLINE void CommandList::Dispatch(Uint32 WorkGroupsX, Uint32 WorkGroupsY, Uint32 WorkGroupsZ)
	{
		InsertCommand<DispatchComputeCommand>(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
	}

	FORCEINLINE void CommandList::DispatchRays(Uint32 Width, Uint32 Height, Uint32 Depth)
	{
		InsertCommand<DispatchRaysCommand>(Width, Height, Depth);
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