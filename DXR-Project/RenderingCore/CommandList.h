#pragma once
#include "Resource.h"
#include "RenderCommand.h"

/*
* CommandAllocator
*/

struct CommandMemoryArena
{
	inline CommandMemoryArena()
		: Mem(nullptr)
		, SizeInBytes(4096)
	{
		Mem = reinterpret_cast<Byte*>(Memory::Malloc(SizeInBytes));
	}

	inline ~CommandMemoryArena()
	{
		Memory::Free(Mem);
	}

	VoidPtr Allocate(Uint64 SizeInBytes);

	FORCEINLINE Uint64 ReservedSize()
	{
		return SizeInBytes - Offset;
	}

	FORCEINLINE void Reset()
	{
		Offset = 0;
	}

	Byte*	Mem;
	Uint64	Offset;
	Uint64	SizeInBytes;
};

/*
* CommandAllocator
*/

class CommandAllocator
{
public:
	CommandAllocator();
	~CommandAllocator() = default;

	template<typename TRenderCommand>
	VoidPtr Allocate()
	{
		return Allocate(sizeof(TRenderCommand));
	}

	VoidPtr Allocate(Uint64 SizeInBytes);

	void Reset();

private:
	Uint32 ArenaIndex;
	CommandMemoryArena* CurrentArena;
	TArray<CommandMemoryArena> Arenas;
};

class RenderTargetView;
class DepthStencilView;
class ShaderResourceView;
class UnorderedAccessView;
class Shader;
class RayTracingScene;
class RayTracingGeometry;

/*
* CommandList
*/

class CommandList
{
	friend class CommandExecutor;

public:
	CommandList();
	~CommandList();

	bool Initialize();

	FORCEINLINE void CommandList::Begin()
	{
		VoidPtr Memory = CmdAllocator.Allocate<BeginRenderCommand>();
		new(Memory) BeginRenderCommand();
	}

