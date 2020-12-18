#pragma once
#include "Resource.h"
#include "Texture.h"
#include "Buffer.h"
#include "RayTracing.h"
#include "RenderCommand.h"

#include "Memory/LinearAllocator.h"

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

	FORCEINLINE void Begin()
	{
		InsertCommand<BeginCommand>();
	}

	FORCEINLINE void End()
	{
		InsertCommand<EndCommand>();
	}

	FORCEINLINE void ClearRenderTargetView(
		RenderTargetView* RenderTargetView, 
		const ColorClearValue& ClearColor)
	{
		VALIDATE(RenderTargetView != nullptr);

		RenderTargetView->AddRef();
		InsertCommand<ClearRenderTargetViewCommand>(RenderTargetView, ClearColor);
	}

	FORCEINLINE void ClearDepthStencilView(
		DepthStencilView* DepthStencilView, 
		const DepthStencilClearValue& ClearValue)
	{
		VALIDATE(DepthStencilView != nullptr);

		DepthStencilView->AddRef();
		InsertCommand<ClearDepthStencilViewCommand>(DepthStencilView, ClearValue);
	}

	FORCEINLINE void ClearUnorderedAccessView(
		UnorderedAccessView* UnorderedAccessView,
		const ColorClearValue& ClearColor)
	{
		VALIDATE(UnorderedAccessView != nullptr);

		UnorderedAccessView->AddRef();
		InsertCommand<ClearUnorderedAccessViewCommand>(UnorderedAccessView, ClearColor);
	}

	FORCEINLINE void BeginRenderPass()
	{
		InsertCommand<BeginRenderPassCommand>();
	}

	FORCEINLINE void EndRenderPass()
	{
		InsertCommand<EndRenderPassCommand>();
	}

	FORCEINLINE void BindViewport(const Viewport& Viewport, Uint32 Slot)
	{
		InsertCommand<BindViewportCommand>(Viewport, Slot);
	}

	FORCEINLINE void BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot)
	{
		InsertCommand<BindScissorRectCommand>(ScissorRect, Slot);
	}

	FORCEINLINE void BindBlendFactor(const ColorClearValue& Color)
	{
		InsertCommand<BindBlendFactorCommand>(Color);
	}

	FORCEINLINE void BindPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
	{
		InsertCommand<BindPrimitiveTopologyCommand>(PrimitveTopologyType);
	}

	FORCEINLINE void BindVertexBuffers(
		VertexBuffer* const* VertexBuffers, 
		Uint32 VertexBufferCount, 
		Uint32 BufferSlot)
	{
		VoidPtr BufferMemory	= CmdAllocator.Allocate(sizeof(VertexBuffer*) * VertexBufferCount, 1);
		VertexBuffer** Buffers	= reinterpret_cast<VertexBuffer**>(BufferMemory);
		for (Uint32 i = 0; i < VertexBufferCount; i++)
		{
			Buffers[i] = VertexBuffers[i];
			SAFEADDREF(Buffers[i]);
		}

		InsertCommand<BindVertexBuffersCommand>(Buffers, VertexBufferCount, BufferSlot);
	}

	FORCEINLINE void BindIndexBuffer(IndexBuffer* IndexBuffer)
	{
		SAFEADDREF(IndexBuffer);
		InsertCommand<BindIndexBufferCommand>(IndexBuffer);
	}

	FORCEINLINE void BindRayTracingScene(RayTracingScene* RayTracingScene)
	{
		SAFEADDREF(RayTracingScene);
		InsertCommand<BindRayTracingSceneCommand>(RayTracingScene);
	}

	FORCEINLINE void BindRenderTargets(
		RenderTargetView* const* RenderTargetViews, 
		Uint32 RenderTargetCount, 
		DepthStencilView* DepthStencilView)
	{
		VoidPtr RenderTargetMemory			= CmdAllocator.Allocate(sizeof(RenderTargetView*) * RenderTargetCount, 1);
		RenderTargetView** RenderTargets	= reinterpret_cast<RenderTargetView**>(RenderTargetMemory);
		for (Uint32 i = 0; i < RenderTargetCount; i++)
		{
			RenderTargets[i] = RenderTargetViews[i];
			SAFEADDREF(RenderTargets[i]);
		}

		SAFEADDREF(DepthStencilView);
		InsertCommand<BindRenderTargetsCommand>(RenderTargets, RenderTargetCount, DepthStencilView);
	}

	FORCEINLINE void BindGraphicsPipelineState(GraphicsPipelineState* PipelineState)
	{
		SAFEADDREF(PipelineState);
		InsertCommand<BindGraphicsPipelineStateCommand>(PipelineState);
	}

	FORCEINLINE void BindComputePipelineState(ComputePipelineState* PipelineState)
	{
		SAFEADDREF(PipelineState);
		InsertCommand<BindComputePipelineStateCommand>(PipelineState);
	}

	FORCEINLINE void BindRayTracingPipelineState(RayTracingPipelineState* PipelineState)
	{
		SAFEADDREF(PipelineState);
		InsertCommand<BindRayTracingPipelineStateCommand>(PipelineState);
	}

	FORCEINLINE void BindConstantBuffers(
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

	FORCEINLINE void BindShaderResourceViews(
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

	FORCEINLINE void BindUnorderedAccessViews(
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

	FORCEINLINE void ResolveTexture(Texture* Destination, Texture* Source)
	{
		SAFEADDREF(Destination);
		SAFEADDREF(Source);
		InsertCommand<ResolveTextureCommand>(Destination, Source);
	}

	FORCEINLINE void UpdateBuffer(
		Buffer* Destination, 
		Uint64 DestinationOffsetInBytes, 
		Uint64 SizeInBytes, 
		const VoidPtr SourceData)
	{
		VoidPtr TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
		Memory::Memcpy(TempSourceData, SourceData, SizeInBytes);

		SAFEADDREF(Destination);
		InsertCommand<UpdateBufferCommand>(
			Destination, 
			DestinationOffsetInBytes, 
			SizeInBytes, 
			SourceData);
	}

	FORCEINLINE void UpdateTexture2D(
		Texture2D* Destination,
		Uint32 Width,
		Uint32 Height,
		Uint32 MipLevel, 
		const VoidPtr SourceData)
	{
		const Uint32 SizeInBytes = Width * Height;
		VoidPtr TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
		Memory::Memcpy(TempSourceData, SourceData, SizeInBytes);

		SAFEADDREF(Destination);
		InsertCommand<UpdateTextureCommand>(
			Destination,
			Width,
			Height,
			MipLevel,
			SourceData);
	}

	FORCEINLINE void CopyBuffer(
		Buffer* Destination, 
		Buffer* Source, 
		const CopyBufferInfo& CopyInfo)
	{
		SAFEADDREF(Destination);
		SAFEADDREF(Source);
		InsertCommand<CopyBufferCommand>(Destination, Source, CopyInfo);
	}

	FORCEINLINE void CopyTexture(
		Texture* Destination, 
		Texture* Source, 
		const CopyTextureInfo& CopyTextureInfo)
	{
		SAFEADDREF(Destination);
		SAFEADDREF(Source);
		InsertCommand<CopyTextureCommand>(Destination, Source, CopyTextureInfo);
	}

	FORCEINLINE void BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
	{
		SAFEADDREF(RayTracingGeometry);
		InsertCommand<BuildRayTracingGeometryCommand>(RayTracingGeometry);
	}

	FORCEINLINE void BuildRayTracingScene(RayTracingScene* RayTracingScene)
	{
		SAFEADDREF(RayTracingScene);
		InsertCommand<BuildRayTracingSceneCommand>(RayTracingScene);
	}

	FORCEINLINE void GenerateMips(Texture* Texture)
	{
		VALIDATE(Texture != nullptr);

		Texture->AddRef();
		InsertCommand<GenerateMipsCommand>(Texture);
	}

	FORCEINLINE void TransitionTexture(
		Texture* Texture, 
		EResourceState BeforeState, 
		EResourceState AfterState)
	{
		VALIDATE(Texture != nullptr);

		Texture->AddRef();
		InsertCommand<TransitionTextureCommand>(Texture, BeforeState, AfterState);
	}

	FORCEINLINE void TransitionBuffer(
		Buffer* Buffer,
		EResourceState BeforeState,
		EResourceState AfterState)
	{
		VALIDATE(Buffer != nullptr);

		Buffer->AddRef();
		InsertCommand<TransitionBufferCommand>(Buffer, BeforeState, AfterState);
	}

	FORCEINLINE void UnorderedAccessTextureBarrier(Texture* Texture)
	{
		VALIDATE(Texture != nullptr);

		Texture->AddRef();
		InsertCommand<UnorderedAccessTextureBarrierCommand>(Texture);
	}

	FORCEINLINE void Draw(
		Uint32 VertexCount, 
		Uint32 StartVertexLocation)
	{
		InsertCommand<DrawCommand>(VertexCount, StartVertexLocation);
	}

	FORCEINLINE void DrawIndexed(
		Uint32 IndexCount, 
		Uint32 StartIndexLocation, 
		Uint32 BaseVertexLocation)
	{
		InsertCommand< DrawIndexedCommand>(IndexCount, StartIndexLocation, BaseVertexLocation);
	}

	FORCEINLINE void DrawInstanced(
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

	FORCEINLINE void DrawIndexedInstanced(
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

	FORCEINLINE void Dispatch(Uint32 WorkGroupsX, Uint32 WorkGroupsY, Uint32 WorkGroupsZ)
	{
		InsertCommand<DispatchComputeCommand>(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
	}

	FORCEINLINE void DispatchRays(Uint32 Width, Uint32 Height, Uint32 Depth)
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

			First	= nullptr;
			Last	= nullptr;
		}

		CmdAllocator.Reset();
	}

private:
	template<typename TCommand, typename... TArgs>
	FORCEINLINE void InsertCommand(TArgs&&... Args)
	{
		VoidPtr Memory = CmdAllocator.Allocate<TCommand>();
		VALIDATE(Memory != nullptr);

		TCommand* Cmd = new(Memory) TCommand(Forward<TArgs>(Args)...);
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

private:
	LinearAllocator CmdAllocator;
	RenderCommand* First;
	RenderCommand* Last;
};

/*
* CommandListExecutor
*/

class CommandListExecutor
{
public:
	static void ExecuteCommandList(CommandList& CmdList);

	FORCEINLINE static void SetContext(ICommandContext* InCmdContext)
	{
		VALIDATE(InCmdContext != nullptr);
		CmdContext = InCmdContext;
	}

	FORCEINLINE static ICommandContext& GetContext()
	{
		VALIDATE(CmdContext != nullptr);
		return *CmdContext;
	}

private:
	static ICommandContext* CmdContext;
};