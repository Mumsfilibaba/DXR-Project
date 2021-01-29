#pragma once
#include "RenderingCore.h"
#include "ICommandContext.h"
#include "PipelineState.h"
#include "Resources.h"
#include "ResourceViews.h"

#include "Memory/Memory.h"

#include "Debug/Debug.h"

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
    BeginCommand()
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
    EndCommand()
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
    ClearRenderTargetViewCommand(RenderTargetView* InRenderTargetView, const ColorClearValue& InClearColor)
        : RenderTargetView(InRenderTargetView)
        , ClearColor(InClearColor)
    {
        VALIDATE(RenderTargetView != nullptr);
    }

    ~ClearRenderTargetViewCommand()
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
    ClearDepthStencilViewCommand(DepthStencilView* InDepthStencilView, const DepthStencilClearValue& InClearValue)
        : DepthStencilView(InDepthStencilView)
        , ClearValue(InClearValue)
    {
        VALIDATE(DepthStencilView != nullptr);
    }

    ~ClearDepthStencilViewCommand()
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
    ClearUnorderedAccessViewFloatCommand(UnorderedAccessView* InUnorderedAccessView, const Float InClearColor[4])
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor()
    {
        VALIDATE(UnorderedAccessView != nullptr);
        Memory::Memcpy(ClearColor, InClearColor, sizeof(InClearColor));
    }

    ~ClearUnorderedAccessViewFloatCommand()
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

// SetShadingRate RenderCommand
struct SetShadingRateCommand : public RenderCommand
{
    SetShadingRateCommand(EShadingRate ShadingRate)
        : ShadingRate(ShadingRate)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.SetShadingRate(ShadingRate);
    }

    EShadingRate ShadingRate;
};