	FORCEINLINE void CommandList::End()
	{
		VoidPtr Memory = CmdAllocator.Allocate<EndRenderCommand>();
		new(Memory) EndRenderCommand();
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::ClearRenderTarget(RenderTargetView* RenderTargetView, const ColorClearValue& ClearColor)
	{
		VoidPtr Memory = CmdAllocator.Allocate<ClearRenderTargetRenderCommand>();
		new(Memory) ClearRenderTargetRenderCommand(RenderTargetView, ClearColor);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::ClearDepthStencil(DepthStencilView* DepthStencilView, const DepthStencilClearValue& ClearValue)
	{
		VoidPtr Memory = CmdAllocator.Allocate<ClearDepthStencilRenderCommand>();
		new(Memory) ClearDepthStencilRenderCommand(DepthStencilView, ClearValue);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BeginRenderPass()
	{
		VoidPtr Memory = CmdAllocator.Allocate<BeginRenderPassRenderCommand>();
		new(Memory) BeginRenderPassRenderCommand();
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::EndRenderPass()
	{
		VoidPtr Memory = CmdAllocator.Allocate<EndRenderPassRenderCommand>();
		new(Memory) EndRenderPassRenderCommand();
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindViewport(const Viewport& Viewport, Uint32 Slot)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindViewportRenderCommand>();
		new(Memory) BindViewportRenderCommand(Viewport, Slot);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindScissorRect(const ScissorRect& ScissorRect, Uint32 Slot)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindScissorRectRenderCommand>();
		new(Memory) BindScissorRectRenderCommand(ScissorRect, Slot);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindBlendFactor()
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindBlendFactorRenderCommand>();
		new(Memory) BindBlendFactorRenderCommand();
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindPrimitiveTopology(EPrimitveTopologyType PrimitveTopologyType)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindPrimitiveTopologyRenderCommand>();
		new(Memory) BindPrimitiveTopologyRenderCommand(PrimitveTopologyType);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindVertexBuffers(Buffer* const* VertexBuffers, Uint32 VertexBufferCount, Uint32 BufferSlot)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindVertexBuffersRenderCommand>();
		new(Memory) BindVertexBuffersRenderCommand(VertexBuffers, VertexBufferCount, BufferSlot);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindIndexBuffer(Buffer* IndexBuffer, EFormat IndexFormat)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindIndexBufferRenderCommand>();
		new(Memory) BindIndexBufferRenderCommand(IndexBuffer, IndexFormat);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindRayTracingScene(RayTracingScene* RayTracingScene)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindRayTracingSceneRenderCommand>();
		new(Memory) BindRayTracingSceneRenderCommand(RayTracingScene);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindRenderTargets(RenderTargetView* const* RenderTargetViews, Uint32 RenderTargetCount, DepthStencilView* DepthStencilView)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindRenderTargetsRenderCommand>();
		new(Memory) BindRenderTargetsRenderCommand(RenderTargetViews, RenderTargetCount, DepthStencilView);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindGraphicsPipelineState(GraphicsPipelineState* PipelineState)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindGraphicsPipelineStateRenderCommand>();
		new(Memory) BindGraphicsPipelineStateRenderCommand(PipelineState);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindComputePipelineState(ComputePipelineState* PipelineState)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindComputePipelineStateRenderCommand>();
		new(Memory) BindComputePipelineStateRenderCommand(PipelineState);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindRayTracingPipelineState(RayTracingPipelineState* PipelineState)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindRayTracingPipelineStateRenderCommand>();
		new(Memory) BindRayTracingPipelineStateRenderCommand(PipelineState);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindConstantBuffers(Shader* Shader, Buffer* const* ConstantBuffers, Uint32 ConstantBufferCount, Uint32 StartSlot)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindConstantBuffersRenderCommand>();
		new(Memory) BindConstantBuffersRenderCommand(Shader, ConstantBuffers, ConstantBufferCount, StartSlot);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindShaderResourceViews(Shader* Shader, ShaderResourceView* const* ShaderResourceViews, Uint32 ShaderResourceViewCount, Uint32 StartSlot)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindShaderResourceViewsRenderCommand>();
		new(Memory) BindShaderResourceViewsRenderCommand(Shader, ShaderResourceViews, ShaderResourceViewCount, StartSlot);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BindUnorderedAccessViews(Shader* Shader, UnorderedAccessView* const* UnorderedAccessViews, Uint32 UnorderedAccessViewCount, Uint32 StartSlot)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BindUnorderedAccessViewsRenderCommand>();
		new(Memory) BindUnorderedAccessViewsRenderCommand(Shader, UnorderedAccessViews, UnorderedAccessViewCount, StartSlot);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::ResolveTexture(Texture* Destination, Texture* Source)
	{
		VoidPtr Memory = CmdAllocator.Allocate<ResolveTextureRenderCommand>();
		new(Memory) ResolveTextureRenderCommand(Destination, Source);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::UpdateBuffer(Buffer* Destination, Uint64 DestinationOffsetInBytes, Uint64 SizeInBytes, const VoidPtr SourceData)
	{
		VoidPtr Memory = CmdAllocator.Allocate<UpdateBufferRenderCommand>();
		new(Memory) UpdateBufferRenderCommand(Destination, DestinationOffsetInBytes, SizeInBytes, SourceData);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::CopyBuffer(Buffer* Destination, Buffer* Source, const CopyBufferInfo& CopyInfo)
	{
		VoidPtr Memory = CmdAllocator.Allocate<CopyBufferRenderCommand>();
		new(Memory) CopyBufferRenderCommand(Destination, Source, CopyInfo);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::CopyTexture(Texture* Destination, Texture* Source, const CopyTextureInfo& CopyTextureInfo)
	{
		VoidPtr Memory = CmdAllocator.Allocate<CopyTextureRenderCommand>();
		new(Memory) CopyTextureRenderCommand(Destination, Source, CopyTextureInfo);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BuildRayTracingGeometryRenderCommand>();
		new(Memory) BuildRayTracingGeometryRenderCommand(RayTracingGeometry);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::BuildRayTracingScene(RayTracingScene* RayTracingScene)
	{
		VoidPtr Memory = CmdAllocator.Allocate<BuildRayTracingSceneRenderCommand>();
		new(Memory) BuildRayTracingSceneRenderCommand(RayTracingScene);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::Draw(Uint32 VertexCount, Uint32 StartVertexLocation)
	{
		VoidPtr Memory = CmdAllocator.Allocate<DrawRenderCommand>();
		new(Memory) DrawRenderCommand(VertexCount, StartVertexLocation);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::DrawIndexed(Uint32 IndexCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation)
	{
		VoidPtr Memory = CmdAllocator.Allocate<DrawIndexedRenderCommand>();
		new(Memory) DrawIndexedRenderCommand(IndexCount, StartIndexLocation, BaseVertexLocation);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::DrawInstanced(Uint32 VertexCountPerInstance, Uint32 InstanceCount, Uint32 StartVertexLocation, Uint32 StartInstanceLocation)
	{
		VoidPtr Memory = CmdAllocator.Allocate<DrawInstancedRenderCommand>();
		new(Memory) DrawInstancedRenderCommand(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::DrawIndexedInstanced(Uint32 IndexCountPerInstance, Uint32 InstanceCount, Uint32 StartIndexLocation, Uint32 BaseVertexLocation, Uint32 StartInstanceLocation)
	{
		VoidPtr Memory = CmdAllocator.Allocate<DrawIndexedInstancedRenderCommand>();
		new(Memory) DrawIndexedInstancedRenderCommand(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::Dispatch(Uint32 WorkGroupsX, Uint32 WorkGroupsY, Uint32 WorkGroupsZ)
	{
		VoidPtr Memory = CmdAllocator.Allocate<DispatchComputeRenderCommand>();
		new(Memory) DispatchComputeRenderCommand(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE void CommandList::DispatchRays(Uint32 Width, Uint32 Height, Uint32 Depth)
	{
		VoidPtr Memory = CmdAllocator.Allocate<DispatchRaysRenderCommand>();
		new(Memory) DispatchRaysRenderCommand(Width, Height, Depth);
		Commands.EmplaceBack(static_cast<RenderCommand*>(Memory));
	}

	FORCEINLINE CommandContext& GetContext() const
	{
		VALIDATE(CmdContext != nullptr);
		return *CmdContext;
	}

private:
	CommandAllocator		CmdAllocator;
	class CommandContext*	CmdContext;
	TArray<RenderCommand*>	Commands;
};

/*
* CommandExecutor
*/

class CommandExecutor
{
public:
	CommandExecutor()	= default;
	~CommandExecutor()	= default;

	void ExecuteCommandList(const CommandList& CmdList);
};