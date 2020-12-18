#pragma once
#include "RenderingCore.h"
#include "ICommandContext.h"
#include "PipelineState.h"

#include "Memory/Memory.h"

// Base rendercommand
struct RenderCommand
{
	inline RenderCommand()
		: NextCmd(nullptr)
	{
	}

	inline virtual ~RenderCommand()
	{
	}

	virtual void Execute(ICommandContext&) const = 0;

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
struct ClearRenderTargetViewCommand : public RenderCommand
{
	inline ClearRenderTargetViewCommand(RenderTargetView* InRenderTargetView, const ColorClearValue& InClearColor)
		: RenderTargetView(InRenderTargetView)
		, ClearColor(InClearColor)
	{
		VALIDATE(RenderTargetView != nullptr);
	}

	inline ~ClearRenderTargetViewCommand()
	{
		SAFERELEASE(RenderTargetView);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.ClearRenderTargetView(RenderTargetView, ClearColor);
	}

	RenderTargetView* RenderTargetView;
	ColorClearValue ClearColor;
};

// Clear DepthStencil RenderCommand
struct ClearDepthStencilViewCommand : public RenderCommand
{
	inline ClearDepthStencilViewCommand(DepthStencilView* InDepthStencilView, const DepthStencilClearValue& InClearValue)
		: DepthStencilView(InDepthStencilView)
		, ClearValue(InClearValue)
	{
		VALIDATE(DepthStencilView != nullptr);
	}

	inline ~ClearDepthStencilViewCommand()
	{
		SAFERELEASE(DepthStencilView);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.ClearDepthStencilView(DepthStencilView, ClearValue);
	}

	DepthStencilView* DepthStencilView;
	DepthStencilClearValue ClearValue;
};

// Clear UnorderedAccessView RenderCommand
struct ClearUnorderedAccessViewCommand : public RenderCommand
{
	inline ClearUnorderedAccessViewCommand(UnorderedAccessView* InUnorderedAccessView, const ColorClearValue& InClearColor)
		: UnorderedAccessView(InUnorderedAccessView)
		, ClearValue(InClearColor)
	{
		VALIDATE(UnorderedAccessView != nullptr);
	}

	inline ~ClearUnorderedAccessViewCommand()
	{
		SAFERELEASE(UnorderedAccessView);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.ClearUnorderedAccessView(UnorderedAccessView, ClearValue);
	}

	UnorderedAccessView* UnorderedAccessView;
	ColorClearValue ClearValue;
};

// Bind Viewport RenderCommand
struct BindViewportCommand : public RenderCommand
{
	inline BindViewportCommand(const Viewport& InViewport, UInt32 InSlot)
		: Viewport(InViewport)
		, Slot(InSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindViewport(Viewport, Slot);
	}

	Viewport Viewport;
	UInt32 Slot;
};

// Bind ScissorRect RenderCommand
struct BindScissorRectCommand : public RenderCommand
{
	inline BindScissorRectCommand(const ScissorRect& InScissorRect, UInt32 InSlot)
		: ScissorRect(InScissorRect)
		, Slot(InSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindScissorRect(ScissorRect, Slot);
	}

	ScissorRect ScissorRect;
	UInt32 Slot;
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
	inline BindVertexBuffersCommand(VertexBuffer* const * InVertexBuffers, UInt32 InVertexBufferCount, UInt32 InStartSlot)
		: VertexBuffers(InVertexBuffers)
		, VertexBufferCount(InVertexBufferCount)
		, StartSlot(InStartSlot)
	{
	}

	inline ~BindVertexBuffersCommand()
	{
		for (UInt32 i = 0; i < VertexBufferCount; i++)
		{
			if (VertexBuffers[i])
			{
				VertexBuffers[i]->Release();
			}
		}
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindVertexBuffers(VertexBuffers, VertexBufferCount, StartSlot);
	}

