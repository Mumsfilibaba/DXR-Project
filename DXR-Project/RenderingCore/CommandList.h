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

	FORCEINLINE void BindViewport(const Viewport& Viewport, UInt32 Slot)
	{
		InsertCommand<BindViewportCommand>(Viewport, Slot);
	}

	FORCEINLINE void BindScissorRect(const ScissorRect& ScissorRect, UInt32 Slot)
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
		UInt32 VertexBufferCount, 
		UInt32 BufferSlot)
	{
		Void* BufferMemory	= CmdAllocator.Allocate(sizeof(VertexBuffer*) * VertexBufferCount, 1);
		VertexBuffer** Buffers	= reinterpret_cast<VertexBuffer**>(BufferMemory);
		for (UInt32 i = 0; i < VertexBufferCount; i++)
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
		UInt32 RenderTargetCount, 
		DepthStencilView* DepthStencilView)
	{
		Void* RenderTargetMemory			= CmdAllocator.Allocate(sizeof(RenderTargetView*) * RenderTargetCount, 1);
		RenderTargetView** RenderTargets	= reinterpret_cast<RenderTargetView**>(RenderTargetMemory);
		for (UInt32 i = 0; i < RenderTargetCount; i++)
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
		UInt32 ConstantBufferCount, 
		UInt32 StartSlot)
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
		UInt32 ShaderResourceViewCount, 
		UInt32 StartSlot)
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
		UInt32 UnorderedAccessViewCount, 
		UInt32 StartSlot)
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
		UInt64 DestinationOffsetInBytes, 
		UInt64 SizeInBytes, 
		const Void* SourceData)
	{
		Void* TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
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
		UInt32 Width,
		UInt32 Height,
		UInt32 MipLevel, 
		const Void* SourceData)
	{
		const UInt32 SizeInBytes = Width * Height;
		Void* TempSourceData = CmdAllocator.Allocate(SizeInBytes, 1);
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
		UInt32 VertexCount, 
		UInt32 StartVertexLocation)
	{
		InsertCommand<DrawCommand>(VertexCount, StartVertexLocation);
	}

	FORCEINLINE void DrawIndexed(
		UInt32 IndexCount, 
		UInt32 StartIndexLocation, 
		UInt32 BaseVertexLocation)
	{
		InsertCommand< DrawIndexedCommand>(IndexCount, StartIndexLocation, BaseVertexLocation);
	}

	FORCEINLINE void DrawInstanced(
		UInt32 VertexCountPerInstance, 
		UInt32 InstanceCount, 
		UInt32 StartVertexLocation, 
		UInt32 StartInstanceLocation)
	{
		InsertCommand<DrawInstancedCommand>(
			VertexCountPerInstance,
			InstanceCount,
			StartVertexLocation,
			StartInstanceLocation);
	}

	FORCEINLINE void DrawIndexedInstanced(
		UInt32 IndexCountPerInstance, 
		UInt32 InstanceCount, 
		UInt32 StartIndexLocation, 
		UInt32 BaseVertexLocation, 
		UInt32 StartInstanceLocation)
	{
		InsertCommand<DrawIndexedInstancedCommand>(
			IndexCountPerInstance,
			InstanceCount,
			StartIndexLocation,
			BaseVertexLocation,
			StartInstanceLocation);
	}

	FORCEINLINE void Dispatch(UInt32 WorkGroupsX, UInt32 WorkGroupsY, UInt32 WorkGroupsZ)
	{
		InsertCommand<DispatchComputeCommand>(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
	}

	FORCEINLINE void DispatchRays(UInt32 Width, UInt32 Height, UInt32 Depth)
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
		Void* Memory = CmdAllocator.Allocate<TCommand>();
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