// Bind Viewport RenderCommand
struct BindViewportCommand : public RenderCommand
{
    BindViewportCommand(Float InWidth, Float InHeight, Float InMinDepth, Float InMaxDepth, Float InX, Float InY)
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
    BindScissorRectCommand(Float InWidth, Float InHeight, Float InX, Float InY)
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
    BindBlendFactorCommand(const ColorClearValue& InColor)
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
    BeginRenderPassCommand()
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
    EndRenderPassCommand()
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
    BindPrimitiveTopologyCommand(EPrimitiveTopology InPrimitiveTopologyType)
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
    BindVertexBuffersCommand(VertexBuffer* const * InVertexBuffers, UInt32 InVertexBufferCount, UInt32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    {
    }

    ~BindVertexBuffersCommand()
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
    BindIndexBufferCommand(IndexBuffer* InIndexBuffer)
        : IndexBuffer(InIndexBuffer)
    {
    }

    ~BindIndexBufferCommand()
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
    BindRayTracingSceneCommand(RayTracingScene* InRayTracingScene)
        : RayTracingScene(InRayTracingScene)
    {
        VALIDATE(RayTracingScene != nullptr);
    }

    ~BindRayTracingSceneCommand()
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
    BindRenderTargetsCommand(RenderTargetView* const * InRenderTargetViews, UInt32 InRenderTargetViewCount, DepthStencilView* InDepthStencilView)
        : RenderTargetViews(InRenderTargetViews)
        , RenderTargetViewCount(InRenderTargetViewCount)
        , DepthStencilView(InDepthStencilView)
    {
    }

    ~BindRenderTargetsCommand()
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
    BindGraphicsPipelineStateCommand(GraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
        VALIDATE(InPipelineState != nullptr);
    }

    ~BindGraphicsPipelineStateCommand()
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
    BindComputePipelineStateCommand(ComputePipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    ~BindComputePipelineStateCommand()
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
    BindRayTracingPipelineStateCommand(RayTracingPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    ~BindRayTracingPipelineStateCommand()
    {
        SAFERELEASE(PipelineState);
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.BindRayTracingPipelineState(PipelineState);
    }

    RayTracingPipelineState* PipelineState;
};

// Bind ShaderResourceViews RenderCommand
struct Bind32BitShaderConstantsCommand : public RenderCommand
{
    Bind32BitShaderConstantsCommand(EShaderStage InShaderStage, const Void* InShader32BitConstants, UInt32 InNum32BitConstants)
        : ShaderStage(InShaderStage)
        , Shader32BitConstants(InShader32BitConstants)
        , Num32BitConstants(InNum32BitConstants)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.Bind32BitShaderConstants(
            ShaderStage,
            Shader32BitConstants,
            Num32BitConstants);
    }

    EShaderStage ShaderStage;
    const Void* Shader32BitConstants;
    UInt32 Num32BitConstants;
};

// Bind ConstantBuffers RenderCommand
struct BindConstantBuffersCommand : public RenderCommand
{
    BindConstantBuffersCommand(EShaderStage InShaderStage, ConstantBuffer* const* InConstantBuffers, UInt32 InConstantBufferCount, UInt32 InStartSlot)
        : ShaderStage(InShaderStage)
        , ConstantBuffers(InConstantBuffers)
        , ConstantBufferCount(InConstantBufferCount)
        , StartSlot(InStartSlot)
    {
    }

    ~BindConstantBuffersCommand()
    {
        for (UInt32 i = 0; i < ConstantBufferCount; i++)
        {
            if (ConstantBuffers[i])
            {
                ConstantBuffers[i]->Release();
            }
        }

        ConstantBuffers = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.BindConstantBuffers(
            ShaderStage,
            ConstantBuffers,
            ConstantBufferCount,
            StartSlot);
    }

    EShaderStage ShaderStage;
    ConstantBuffer* const* ConstantBuffers;
    UInt32 ConstantBufferCount;
    UInt32 StartSlot;
};

// Bind ShaderResourceView RenderCommand
struct BindShaderResourceViewsCommand : public RenderCommand
{
    BindShaderResourceViewsCommand(EShaderStage InShaderStage, ShaderResourceView* const* InShaderResourceViews, UInt32 InConstantBufferCount, UInt32 InStartSlot)
        : ShaderStage(InShaderStage)
        , ShaderResourceViews(InShaderResourceViews)
        , ShaderResourceViewCount(InConstantBufferCount)
        , StartSlot(InStartSlot)
    {
    }

    ~BindShaderResourceViewsCommand()
    {
        for (UInt32 i = 0; i < ShaderResourceViewCount; i++)
        {
            if (ShaderResourceViews[i])
            {
                ShaderResourceViews[i]->Release();
            }
        }

        ShaderResourceViews = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.BindShaderResourceViews(
            ShaderStage,
            ShaderResourceViews,
            ShaderResourceViewCount,
            StartSlot);
    }

    EShaderStage ShaderStage;
    ShaderResourceView* const* ShaderResourceViews;
    UInt32 ShaderResourceViewCount;
    UInt32 StartSlot;
};

// Bind UnorderedAccessViews RenderCommand
struct BindUnorderedAccessViewsCommand : public RenderCommand
{
    BindUnorderedAccessViewsCommand(EShaderStage InShaderStage, UnorderedAccessView* const* InUnorderedAccessViews, UInt32 InUnorderedAccessViewCount, UInt32 InStartSlot)
        : ShaderStage(InShaderStage)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , UnorderedAccessViewCount(InUnorderedAccessViewCount)
        , StartSlot(InStartSlot)
    {
    }

    ~BindUnorderedAccessViewsCommand()
    {
        for (UInt32 i = 0; i < UnorderedAccessViewCount; i++)
        {
            if (UnorderedAccessViews[i])
            {
                UnorderedAccessViews[i]->Release();
            }
        }

        UnorderedAccessViews = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.BindUnorderedAccessViews(
            ShaderStage,
            UnorderedAccessViews,
            UnorderedAccessViewCount,
            StartSlot);
    }

    EShaderStage ShaderStage;
    UnorderedAccessView* const* UnorderedAccessViews;
    UInt32 UnorderedAccessViewCount;
    UInt32 StartSlot;
};

// Bind SamplerStates RenderCommand
struct BindSamplerStatesCommand : public RenderCommand
{
    BindSamplerStatesCommand(EShaderStage InShaderStage, SamplerState* const* InSamplerStates, UInt32 InSamplerStateCount, UInt32 InStartSlot)
        : ShaderStage(InShaderStage)
        , SamplerStates(InSamplerStates)
        , SamplerStateCount(InSamplerStateCount)
        , StartSlot(InStartSlot)
    {
    }

    ~BindSamplerStatesCommand()
    {
        for (UInt32 i = 0; i < SamplerStateCount; i++)
        {
            if (SamplerStates[i])
            {
                SamplerStates[i]->Release();
            }
        }

        SamplerStates = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.BindSamplerStates(
            ShaderStage,
            SamplerStates,
            SamplerStateCount,
            StartSlot);
    }

    EShaderStage ShaderStage;
    SamplerState* const* SamplerStates;
    UInt32 SamplerStateCount;
    UInt32 StartSlot;
};

// Resolve Texture RenderCommand
struct ResolveTextureCommand : public RenderCommand
{
    ResolveTextureCommand(Texture* InDestination, Texture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
        VALIDATE(Destination != nullptr);
        VALIDATE(Source != nullptr);
    }

    ~ResolveTextureCommand()
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
    UpdateBufferCommand(Buffer* InDestination, UInt64 InDestinationOffsetInBytes, UInt64 InSizeInBytes, const Void* InSourceData)
        : Destination(InDestination)
        , DestinationOffsetInBytes(InDestinationOffsetInBytes)
        , SizeInBytes(InSizeInBytes)
        , SourceData(InSourceData)
    {
        VALIDATE(InDestination != nullptr);
        VALIDATE(InSourceData != nullptr);
        VALIDATE(InSizeInBytes != 0);
    }

    ~UpdateBufferCommand()
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
    const Void* SourceData;
};

// Update Texture2D RenderCommand
struct UpdateTexture2DCommand : public RenderCommand
{
    UpdateTexture2DCommand(Texture2D* InDestination, UInt32 InWidth, UInt32 InHeight, UInt32	InMipLevel, const Void* InSourceData)
        : Destination(InDestination)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevel(InMipLevel)
        , SourceData(InSourceData)
    {
        VALIDATE(InDestination != nullptr);
        VALIDATE(InSourceData != nullptr);
    }

    ~UpdateTexture2DCommand()
    {
        SAFERELEASE(Destination);
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.UpdateTexture2D(Destination, Width, Height, MipLevel, SourceData);
    }

    Texture2D* Destination;
    UInt32 Width;
    UInt32 Height;
    UInt32 MipLevel;
    const Void*	SourceData;
};

// Copy Buffer RenderCommand
struct CopyBufferCommand : public RenderCommand
{
    CopyBufferCommand(Buffer* InDestination, Buffer* InSource, const CopyBufferInfo& InCopyBufferInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyBufferInfo(InCopyBufferInfo)
    {
        VALIDATE(Destination != nullptr);
        VALIDATE(Source != nullptr);
    }

    ~CopyBufferCommand()
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
    CopyTextureCommand(Texture* InDestination, Texture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
        VALIDATE(Destination != nullptr);
        VALIDATE(Source != nullptr);
    }

    ~CopyTextureCommand()
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
    CopyTextureRegionCommand(Texture* InDestination, Texture* InSource, const CopyTextureInfo& InCopyTextureInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyTextureInfo(InCopyTextureInfo)
    {
        VALIDATE(Destination != nullptr);
        VALIDATE(Source != nullptr);
    }

    ~CopyTextureRegionCommand()
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
    DestroyResourceCommand(PipelineResource* InResource)
        : Resource(InResource)
    {
        VALIDATE(Resource != nullptr);
    }

    ~DestroyResourceCommand()
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
    BuildRayTracingGeometryCommand(RayTracingGeometry* InRayTracingGeometry)
        : RayTracingGeometry(InRayTracingGeometry)
    {
        VALIDATE(RayTracingGeometry != nullptr);
    }

    ~BuildRayTracingGeometryCommand()
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
    BuildRayTracingSceneCommand(RayTracingScene* InRayTracingScene)
        : RayTracingScene(InRayTracingScene)
    {
        VALIDATE(RayTracingScene != nullptr);
    }

    ~BuildRayTracingSceneCommand()
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
    GenerateMipsCommand(Texture* InTexture)
        : Texture(InTexture)
    {
        VALIDATE(Texture != nullptr);
    }

    ~GenerateMipsCommand()
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
    TransitionTextureCommand(Texture* InTexture, EResourceState InBeforeState, EResourceState InAfterState)
        : Texture(InTexture)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
        VALIDATE(Texture != nullptr);
    }

    ~TransitionTextureCommand()
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
    TransitionBufferCommand(Buffer* InBuffer, EResourceState InBeforeState, EResourceState InAfterState)
        : Buffer(InBuffer)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
        VALIDATE(Buffer != nullptr);
    }

    ~TransitionBufferCommand()
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
    UnorderedAccessTextureBarrierCommand(Texture* InTexture)
        : Texture(InTexture)
    {
        VALIDATE(Texture != nullptr);
    }

    ~UnorderedAccessTextureBarrierCommand()
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
    DrawCommand(UInt32 InVertexCount, UInt32 InStartVertexLocation)
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
    DrawIndexedCommand(UInt32 InIndexCount, UInt32 InStartIndexLocation, UInt32 InBaseVertexLocation)
        : IndexCount(InIndexCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        CmdContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    UInt32 IndexCount;
    UInt32 StartIndexLocation;
    Int32  BaseVertexLocation;
};

// DrawInstanced RenderCommand
struct DrawInstancedCommand : public RenderCommand
{
    DrawInstancedCommand(UInt32 InVertexCountPerInstance, UInt32 InInstanceCount, UInt32 InStartVertexLocation, UInt32 InStartInstanceLocation)
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
    DrawIndexedInstancedCommand(UInt32 InIndexCountPerInstance, UInt32 InInstanceCount, UInt32 InStartIndexLocation, UInt32 InBaseVertexLocation, UInt32 InStartInstanceLocation)
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

    UInt32 IndexCountPerInstance;
    UInt32 InstanceCount;
    UInt32 StartIndexLocation;
    Int32  BaseVertexLocation;
    UInt32 StartInstanceLocation;
};

// Dispatch Compute RenderCommand
struct DispatchComputeCommand : public RenderCommand
{
    DispatchComputeCommand(UInt32 InThreadGroupCountX, UInt32 InThreadGroupCountY, UInt32 InThreadGroupCountZ)
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
    DispatchRaysCommand(UInt32 InWidth, UInt32 InHeigh, UInt32 InDepth)
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

// InsertCommandListMarker RenderCommand
struct InsertCommandListMarkerCommand : public RenderCommand
{
    InsertCommandListMarkerCommand(const std::string& InMarker)
        : Marker(InMarker)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) const override
    {
        Debug::OutputDebugString(Marker + '\n');
        LOG_INFO(Marker);

        CmdContext.InsertMarker(Marker);
    }

    std::string Marker;
};