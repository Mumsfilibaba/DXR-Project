#pragma once
#include "RenderingCore.h"
#include "CommandContext.h"
#include "RenderingAPI.h"

#include "Memory/Memory.h"

// Base rendercommand
struct RenderCommand
{
	virtual ~RenderCommand() = default;

	virtual void Execute(CommandContext& CmdContext) const
	{
	}

	inline void operator()(CommandContext& CmdContext) const
	{
		Execute(CmdContext);
	}
};

// Begin RenderCommand
struct BeginRenderCommand : public RenderCommand
{
	inline BeginRenderCommand()
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.Begin();
	}
};

// End RenderCommand
struct EndRenderCommand : public RenderCommand
{
	inline EndRenderCommand()
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.End();
	}
};

// Clear RenderTarget RenderCommand
struct ClearRenderTargetRenderCommand : public RenderCommand
{
	inline ClearRenderTargetRenderCommand(RenderTargetView* InRenderTargetView, const ColorClearValue& InClearColor)
		: RenderTargetView(InRenderTargetView)
		, ClearColor(InClearColor)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.ClearRenderTarget(RenderTargetView, ClearColor);
	}

	RenderTargetView* RenderTargetView;
	ColorClearValue ClearColor;
};

// Clear DepthStencil RenderCommand
struct ClearDepthStencilRenderCommand : public RenderCommand
{
	inline ClearDepthStencilRenderCommand(DepthStencilView* InDepthStencilView, const DepthStencilClearValue& InClearValue)
		: DepthStencilView(InDepthStencilView)
		, ClearValue(InClearValue)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.ClearDepthStencil(DepthStencilView, ClearValue);
	}

	DepthStencilView* DepthStencilView;
	DepthStencilClearValue ClearValue;
};

// Bind Viewport RenderCommand
struct BindViewportRenderCommand : public RenderCommand
{
	inline BindViewportRenderCommand(const Viewport& InViewport, Uint32 InSlot)
		: Viewport(InViewport)
		, Slot(InSlot)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindViewport(Viewport, Slot);
	}

	Viewport Viewport;
	Uint32 Slot;
};

// Bind ScissorRect RenderCommand
struct BindScissorRectRenderCommand : public RenderCommand
{
	inline BindScissorRectRenderCommand(const ScissorRect& InScissorRect, Uint32 InSlot)
		: ScissorRect(InScissorRect)
		, Slot(InSlot)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindScissorRect(ScissorRect, Slot);
	}

	ScissorRect ScissorRect;
	Uint32 Slot;
};

// Bind BlendFactor RenderCommand
struct BindBlendFactorRenderCommand : public RenderCommand
{
	inline BindBlendFactorRenderCommand()
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindBlendFactor();
	}
};

// BeginRenderPass RenderCommand
struct BeginRenderPassRenderCommand : public RenderCommand
{
	inline BeginRenderPassRenderCommand()
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BeginRenderPass();
	}
};

// End RenderCommand
struct EndRenderPassRenderCommand : public RenderCommand
{
	inline EndRenderPassRenderCommand()
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.EndRenderPass();
	}
};

// Bind PrimitiveTopology RenderCommand
struct BindPrimitiveTopologyRenderCommand : public RenderCommand
{
	inline BindPrimitiveTopologyRenderCommand(EPrimitveTopologyType InPrimitiveTopologyType)
		: PrimitiveTopologyType(InPrimitiveTopologyType)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindPrimitiveTopology(PrimitiveTopologyType);
	}

	EPrimitveTopologyType PrimitiveTopologyType;
};

// Bind VertexBuffers RenderCommand
struct BindVertexBuffersRenderCommand : public RenderCommand
{
	inline BindVertexBuffersRenderCommand(Buffer* const * InVertexBuffers, Uint32 InVertexBufferCount, Uint32 InSlot)
		: VertexBuffers(InVertexBuffers)
		, VertexBufferCount(InVertexBufferCount)
		, Slot(InSlot)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindVertexBuffers(VertexBuffers, VertexBufferCount, Slot);
	}

	Buffer* const* VertexBuffers;
	Uint32 VertexBufferCount;
	Uint32 Slot;
};

