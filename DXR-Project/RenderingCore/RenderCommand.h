#pragma once
#include "RenderingCore.h"
#include "ICommandContext.h"
#include "PipelineState.h"

#include "Memory/Memory.h"

// Base rendercommand
struct RenderCommand
{
	virtual ~RenderCommand() = default;

	virtual void Execute(ICommandContext&) const
	{
	}

	inline void operator()(ICommandContext& CmdContext) const
	{
		Execute(CmdContext);
	}

	RenderCommand* NextCmd = nullptr;
};

// Begin RenderCommand
struct BeginCommand : public RenderCommand
{
	inline BeginCommand()
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.Begin();
	}
};

// End RenderCommand
struct EndCommand : public RenderCommand
{
	inline EndCommand()
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.End();
	}
};

// Clear RenderTarget RenderCommand
struct ClearRenderTargetCommand : public RenderCommand
{
	inline ClearRenderTargetCommand(RenderTargetView* InRenderTargetView, const ColorClearValue& InClearColor)
		: RenderTargetView(InRenderTargetView)
		, ClearColor(InClearColor)
	{
		VALIDATE(RenderTargetView != nullptr);
	}

	inline ~ClearRenderTargetCommand()
	{
		SAFERELEASE(RenderTargetView);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.ClearRenderTarget(RenderTargetView, ClearColor);
	}

	RenderTargetView* RenderTargetView;
	ColorClearValue ClearColor;
};

// Clear DepthStencil RenderCommand
struct ClearDepthStencilCommand : public RenderCommand
{
	inline ClearDepthStencilCommand(DepthStencilView* InDepthStencilView, const DepthStencilClearValue& InClearValue)
		: DepthStencilView(InDepthStencilView)
		, ClearValue(InClearValue)
	{
		VALIDATE(DepthStencilView != nullptr);
	}

	inline ~ClearDepthStencilCommand()
	{
		SAFERELEASE(DepthStencilView);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.ClearDepthStencil(DepthStencilView, ClearValue);
	}

	DepthStencilView* DepthStencilView;
	DepthStencilClearValue ClearValue;
};

// Bind Viewport RenderCommand
struct BindViewportCommand : public RenderCommand
{
	inline BindViewportCommand(const Viewport& InViewport, Uint32 InSlot)
		: Viewport(InViewport)
		, Slot(InSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindViewport(Viewport, Slot);
	}

	Viewport Viewport;
	Uint32 Slot;
};

// Bind ScissorRect RenderCommand
struct BindScissorRectCommand : public RenderCommand
{
	inline BindScissorRectCommand(const ScissorRect& InScissorRect, Uint32 InSlot)
		: ScissorRect(InScissorRect)
		, Slot(InSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindScissorRect(ScissorRect, Slot);
	}

	ScissorRect ScissorRect;
	Uint32 Slot;
};

// Bind BlendFactor RenderCommand
struct BindBlendFactorCommand : public RenderCommand
{
	inline BindBlendFactorCommand(const ColorClearValue& InColor)
		: Color(InColor)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindBlendFactor(Color);
	}

	ColorClearValue Color;
};

// BeginRenderPass RenderCommand
struct BeginRenderPassCommand : public RenderCommand
{
	inline BeginRenderPassCommand()
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BeginRenderPass();
	}
};

// End RenderCommand
struct EndRenderPassCommand : public RenderCommand
{
	inline EndRenderPassCommand()
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.EndRenderPass();
	}
};

// Bind PrimitiveTopology RenderCommand
struct BindPrimitiveTopologyCommand : public RenderCommand
{
	inline BindPrimitiveTopologyCommand(EPrimitiveTopology InPrimitiveTopologyType)
		: PrimitiveTopologyType(InPrimitiveTopologyType)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindPrimitiveTopology(PrimitiveTopologyType);
	}

	EPrimitiveTopology PrimitiveTopologyType;
};

// Bind VertexBuffers RenderCommand
struct BindVertexBuffersCommand : public RenderCommand
{
	inline BindVertexBuffersCommand(VertexBuffer* const * InVertexBuffers, Uint32 InVertexBufferCount, Uint32 InSlot)
		: VertexBuffers(InVertexBuffers)
		, VertexBufferCount(InVertexBufferCount)
		, Slot(InSlot)
	{
		VALIDATE(VertexBuffers != nullptr);
		for (Uint32 i = 0; i < VertexBufferCount; i++)
		{
			VALIDATE(VertexBuffers[i] != nullptr);
		}
	}

