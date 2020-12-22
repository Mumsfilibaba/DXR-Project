#pragma once
#include "RenderingCore.h"
#include "ICommandContext.h"
#include "PipelineState.h"

#include "Memory/Memory.h"

// Base rendercommand
struct RenderCommand
{
	virtual ~RenderCommand() = default;

	virtual void Execute(ICommandContext&) const = 0;

	FORCEINLINE void operator()(ICommandContext& CmdContext) const
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
		// Empty for now
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
		// Empty for now
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
struct ClearUnorderedAccessViewFloatCommand : public RenderCommand
{
	inline ClearUnorderedAccessViewFloatCommand(UnorderedAccessView* InUnorderedAccessView, const Float InClearColor[4])
		: UnorderedAccessView(InUnorderedAccessView)
		, ClearColor()
	{
		VALIDATE(UnorderedAccessView != nullptr);
		Memory::Memcpy(ClearColor, InClearColor, sizeof(InClearColor));
	}

	inline ~ClearUnorderedAccessViewFloatCommand()
	{
		SAFERELEASE(UnorderedAccessView);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
	}

	UnorderedAccessView* UnorderedAccessView;
	Float ClearColor[4];
};

// Bind Viewport RenderCommand
struct BindViewportCommand : public RenderCommand
{
	inline BindViewportCommand(Float InWidth, Float InHeight, Float InMinDepth, Float InMaxDepth, Float InX, Float InY)
		: Width(InWidth)
		, Height(InHeight)
		, MinDepth(InMinDepth)
		, MaxDepth(InMaxDepth)
		, x(InX)
		, y(InY)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindViewport(Width, Height, MinDepth, MaxDepth, x, y);
	}

	Float Width;
	Float Height;
	Float MinDepth;
	Float MaxDepth;
	Float x;
	Float y;
};

// Bind ScissorRect RenderCommand
struct BindScissorRectCommand : public RenderCommand
{
	inline BindScissorRectCommand(Float InWidth, Float InHeight, Float InX, Float InY)
		: Width(InWidth)
		, Height(InHeight)
		, x(InX)
		, y(InY)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.BindScissorRect(Width, Height, x, y);
	}

	Float Width;
	Float Height;
	Float x;
	Float y;
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
		// Empty for now
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
		// Empty for now
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
		VALIDATE(InPipelineState != nullptr);
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
	inline BindConstantBuffersCommand(ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
		: ConstantBuffers(InConstantBuffers)
		, ConstantBufferCount(InConstantBufferCount)
		, StartSlot(InStartSlot)
	{
	}

	ConstantBuffer* const* ConstantBuffers;
	UInt32 ConstantBufferCount;
	UInt32 StartSlot;
};

// Bind ShaderResourceViews RenderCommand
struct BindShaderResourceViewsCommand : public RenderCommand
{
	inline BindShaderResourceViewsCommand(ShaderResourceView* const* InShaderResourceViews, UInt32 InShaderResourceViewCount, UInt32 InStartSlot)
		: ShaderResourceViews(InShaderResourceViews)
		, ShaderResourceViewCount(InShaderResourceViewCount)
		, StartSlot(InStartSlot)
	{
	}

	ShaderResourceView* const* ShaderResourceViews;
	UInt32 ShaderResourceViewCount;
	UInt32 StartSlot;
};