	VertexBuffer* const* VertexBuffers;
	UInt32 VertexBufferCount;
	UInt32 StartSlot;
};

// Bind IndexBuffer RenderCommand
struct BindIndexBufferCommand : public RenderCommand
{
	inline BindIndexBufferCommand(IndexBuffer* InIndexBuffer)
		: IndexBuffer(InIndexBuffer)
	{
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
	inline BindRenderTargetsCommand(RenderTargetView* const * InRenderTargetViews, UInt32 InRenderTargetViewCount, DepthStencilView* InDepthStencilView)
		: RenderTargetViews(InRenderTargetViews)
		, RenderTargetViewCount(InRenderTargetViewCount)
		, DepthStencilView(InDepthStencilView)
	{
	}

	inline ~BindRenderTargetsCommand()
	{
		SAFERELEASE(DepthStencilView);

		for (UInt32 i = 0; i < RenderTargetViewCount; i++)
		{
			if (RenderTargetViews[i])
			{
				RenderTargetViews[i]->Release();
			}
		}
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindRenderTargets(RenderTargetViews, RenderTargetViewCount, DepthStencilView);
	}

	RenderTargetView* const* RenderTargetViews;
	UInt32 RenderTargetViewCount;
	DepthStencilView* DepthStencilView;
};

// Bind GraphicsPipelineState RenderCommand
struct BindGraphicsPipelineStateCommand : public RenderCommand
{
	inline BindGraphicsPipelineStateCommand(GraphicsPipelineState* InPipelineState)
		: PipelineState(InPipelineState)
	{
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
	inline BindConstantBuffersCommand(Shader* InShader, ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
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
	UInt32 ConstantBufferCount;
	UInt32 StartSlot;
};

// Bind ShaderResourceViews RenderCommand
struct BindShaderResourceViewsCommand : public RenderCommand
{
	inline BindShaderResourceViewsCommand(Shader* InShader, ShaderResourceView* const* InShaderResourceViews, UInt32 InShaderResourceViewCount, UInt32 InStartSlot)
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
	UInt32 ShaderResourceViewCount;
	UInt32 StartSlot;
};

// Bind UnorderedAccessViews RenderCommand
struct BindUnorderedAccessViewsCommand : public RenderCommand
{
	inline BindUnorderedAccessViewsCommand(Shader* InShader, UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
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
	UInt32 UnorderedAccessViewCount;
	UInt32 StartSlot;
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
	inline UpdateBufferCommand(Buffer* InDestination, UInt64 InDestinationOffsetInBytes, UInt64 InSizeInBytes, const Void* InSourceData)
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
	UInt64 	DestinationOffsetInBytes;
	UInt64 	SizeInBytes;
	Void* SourceData;
};

// Update Texture RenderCommand
struct UpdateTextureCommand : public RenderCommand
{
	inline UpdateTextureCommand(Texture2D* InDestination, UInt32 InWidth, UInt32 InHeight, UInt32	InMipLevel, const Void* InSourceData)
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
	UInt32	Width;
	UInt32	Height;
	UInt32	MipLevel;
	const Void*	SourceData;
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
		VALIDATE(Texture != nullptr);
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
		VALIDATE(Texture != nullptr);
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
		VALIDATE(Buffer != nullptr);
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
		VALIDATE(Texture != nullptr);
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
	inline DrawCommand(UInt32 InVertexCount, UInt32 InStartVertexLocation)
		: VertexCount(InVertexCount)
		, StartVertexLocation(InStartVertexLocation)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.Draw(VertexCount, StartVertexLocation);
	}

	UInt32 VertexCount;
	UInt32 StartVertexLocation;
};

// DrawIndexed RenderCommand
struct DrawIndexedCommand : public RenderCommand
{
	inline DrawIndexedCommand(UInt32 InIndexCount, UInt32 InStartIndexLocation, UInt32 InBaseVertexLocation)
		: IndexCount(InIndexCount)
		, StartIndexLocation(InStartIndexLocation)
		, BaseVertexLocation(InBaseVertexLocation)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
	}

	UInt32	IndexCount;
	UInt32	StartIndexLocation;
	Int32	BaseVertexLocation;
};

// DrawInstanced RenderCommand
struct DrawInstancedCommand : public RenderCommand
{
	inline DrawInstancedCommand(UInt32 InVertexCountPerInstance, UInt32 InInstanceCount, UInt32 InStartVertexLocation, UInt32 InStartInstanceLocation)
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

	UInt32 VertexCountPerInstance;
	UInt32 InstanceCount;
	UInt32 StartVertexLocation;
	UInt32 StartInstanceLocation;
};

// DrawIndexedInstanced RenderCommand
struct DrawIndexedInstancedCommand : public RenderCommand
{
	inline DrawIndexedInstancedCommand(UInt32 InIndexCountPerInstance, UInt32 InInstanceCount, UInt32 InStartIndexLocation, UInt32 InBaseVertexLocation, UInt32 InStartInstanceLocation)
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

	UInt32	IndexCountPerInstance;
	UInt32	InstanceCount;
	UInt32	StartIndexLocation;
	Int32	BaseVertexLocation;
	UInt32	StartInstanceLocation;
};

// Dispatch Compute RenderCommand
struct DispatchComputeCommand : public RenderCommand
{
	inline DispatchComputeCommand(UInt32 InWorkGroupsX, UInt32 InWorkGroupsY, UInt32 InWorkGroupsZ)
		: WorkGroupsX(InWorkGroupsX)
		, WorkGroupsY(InWorkGroupsY)
		, WorkGroupsZ(InWorkGroupsZ)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
	}

	UInt32 WorkGroupsX;
	UInt32 WorkGroupsY;
	UInt32 WorkGroupsZ;
};

// Dispatch Rays RenderCommand
struct DispatchRaysCommand : public RenderCommand
{
	inline DispatchRaysCommand(UInt32 InWidth, UInt32 InHeigh, UInt32 InDepth)
		: Width(InWidth)
		, Height(InHeigh)
		, Depth(InDepth)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.Dispatch(Width, Height, Depth);
	}

	UInt32 Width;
	UInt32 Height;
	UInt32 Depth;
};