	inline ~BindVertexBuffersCommand()
	{
		for (Uint32 i = 0; i < VertexBufferCount; i++)
		{
			VertexBuffers[i]->Release();
		}
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindVertexBuffers(VertexBuffers, VertexBufferCount, Slot);
	}

	VertexBuffer* const* VertexBuffers;
	Uint32 VertexBufferCount;
	Uint32 Slot;
};

// Bind IndexBuffer RenderCommand
struct BindIndexBufferCommand : public RenderCommand
{
	inline BindIndexBufferCommand(IndexBuffer* InIndexBuffer)
		: IndexBuffer(InIndexBuffer)
	{
		VALIDATE(IndexBuffer != nullptr);
	}

	inline ~BindIndexBufferCommand()
	{
		SAFERELEASE(IndexBuffer);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindIndexBuffer(IndexBuffer);
	}

	IndexBuffer* IndexBuffer;
};

// Bind RayTracingScene RenderCommand
struct BindRayTracingSceneCommand : public RenderCommand
{
	inline BindRayTracingSceneCommand(RayTracingScene* InRayTracingScene)
		: RayTracingScene(InRayTracingScene)
	{
		VALIDATE(RayTracingScene != nullptr);
	}

	inline ~BindRayTracingSceneCommand()
	{
		SAFERELEASE(RayTracingScene);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindRayTracingScene(RayTracingScene);
	}

	RayTracingScene* RayTracingScene;
};

// Bind BlendFactor RenderCommand
struct BindRenderTargetsCommand : public RenderCommand
{
	inline BindRenderTargetsCommand(RenderTargetView* const * InRenderTargetViews, Uint32 InRenderTargetViewCount, DepthStencilView* InDepthStencilView)
		: RenderTargetViews(InRenderTargetViews)
		, RenderTargetViewCount(InRenderTargetViewCount)
		, DepthStencilView(InDepthStencilView)
	{
		VALIDATE(DepthStencilView != nullptr);
		VALIDATE(RenderTargetViews != nullptr);
		for (Uint32 i = 0; i < RenderTargetViewCount; i++)
		{
			VALIDATE(RenderTargetViews[i] != nullptr);
		}
	}

	inline ~BindRenderTargetsCommand()
	{
		SAFERELEASE(DepthStencilView);
		for (Uint32 i = 0; i < RenderTargetViewCount; i++)
		{
			RenderTargetViews[i]->Release();
		}
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindRenderTargets(RenderTargetViews, RenderTargetViewCount, DepthStencilView);
	}

	RenderTargetView* const* RenderTargetViews;
	Uint32 RenderTargetViewCount;
	DepthStencilView* DepthStencilView;
};

// Bind GraphicsPipelineState RenderCommand
struct BindGraphicsPipelineStateCommand : public RenderCommand
{
	inline BindGraphicsPipelineStateCommand(GraphicsPipelineState* InPipelineState)
		: PipelineState(InPipelineState)
	{
		VALIDATE(PipelineState != nullptr);
	}

	inline ~BindGraphicsPipelineStateCommand()
	{
		SAFERELEASE(PipelineState);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindGraphicsPipelineState(PipelineState);
	}

	GraphicsPipelineState* PipelineState;
};

// Bind ComputePipelineState RenderCommand
struct BindComputePipelineStateCommand : public RenderCommand
{
	inline BindComputePipelineStateCommand(ComputePipelineState* InPipelineState)
		: PipelineState(InPipelineState)
	{
		VALIDATE(PipelineState != nullptr);
	}

	inline ~BindComputePipelineStateCommand()
	{
		SAFERELEASE(PipelineState);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindComputePipelineState(PipelineState);
	}

	ComputePipelineState* PipelineState;
};

// Bind RayTracingPipelineState RenderCommand
struct BindRayTracingPipelineStateCommand : public RenderCommand
{
	inline BindRayTracingPipelineStateCommand(RayTracingPipelineState* InPipelineState)
		: PipelineState(InPipelineState)
	{
		VALIDATE(PipelineState != nullptr);
	}