// Bind UnorderedAccessViews RenderCommand
struct BindUnorderedAccessViewsCommand : public RenderCommand
{
	inline BindUnorderedAccessViewsCommand(UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
		: UnorderedAccessViews(InUnorderedAccessViews)
		, UnorderedAccessViewCount(InUnorderedAccessViewCount)
		, StartSlot(InStartSlot)
	{
	}

	UnorderedAccessView* const* UnorderedAccessViews;
	UInt32 UnorderedAccessViewCount;
	UInt32 StartSlot;
};

// VertexShader Bind ConstantBuffers RenderCommand
struct VSBindConstantBuffersCommand : public BindConstantBuffersCommand
{
	inline VSBindConstantBuffersCommand(ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
		: BindConstantBuffersCommand(InConstantBuffers, InConstantBufferCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.VSBindConstantBuffers(
			ConstantBuffers,
			ConstantBufferCount,
			StartSlot);
	}
};

// VertexShader Bind ShaderResourceViews RenderCommand
struct VSBindShaderResourceViewsCommand : public BindShaderResourceViewsCommand
{
	inline VSBindShaderResourceViewsCommand(ShaderResourceView* const* InShaderResourceViews, UInt32 InShaderResourceViewCount, UInt32 InStartSlot)
		: BindShaderResourceViewsCommand(InShaderResourceViews, InShaderResourceViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.VSBindShaderResourceViews(
			ShaderResourceViews,
			ShaderResourceViewCount,
			StartSlot);
	}
};

// VertexShader Bind UnorderedAccessViews RenderCommand
struct VSBindUnorderedAccessViewsCommand : public BindUnorderedAccessViewsCommand
{
	inline VSBindUnorderedAccessViewsCommand(UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
		: BindUnorderedAccessViewsCommand(InUnorderedAccessViews, InUnorderedAccessViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.VSBindUnorderedAccessViews(
			UnorderedAccessViews,
			UnorderedAccessViewCount,
			StartSlot);
	}
};

// HullShader Bind ConstantBuffers RenderCommand
struct HSBindConstantBuffersCommand : public BindConstantBuffersCommand
{
	inline HSBindConstantBuffersCommand(ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
		: BindConstantBuffersCommand(InConstantBuffers, InConstantBufferCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.HSBindConstantBuffers(
			ConstantBuffers,
			ConstantBufferCount,
			StartSlot);
	}
};

// HullShader Bind ShaderResourceViews RenderCommand
struct HSBindShaderResourceViewsCommand : public BindShaderResourceViewsCommand
{
	inline HSBindShaderResourceViewsCommand(ShaderResourceView* const* InShaderResourceViews, UInt32 InShaderResourceViewCount, UInt32 InStartSlot)
		: BindShaderResourceViewsCommand(InShaderResourceViews, InShaderResourceViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.HSBindShaderResourceViews(
			ShaderResourceViews,
			ShaderResourceViewCount,
			StartSlot);
	}
};

// HullShader Bind UnorderedAccessViews RenderCommand
struct HSBindUnorderedAccessViewsCommand : public BindUnorderedAccessViewsCommand
{
	inline HSBindUnorderedAccessViewsCommand(UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
		: BindUnorderedAccessViewsCommand(InUnorderedAccessViews, InUnorderedAccessViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.HSBindUnorderedAccessViews(
			UnorderedAccessViews,
			UnorderedAccessViewCount,
			StartSlot);
	}
};

// DomainShader Bind ConstantBuffers RenderCommand
struct DSBindConstantBuffersCommand : public BindConstantBuffersCommand
{
	inline DSBindConstantBuffersCommand(ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
		: BindConstantBuffersCommand(InConstantBuffers, InConstantBufferCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.DSBindConstantBuffers(
			ConstantBuffers,
			ConstantBufferCount,
			StartSlot);
	}
};

// DomainShader Bind ShaderResourceViews RenderCommand
struct DSBindShaderResourceViewsCommand : public BindShaderResourceViewsCommand
{
	inline DSBindShaderResourceViewsCommand(ShaderResourceView* const* InShaderResourceViews, UInt32 InShaderResourceViewCount, UInt32 InStartSlot)
		: BindShaderResourceViewsCommand(InShaderResourceViews, InShaderResourceViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.DSBindShaderResourceViews(
			ShaderResourceViews,
			ShaderResourceViewCount,
			StartSlot);
	}
};

// DomainShader Bind UnorderedAccessViews RenderCommand
struct DSBindUnorderedAccessViewsCommand : public BindUnorderedAccessViewsCommand
{
	inline DSBindUnorderedAccessViewsCommand(UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
		: BindUnorderedAccessViewsCommand(InUnorderedAccessViews, InUnorderedAccessViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.DSBindUnorderedAccessViews(
			UnorderedAccessViews,
			UnorderedAccessViewCount,
			StartSlot);
	}
};

// GeometryShader Bind ConstantBuffers RenderCommand
struct GSBindConstantBuffersCommand : public BindConstantBuffersCommand
{
	inline GSBindConstantBuffersCommand(ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
		: BindConstantBuffersCommand(InConstantBuffers, InConstantBufferCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.GSBindConstantBuffers(
			ConstantBuffers,
			ConstantBufferCount,
			StartSlot);
	}
};

// GeometryShader Bind ShaderResourceViews RenderCommand
struct GSBindShaderResourceViewsCommand : public BindShaderResourceViewsCommand
{
	inline GSBindShaderResourceViewsCommand(ShaderResourceView* const* InShaderResourceViews, UInt32 InShaderResourceViewCount, UInt32 InStartSlot)
		: BindShaderResourceViewsCommand(InShaderResourceViews, InShaderResourceViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.GSBindShaderResourceViews(
			ShaderResourceViews,
			ShaderResourceViewCount,
			StartSlot);
	}
};

// GeometryShader Bind UnorderedAccessViews RenderCommand
struct GSBindUnorderedAccessViewsCommand : public BindUnorderedAccessViewsCommand
{
	inline GSBindUnorderedAccessViewsCommand(UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
		: BindUnorderedAccessViewsCommand(InUnorderedAccessViews, InUnorderedAccessViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.GSBindUnorderedAccessViews(
			UnorderedAccessViews,
			UnorderedAccessViewCount,
			StartSlot);
	}
};

// PixelShader Bind ConstantBuffers RenderCommand
struct PSBindConstantBuffersCommand : public BindConstantBuffersCommand
{
	inline PSBindConstantBuffersCommand(ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
		: BindConstantBuffersCommand(InConstantBuffers, InConstantBufferCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.PSBindConstantBuffers(
			ConstantBuffers,
			ConstantBufferCount,
			StartSlot);
	}
};

// PixelShader Bind ShaderResourceViews RenderCommand
struct PSBindShaderResourceViewsCommand : public BindShaderResourceViewsCommand
{
	inline PSBindShaderResourceViewsCommand(ShaderResourceView* const* InShaderResourceViews, UInt32 InShaderResourceViewCount, UInt32 InStartSlot)
		: BindShaderResourceViewsCommand(InShaderResourceViews, InShaderResourceViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.PSBindShaderResourceViews(
			ShaderResourceViews,
			ShaderResourceViewCount,
			StartSlot);
	}
};

// PixelShader Bind UnorderedAccessViews RenderCommand
struct PSBindUnorderedAccessViewsCommand : public BindUnorderedAccessViewsCommand
{
	inline PSBindUnorderedAccessViewsCommand(UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
		: BindUnorderedAccessViewsCommand(InUnorderedAccessViews, InUnorderedAccessViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.PSBindUnorderedAccessViews(
			UnorderedAccessViews,
			UnorderedAccessViewCount,
			StartSlot);
	}
};

// ComputeShader Bind ConstantBuffers RenderCommand
struct CSBindConstantBuffersCommand : public BindConstantBuffersCommand
{
	inline CSBindConstantBuffersCommand(ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
		: BindConstantBuffersCommand(InConstantBuffers, InConstantBufferCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.CSBindConstantBuffers(
			ConstantBuffers,
			ConstantBufferCount,
			StartSlot);
	}
};

// ComputeShader Bind ShaderResourceViews RenderCommand
struct CSBindShaderResourceViewsCommand : public BindShaderResourceViewsCommand
{
	inline CSBindShaderResourceViewsCommand(ShaderResourceView* const* InShaderResourceViews, UInt32 InShaderResourceViewCount, UInt32 InStartSlot)
		: BindShaderResourceViewsCommand(InShaderResourceViews, InShaderResourceViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.CSBindShaderResourceViews(
			ShaderResourceViews,
			ShaderResourceViewCount,
			StartSlot);
	}
};

// ComputeShader Bind UnorderedAccessViews RenderCommand
struct CSBindUnorderedAccessViewsCommand : public BindUnorderedAccessViewsCommand
{
	inline CSBindUnorderedAccessViewsCommand(UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
		: BindUnorderedAccessViewsCommand(InUnorderedAccessViews, InUnorderedAccessViewCount, InStartSlot)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.CSBindUnorderedAccessViews(
			UnorderedAccessViews,
			UnorderedAccessViewCount,
			StartSlot);
	}
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
		CmdContext.UpdateBuffer(
			Destination, 
			DestinationOffsetInBytes, 
			SizeInBytes, 
			SourceData);
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
	inline CopyTextureCommand(Texture* InDestination, Texture* InSource)
		: Destination(InDestination)
		, Source(InSource)
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
		CmdContext.CopyTexture(Destination, Source);
	}