// Bind IndexBuffer RenderCommand
struct BindIndexBufferRenderCommand : public RenderCommand
{
	inline BindIndexBufferRenderCommand(Buffer* InIndexBuffer, EFormat InIndexFormat)
		: IndexBuffer(InIndexBuffer)
		, IndexFormat(InIndexFormat)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindIndexBuffer(IndexBuffer, IndexFormat);
	}

	Buffer* IndexBuffer;
	EFormat IndexFormat;
};

// Bind RayTracingScene RenderCommand
struct BindRayTracingSceneRenderCommand : public RenderCommand
{
	inline BindRayTracingSceneRenderCommand(RayTracingScene* InRayTracingScene)
		: RayTracingScene(InRayTracingScene)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindRayTracingScene(RayTracingScene);
	}

	RayTracingScene* RayTracingScene;
};

// Bind BlendFactor RenderCommand
struct BindRenderTargetsRenderCommand : public RenderCommand
{
	inline BindRenderTargetsRenderCommand(RenderTargetView* const * InRenderTargetViews, Uint32 InRenderTargetViewCount, DepthStencilView* InDepthStencilView)
		: RenderTargetViews(InRenderTargetViews)
		, RenderTargetViewCount(InRenderTargetViewCount)
		, DepthStencilView(InDepthStencilView)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindRenderTargets(RenderTargetViews, RenderTargetViewCount, DepthStencilView);
	}

	RenderTargetView* const* RenderTargetViews;
	Uint32 RenderTargetViewCount;
	DepthStencilView* DepthStencilView;
};

// Bind GraphicsPipelineState RenderCommand
struct BindGraphicsPipelineStateRenderCommand : public RenderCommand
{
	inline BindGraphicsPipelineStateRenderCommand(GraphicsPipelineState* InPipelineState)
		: PipelineState(InPipelineState)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindGraphicsPipelineState(PipelineState);
	}

	GraphicsPipelineState* PipelineState;
};

// Bind ComputePipelineState RenderCommand
struct BindComputePipelineStateRenderCommand : public RenderCommand
{
	inline BindComputePipelineStateRenderCommand(ComputePipelineState* InPipelineState)
		: PipelineState(InPipelineState)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindComputePipelineState(PipelineState);
	}

	ComputePipelineState* PipelineState;
};

// Bind RayTracingPipelineState RenderCommand
struct BindRayTracingPipelineStateRenderCommand : public RenderCommand
{
	inline BindRayTracingPipelineStateRenderCommand(RayTracingPipelineState* InPipelineState)
		: PipelineState(InPipelineState)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindRayTracingPipelineState(PipelineState);
	}

	RayTracingPipelineState* PipelineState;
};

// Bind ConstantBuffers RenderCommand
struct BindConstantBuffersRenderCommand : public RenderCommand
{
	inline BindConstantBuffersRenderCommand(Shader* InShader, Buffer* const* InConstantBuffers, Uint32 InConstantBufferCount, Uint32 InStartSlot)
		: Shader(InShader)
		, ConstantBuffers(InConstantBuffers)
		, ConstantBufferCount(InConstantBufferCount)
		, StartSlot(InStartSlot)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindConstantBuffers(Shader, ConstantBuffers, ConstantBufferCount, StartSlot);
	}

	Shader* Shader;
	Buffer* const* ConstantBuffers;
	Uint32 ConstantBufferCount;
	Uint32 StartSlot;
};

// Bind ShaderResourceViews RenderCommand
struct BindShaderResourceViewsRenderCommand : public RenderCommand
{
	inline BindShaderResourceViewsRenderCommand(Shader* InShader, ShaderResourceView* const* InShaderResourceViews, Uint32 InShaderResourceViewCount, Uint32 InStartSlot)
		: Shader(InShader)
		, ShaderResourceViews(InShaderResourceViews)
		, ShaderResourceViewCount(InShaderResourceViewCount)
		, StartSlot(InStartSlot)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindShaderResourceViews(Shader, ShaderResourceViews, ShaderResourceViewCount, StartSlot);
	}

	Shader* Shader;
	ShaderResourceView* const* ShaderResourceViews;
	Uint32 ShaderResourceViewCount;
	Uint32 StartSlot;
};