	inline ~BindRayTracingPipelineStateCommand()
	{
		SAFERELEASE(PipelineState);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindRayTracingPipelineState(PipelineState);
	}

	RayTracingPipelineState* PipelineState;
};

// Bind ConstantBuffers RenderCommand
struct BindConstantBuffersCommand : public RenderCommand
{
	inline BindConstantBuffersCommand(Shader* InShader, ConstantBuffer* const* InConstantBuffers, Uint32 InConstantBufferCount, Uint32 InStartSlot)
		: Shader(InShader)
		, ConstantBuffers(InConstantBuffers)
		, ConstantBufferCount(InConstantBufferCount)
		, StartSlot(InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindConstantBuffers(Shader, ConstantBuffers, ConstantBufferCount, StartSlot);
	}

	Shader* Shader;
	ConstantBuffer* const* ConstantBuffers;
	Uint32 ConstantBufferCount;
	Uint32 StartSlot;
};

// Bind ShaderResourceViews RenderCommand
struct BindShaderResourceViewsCommand : public RenderCommand
{
	inline BindShaderResourceViewsCommand(Shader* InShader, ShaderResourceView* const* InShaderResourceViews, Uint32 InShaderResourceViewCount, Uint32 InStartSlot)
		: Shader(InShader)
		, ShaderResourceViews(InShaderResourceViews)
		, ShaderResourceViewCount(InShaderResourceViewCount)
		, StartSlot(InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindShaderResourceViews(Shader, ShaderResourceViews, ShaderResourceViewCount, StartSlot);
	}

	Shader* Shader;
	ShaderResourceView* const* ShaderResourceViews;
	Uint32 ShaderResourceViewCount;
	Uint32 StartSlot;
};

// Bind UnorderedAccessViews RenderCommand
struct BindUnorderedAccessViewsCommand : public RenderCommand
{
	inline BindUnorderedAccessViewsCommand(Shader* InShader, UnorderedAccessView* const* InUnorderedAccessViews, Uint32 InUnorderedAccessViewCount, Uint32 InStartSlot)
		: Shader(InShader)
		, UnorderedAccessViews(InUnorderedAccessViews)
		, UnorderedAccessViewCount(InUnorderedAccessViewCount)
		, StartSlot(InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindUnorderedAccessViews(Shader, UnorderedAccessViews, UnorderedAccessViewCount, StartSlot);
	}

	Shader* Shader;
	UnorderedAccessView* const* UnorderedAccessViews;
	Uint32 UnorderedAccessViewCount;
	Uint32 StartSlot;
};

// Resolve Texture RenderCommand
struct ResolveTextureCommand : public RenderCommand
{
	inline ResolveTextureCommand(Texture* InDestination, Texture* InSource)
		: Destination(InDestination)
		, Source(InSource)
	{
		VALIDATE(Destination != nullptr);
		VALIDATE(Source != nullptr);
	}

	inline ~ResolveTextureCommand()
	{
		SAFERELEASE(Destination);
		SAFERELEASE(Source);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.ResolveTexture(Destination, Source);
	}

	Texture* Destination;
	Texture* Source;
};

// Update Buffer RenderCommand
struct UpdateBufferCommand : public RenderCommand
{
	inline UpdateBufferCommand(Buffer* InDestination, Uint64 InDestinationOffsetInBytes, Uint64 InSizeInBytes, const VoidPtr InSourceData)
		: Destination(InDestination)
		, DestinationOffsetInBytes(InDestinationOffsetInBytes)
		, SizeInBytes(InSizeInBytes)
		, SourceData(nullptr)
	{
		VALIDATE(InDestination != nullptr);
		VALIDATE(InSourceData != nullptr);
	}

	inline ~UpdateBufferCommand()
	{
		SAFERELEASE(Destination);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.UpdateBuffer(Destination, DestinationOffsetInBytes, SizeInBytes, SourceData);
	}

	Buffer* Destination;
	Uint64 	DestinationOffsetInBytes;
	Uint64 	SizeInBytes;
	VoidPtr SourceData;
};