	Texture* Destination;
	Texture* Source;
};

// Copy Texture RenderCommand
struct CopyTextureRegionCommand : public RenderCommand
{
	inline CopyTextureRegionCommand(Texture* InDestination, Texture* InSource, const CopyTextureInfo& InCopyTextureInfo)
		: Destination(InDestination)
		, Source(InSource)
		, CopyTextureInfo(InCopyTextureInfo)
	{
		VALIDATE(Destination != nullptr);
		VALIDATE(Source != nullptr);
	}

	inline ~CopyTextureRegionCommand()
	{
		SAFERELEASE(Destination);
		SAFERELEASE(Source);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.CopyTextureRegion(Destination, Source, CopyTextureInfo);
	}

	Texture* Destination;
	Texture* Source;
	CopyTextureInfo CopyTextureInfo;
};

// Destroy Resource RenderCommand
struct DestroyResourceCommand : public RenderCommand
{
	inline DestroyResourceCommand(PipelineResource* InResource)
		: Resource(InResource)
	{
		VALIDATE(Resource != nullptr);
	}

	inline ~DestroyResourceCommand()
	{
		SAFERELEASE(Resource);
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.DestroyResource(Resource);
	}

	PipelineResource* Resource;
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
		CmdContext.DrawInstanced(
			VertexCountPerInstance, 
			InstanceCount, 
			StartVertexLocation, 
			StartInstanceLocation);
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
		CmdContext.DrawIndexedInstanced(
			IndexCountPerInstance, 
			InstanceCount, 
			StartIndexLocation, 
			BaseVertexLocation,
			StartInstanceLocation);
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
	inline DispatchComputeCommand(UInt32 InThreadGroupCountX, UInt32 InThreadGroupCountY, UInt32 InThreadGroupCountZ)
		: ThreadGroupCountX(InThreadGroupCountX)
		, ThreadGroupCountY(InThreadGroupCountY)
		, ThreadGroupCountZ(InThreadGroupCountZ)
	{
	}

	virtual void Execute(ICommandContext& CmdContext) const override
	{
		CmdContext.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	UInt32 ThreadGroupCountX;
	UInt32 ThreadGroupCountY;
	UInt32 ThreadGroupCountZ;
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