// Bind UnorderedAccessViews RenderCommand
struct BindUnorderedAccessViewsRenderCommand : public RenderCommand
{
	inline BindUnorderedAccessViewsRenderCommand(Shader* InShader, UnorderedAccessView* const* InUnorderedAccessViews, Uint32 InUnorderedAccessViewCount, Uint32 InStartSlot)
		: Shader(InShader)
		, UnorderedAccessViews(InUnorderedAccessViews)
		, UnorderedAccessViewCount(InUnorderedAccessViewCount)
		, StartSlot(InStartSlot)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BindUnorderedAccessViews(Shader, UnorderedAccessViews, UnorderedAccessViewCount, StartSlot);
	}

	Shader* Shader;
	UnorderedAccessView* const* UnorderedAccessViews;
	Uint32 UnorderedAccessViewCount;
	Uint32 StartSlot;
};

// Resolve Texture RenderCommand
struct ResolveTextureRenderCommand : public RenderCommand
{
	inline ResolveTextureRenderCommand(Texture* InDestination, Texture* InSource)
		: Destination(InDestination)
		, Source(InSource)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.ResolveTexture(Destination, Source);
	}

	Texture* Destination;
	Texture* Source;
};

// Resolve Texture RenderCommand
struct UpdateBufferRenderCommand : public RenderCommand
{
	inline UpdateBufferRenderCommand(Buffer* InDestination, Uint64 InDestinationOffsetInBytes, Uint64 InSizeInBytes, const VoidPtr InSourceData)
		: Destination(InDestination)
		, DestinationOffsetInBytes(InDestinationOffsetInBytes)
		, SizeInBytes(InSizeInBytes)
		, SourceData(nullptr)
	{
		if (SizeInBytes > 0)
		{
			SourceData = Memory::Malloc(SizeInBytes);
			Memory::Memcpy(SourceData, InSourceData, SizeInBytes);
		}
	}

	inline ~UpdateBufferRenderCommand()
	{
		Memory::Free(SourceData);
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.UpdateBuffer(Destination, DestinationOffsetInBytes, SizeInBytes, SourceData);
	}

	Buffer*	Destination;
	Uint64 	DestinationOffsetInBytes; 
	Uint64 	SizeInBytes;
	VoidPtr SourceData;
};

// Copy Buffer RenderCommand
struct CopyBufferRenderCommand : public RenderCommand
{
	inline CopyBufferRenderCommand(Buffer* InDestination, Buffer* InSource, const CopyBufferInfo& InCopyBufferInfo)
		: Destination(InDestination)
		, Source(InSource)
		, CopyBufferInfo(InCopyBufferInfo)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.CopyBuffer(Destination, Source, CopyBufferInfo);
	}

	Buffer* Destination;
	Buffer* Source;
	CopyBufferInfo CopyBufferInfo;
};

// Copy Texture RenderCommand
struct CopyTextureRenderCommand : public RenderCommand
{
	inline CopyTextureRenderCommand(Texture* InDestination, Texture* InSource, const CopyTextureInfo& InCopyTextureInfo)
		: Destination(InDestination)
		, Source(InSource)
		, CopyTextureInfo(InCopyTextureInfo)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.CopyTexture(Destination, Source, CopyTextureInfo);
	}

	Texture* Destination;
	Texture* Source;
	CopyTextureInfo CopyTextureInfo;
};

// Build RayTracing Geoemtry RenderCommand
struct BuildRayTracingGeometryRenderCommand : public RenderCommand
{
	inline BuildRayTracingGeometryRenderCommand(RayTracingGeometry* InRayTracingGeometry)
		: RayTracingGeometry(InRayTracingGeometry)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BuildRayTracingGeometry(RayTracingGeometry);
	}

	RayTracingGeometry* RayTracingGeometry;
};