// Update Texture RenderCommand
struct UpdateTextureCommand : public RenderCommand
{
	inline UpdateTextureCommand(Texture2D* InDestination, Uint32 InWidth, Uint32 InHeight, Uint32	InMipLevel, const VoidPtr InSourceData)
		: Destination(InDestination)
		, Width(InWidth)
		, Height(InHeight)
		, MipLevel(InMipLevel)
		, SourceData(InSourceData)
	{
		VALIDATE(InDestination != nullptr);
		VALIDATE(InSourceData != nullptr);
	}

	inline ~UpdateTextureCommand()
	{
		SAFERELEASE(Destination);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.UpdateTexture2D(Destination, Width, Height, MipLevel, SourceData);
	}

	Texture2D* Destination;
	Uint32	Width;
	Uint32	Height;
	Uint32	MipLevel;
	VoidPtr	SourceData;
};

// Copy Buffer RenderCommand
struct CopyBufferCommand : public RenderCommand
{
	inline CopyBufferCommand(Buffer* InDestination, Buffer* InSource, const CopyBufferInfo& InCopyBufferInfo)
		: Destination(InDestination)
		, Source(InSource)
		, CopyBufferInfo(InCopyBufferInfo)
	{
		VALIDATE(Destination != nullptr);
		VALIDATE(Source != nullptr);
	}

	inline ~CopyBufferCommand()
	{
		SAFERELEASE(Destination);
		SAFERELEASE(Source);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.CopyBuffer(Destination, Source, CopyBufferInfo);
	}

	Buffer* Destination;
	Buffer* Source;
	CopyBufferInfo CopyBufferInfo;
};

// Copy Texture RenderCommand
struct CopyTextureCommand : public RenderCommand
{
	inline CopyTextureCommand(Texture* InDestination, Texture* InSource, const CopyTextureInfo& InCopyTextureInfo)
		: Destination(InDestination)
		, Source(InSource)
		, CopyTextureInfo(InCopyTextureInfo)
	{
		VALIDATE(Destination != nullptr);
		VALIDATE(Source != nullptr);
	}

	inline ~CopyTextureCommand()
	{
		SAFERELEASE(Destination);
		SAFERELEASE(Source);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.CopyTexture(Destination, Source, CopyTextureInfo);
	}

	Texture* Destination;
	Texture* Source;
	CopyTextureInfo CopyTextureInfo;
};

// Build RayTracing Geoemtry RenderCommand
struct BuildRayTracingGeometryCommand : public RenderCommand
{
	inline BuildRayTracingGeometryCommand(RayTracingGeometry* InRayTracingGeometry)
		: RayTracingGeometry(InRayTracingGeometry)
	{
		VALIDATE(RayTracingGeometry != nullptr);
	}

	inline ~BuildRayTracingGeometryCommand()
	{
		SAFERELEASE(RayTracingGeometry);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BuildRayTracingGeometry(RayTracingGeometry);
	}

	RayTracingGeometry* RayTracingGeometry;
};

// Build RayTracing Scene RenderCommand
struct BuildRayTracingSceneCommand : public RenderCommand
{
	inline BuildRayTracingSceneCommand(RayTracingScene* InRayTracingScene)
		: RayTracingScene(InRayTracingScene)
	{
		VALIDATE(RayTracingScene != nullptr);
	}

	inline ~BuildRayTracingSceneCommand()
	{
		SAFERELEASE(RayTracingScene);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BuildRayTracingScene(RayTracingScene);
	}

	RayTracingScene* RayTracingScene;
};

// GenerateMips RenderCommand
struct GenerateMipsCommand : public RenderCommand
{
	inline GenerateMipsCommand(Texture* InTexture)
		: Texture(InTexture)
	{
	}

	inline ~GenerateMipsCommand()
	{
		SAFERELEASE(Texture);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.GenerateMips(Texture);
	}

	Texture* Texture;
};

// TransitionTexture RenderCommand
struct TransitionTextureCommand : public RenderCommand
{
	inline TransitionTextureCommand(Texture* InTexture, EResourceState InBeforeState, EResourceState InAfterState)
		: Texture(InTexture)
		, BeforeState(InBeforeState)
		, AfterState(InAfterState)
	{
	}

	inline ~TransitionTextureCommand()
	{
		SAFERELEASE(Texture);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.TransitionTexture(Texture, BeforeState, AfterState);
	}

	Texture* Texture;
	EResourceState BeforeState;
	EResourceState AfterState;
};

// TransitionBuffer RenderCommand
struct TransitionBufferCommand : public RenderCommand
{
	inline TransitionBufferCommand(Buffer* InBuffer, EResourceState InBeforeState, EResourceState InAfterState)
		: Buffer(InBuffer)
		, BeforeState(InBeforeState)
		, AfterState(InAfterState)
	{
	}

	inline ~TransitionBufferCommand()
	{
		SAFERELEASE(Buffer);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.TransitionBuffer(Buffer, BeforeState, AfterState);
	}

	Buffer* Buffer;
	EResourceState BeforeState;
	EResourceState AfterState;
};

// UnorderedAccessTextureBarrier RenderCommand
struct UnorderedAccessTextureBarrierCommand : public RenderCommand
{
	inline UnorderedAccessTextureBarrierCommand(Texture* InTexture)
		: Texture(InTexture)
	{
	}

	inline ~UnorderedAccessTextureBarrierCommand()
	{
		SAFERELEASE(Texture);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.UnorderedAccessTextureBarrier(Texture);
	}

	Texture* Texture;
};

// Draw RenderCommand
struct DrawCommand : public RenderCommand
{
	inline DrawCommand(Uint32 InVertexCount, Uint32 InStartVertexLocation)
		: VertexCount(InVertexCount)
		, StartVertexLocation(InStartVertexLocation)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.Draw(VertexCount, StartVertexLocation);
	}

	Uint32 VertexCount;
	Uint32 StartVertexLocation;
};

// DrawIndexed RenderCommand
struct DrawIndexedCommand : public RenderCommand
{
	inline DrawIndexedCommand(Uint32 InIndexCount, Uint32 InStartIndexLocation, Uint32 InBaseVertexLocation)
		: IndexCount(InIndexCount)
		, StartIndexLocation(InStartIndexLocation)
		, BaseVertexLocation(InBaseVertexLocation)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
	}

	Uint32	IndexCount;
	Uint32	StartIndexLocation;
	Int32	BaseVertexLocation;
};

// DrawInstanced RenderCommand
struct DrawInstancedCommand : public RenderCommand
{
	inline DrawInstancedCommand(Uint32 InVertexCountPerInstance, Uint32 InInstanceCount, Uint32 InStartVertexLocation, Uint32 InStartInstanceLocation)
		: VertexCountPerInstance(InVertexCountPerInstance)
		, InstanceCount(InInstanceCount)
		, StartVertexLocation(InStartVertexLocation)
		, StartInstanceLocation(InStartInstanceLocation)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	Uint32 VertexCountPerInstance;
	Uint32 InstanceCount;
	Uint32 StartVertexLocation;
	Uint32 StartInstanceLocation;
};

// DrawIndexedInstanced RenderCommand
struct DrawIndexedInstancedCommand : public RenderCommand
{
	inline DrawIndexedInstancedCommand(Uint32 InIndexCountPerInstance, Uint32 InInstanceCount, Uint32 InStartIndexLocation, Uint32 InBaseVertexLocation, Uint32 InStartInstanceLocation)
		: IndexCountPerInstance(InIndexCountPerInstance)
		, InstanceCount(InInstanceCount)
		, StartIndexLocation(InStartIndexLocation)
		, BaseVertexLocation(InBaseVertexLocation)
		, StartInstanceLocation(InStartInstanceLocation)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
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
struct DispatchComputeCommand : public RenderCommand
{
	inline DispatchComputeCommand(Uint32 InWorkGroupsX, Uint32 InWorkGroupsY, Uint32 InWorkGroupsZ)
		: WorkGroupsX(InWorkGroupsX)
		, WorkGroupsY(InWorkGroupsY)
		, WorkGroupsZ(InWorkGroupsZ)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
	}

	Uint32 WorkGroupsX;
	Uint32 WorkGroupsY;
	Uint32 WorkGroupsZ;
};

// Dispatch Rays RenderCommand
struct DispatchRaysCommand : public RenderCommand
{
	inline DispatchRaysCommand(Uint32 InWidth, Uint32 InHeigh, Uint32 InDepth)
		: Width(InWidth)
		, Height(InHeigh)
		, Depth(InDepth)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.Dispatch(Width, Height, Depth);
	}

	Uint32 Width;
	Uint32 Height;
	Uint32 Depth;
};