// Build RayTracing Scene RenderCommand
struct BuildRayTracingSceneRenderCommand : public RenderCommand
{
	inline BuildRayTracingSceneRenderCommand(RayTracingScene* InRayTracingScene)
		: RayTracingScene(InRayTracingScene)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.BuildRayTracingScene(RayTracingScene);
	}

	RayTracingScene* RayTracingScene;
};

// Draw RenderCommand
struct DrawRenderCommand : public RenderCommand
{
	inline DrawRenderCommand(Uint32 InVertexCount, Uint32 InStartVertexLocation)
		: VertexCount(InVertexCount)
		, StartVertexLocation(InStartVertexLocation)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.Draw(VertexCount, StartVertexLocation);
	}

	Uint32 VertexCount;
	Uint32 StartVertexLocation;
};

// DrawIndexed RenderCommand
struct DrawIndexedRenderCommand : public RenderCommand
{
	inline DrawIndexedRenderCommand(Uint32 InIndexCount, Uint32 InStartIndexLocation, Uint32 InBaseVertexLocation)
		: IndexCount(InIndexCount)
		, StartIndexLocation(InStartIndexLocation)
		, BaseVertexLocation(InBaseVertexLocation)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
	}

	Uint32	IndexCount;
	Uint32	StartIndexLocation;
	Int32	BaseVertexLocation;
};

// DrawInstanced RenderCommand
struct DrawInstancedRenderCommand : public RenderCommand
{
	inline DrawInstancedRenderCommand(Uint32 InVertexCountPerInstance, Uint32 InInstanceCount, Uint32 InStartVertexLocation, Uint32 InStartInstanceLocation)
		: VertexCountPerInstance(InVertexCountPerInstance)
		, InstanceCount(InInstanceCount)
		, StartVertexLocation(InStartVertexLocation)
		, StartInstanceLocation(InStartInstanceLocation)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	Uint32 VertexCountPerInstance;
	Uint32 InstanceCount;
	Uint32 StartVertexLocation;
	Uint32 StartInstanceLocation;
};

// DrawIndexedInstanced RenderCommand
struct DrawIndexedInstancedRenderCommand : public RenderCommand
{
	inline DrawIndexedInstancedRenderCommand(Uint32 InIndexCountPerInstance, Uint32 InInstanceCount, Uint32 InStartIndexLocation, Uint32 InBaseVertexLocation, Uint32 InStartInstanceLocation)
		: IndexCountPerInstance(InIndexCountPerInstance)
		, InstanceCount(InInstanceCount)
		, StartIndexLocation(InStartIndexLocation)
		, BaseVertexLocation(InBaseVertexLocation)
		, StartInstanceLocation(InStartInstanceLocation)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}

	Uint32	IndexCountPerInstance;
	Uint32	InstanceCount;
	Uint32	StartIndexLocation;
	Int32	BaseVertexLocation;
	Uint32	StartInstanceLocation;
};

// Dispatch Compute RenderCommand
struct DispatchComputeRenderCommand : public RenderCommand
{
	inline DispatchComputeRenderCommand(Uint32 InWorkGroupsX, Uint32 InWorkGroupsY, Uint32 InWorkGroupsZ)
		: WorkGroupsX(InWorkGroupsX)
		, WorkGroupsY(InWorkGroupsY)
		, WorkGroupsZ(InWorkGroupsZ)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
	}

	Uint32 WorkGroupsX;
	Uint32 WorkGroupsY;
	Uint32 WorkGroupsZ;
};

// Dispatch Rays RenderCommand
struct DispatchRaysRenderCommand : public RenderCommand
{
	inline DispatchRaysRenderCommand(Uint32 InWidth, Uint32 InHeigh, Uint32 InDepth)
		: Width(InWidth)
		, Height(InHeigh)
		, Depth(InDepth)
	{
	}

	virtual void Execute(CommandContext& CmdContext) const override
	{
		CmdContext.Dispatch(Width, Height, Depth);
	}

	Uint32 Width;
	Uint32 Height;
	Uint32 Depth;
};