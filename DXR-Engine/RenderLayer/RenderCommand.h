#pragma once
#include "RenderingCore.h"
#include "ICommandContext.h"
#include "Resources.h"
#include "ResourceViews.h"

#include "Memory/Memory.h"

#include "Debug/Debug.h"

#include "Application/Log.h"

#include <Containers/ArrayView.h>

// Base rendercommand
struct RenderCommand
{
    virtual ~RenderCommand() = default;

    virtual void Execute(ICommandContext&) = 0;

    FORCEINLINE void operator()(ICommandContext& CmdContext)
    {
        Execute(CmdContext);
    }

    RenderCommand* NextCmd = nullptr;
};

// Begin RenderCommand
struct BeginRenderCommand : public RenderCommand
{
    BeginRenderCommand()
    {
        // Empty for now
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.Begin();
    }
};

// End RenderCommand
struct EndRenderCommand : public RenderCommand
{
    EndRenderCommand()
    {
        // Empty for now
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.End();
    }
};

// Clear RenderTarget RenderCommand
struct ClearRenderTargetViewRenderCommand : public RenderCommand
{
    ClearRenderTargetViewRenderCommand(RenderTargetView* InRenderTargetView, const ColorF& InClearColor)
        : RenderTargetView(InRenderTargetView)
        , ClearColor(InClearColor)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.ClearRenderTargetView(RenderTargetView.Get(), ClearColor);
    }

    TRef<RenderTargetView> RenderTargetView;
    ColorF ClearColor;
};

// Clear DepthStencil RenderCommand
struct ClearDepthStencilViewRenderCommand : public RenderCommand
{
    ClearDepthStencilViewRenderCommand(DepthStencilView* InDepthStencilView, const DepthStencilF& InClearValue)
        : DepthStencilView(InDepthStencilView)
        , ClearValue(InClearValue)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.ClearDepthStencilView(DepthStencilView.Get(), ClearValue);
    }

    TRef<DepthStencilView> DepthStencilView;
    DepthStencilF ClearValue;
};

// Clear UnorderedAccessView RenderCommand
struct ClearUnorderedAccessViewFloatRenderCommand : public RenderCommand
{
    ClearUnorderedAccessViewFloatRenderCommand(UnorderedAccessView* InUnorderedAccessView, const ColorF& InClearColor)
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor(InClearColor)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.ClearUnorderedAccessViewFloat(UnorderedAccessView.Get(), ClearColor);
    }

    TRef<UnorderedAccessView> UnorderedAccessView;
    ColorF ClearColor;
};

// SetShadingRate RenderCommand
struct SetShadingRateRenderCommand : public RenderCommand
{
    SetShadingRateRenderCommand(EShadingRate ShadingRate)
        : ShadingRate(ShadingRate)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetShadingRate(ShadingRate);
    }

    EShadingRate ShadingRate;
};

// SetShadingRateImage RenderCommand
struct SetShadingRateImageRenderCommand : public RenderCommand
{
    SetShadingRateImageRenderCommand(Texture2D* InShadingImage)
        : ShadingImage(InShadingImage)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetShadingRateImage(ShadingImage.Get());
    }

    TRef<Texture2D> ShadingImage;
};

// Set Viewport RenderCommand
struct SetViewportRenderCommand : public RenderCommand
{
    SetViewportRenderCommand(Float InWidth, Float InHeight, Float InMinDepth, Float InMaxDepth, Float InX, Float InY)
        : Width(InWidth)
        , Height(InHeight)
        , MinDepth(InMinDepth)
        , MaxDepth(InMaxDepth)
        , x(InX)
        , y(InY)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetViewport(Width, Height, MinDepth, MaxDepth, x, y);
    }

    Float Width;
    Float Height;
    Float MinDepth;
    Float MaxDepth;
    Float x;
    Float y;
};

// Set ScissorRect RenderCommand
struct SetScissorRectRenderCommand : public RenderCommand
{
    SetScissorRectRenderCommand(Float InWidth, Float InHeight, Float InX, Float InY)
        : Width(InWidth)
        , Height(InHeight)
        , x(InX)
        , y(InY)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetScissorRect(Width, Height, x, y);
    }

    Float Width;
    Float Height;
    Float x;
    Float y;
};

// Set BlendFactor RenderCommand
struct SetBlendFactorRenderCommand : public RenderCommand
{
    SetBlendFactorRenderCommand(const ColorF& InColor)
        : Color(InColor)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetBlendFactor(Color);
    }

    ColorF Color;
};

// BeginRenderPass RenderCommand
struct BeginRenderPassRenderCommand : public RenderCommand
{
    BeginRenderPassRenderCommand()
    {
        // Empty for now
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.BeginRenderPass();
    }
};

// End RenderCommand
struct EndRenderPassRenderCommand : public RenderCommand
{
    EndRenderPassRenderCommand()
    {
        // Empty for now
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.EndRenderPass();
    }
};

// Bind PrimitiveTopology RenderCommand
struct SetPrimitiveTopologyRenderCommand : public RenderCommand
{
    SetPrimitiveTopologyRenderCommand(EPrimitiveTopology InPrimitiveTopologyType)
        : PrimitiveTopologyType(InPrimitiveTopologyType)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetPrimitiveTopology(PrimitiveTopologyType);
    }

    EPrimitiveTopology PrimitiveTopologyType;
};

// Set VertexBuffers RenderCommand
struct SetVertexBuffersRenderCommand : public RenderCommand
{
    SetVertexBuffersRenderCommand(VertexBuffer** InVertexBuffers, UInt32 InVertexBufferCount, UInt32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    {
    }

    ~SetVertexBuffersRenderCommand()
    {
        for (UInt32 i = 0; i < VertexBufferCount; i++)
        {
            SafeRelease(VertexBuffers[i]);
        }

        VertexBuffers = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetVertexBuffers(VertexBuffers, VertexBufferCount, StartSlot);
    }

    VertexBuffer** VertexBuffers;
    UInt32 VertexBufferCount;
    UInt32 StartSlot;
};

// Set IndexBuffer RenderCommand
struct SetIndexBufferRenderCommand : public RenderCommand
{
    SetIndexBufferRenderCommand(IndexBuffer* InIndexBuffer)
        : IndexBuffer(InIndexBuffer)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetIndexBuffer(IndexBuffer.Get());
    }

    TRef<IndexBuffer> IndexBuffer;
};

// Set RenderTargets RenderCommand
struct SetRenderTargetsRenderCommand : public RenderCommand
{
    SetRenderTargetsRenderCommand(RenderTargetView** InRenderTargetViews, UInt32 InRenderTargetViewCount, DepthStencilView* InDepthStencilView)
        : RenderTargetViews(InRenderTargetViews)
        , RenderTargetViewCount(InRenderTargetViewCount)
        , DepthStencilView(InDepthStencilView)
    {
    }

    ~SetRenderTargetsRenderCommand()
    {
        for (UInt32 i = 0; i < RenderTargetViewCount; i++)
        {
            SafeRelease(RenderTargetViews[i]);
        }

        RenderTargetViews = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetRenderTargets(RenderTargetViews, RenderTargetViewCount, DepthStencilView.Get());
    }

    RenderTargetView** RenderTargetViews;
    UInt32 RenderTargetViewCount;
    TRef<DepthStencilView> DepthStencilView;
};

// SetRayTracingBindings RenderCommand
struct SetRayTracingBindingsRenderCommand : public RenderCommand
{
    SetRayTracingBindingsRenderCommand(
        RayTracingScene* InRayTracingScene, 
        RayTracingPipelineState* InPipelineState, 
        const RayTracingShaderResources* InGlobalResources, 
        const RayTracingShaderResources* InRayGenLocalResources,
        const RayTracingShaderResources* InMissLocalResources, 
        const RayTracingShaderResources* InHitGroupResources, 
        UInt32 InNumHitGroupResources)
        : Scene(InRayTracingScene)
        , PipelineState(InPipelineState)
        , GlobalResources(InGlobalResources)
        , RayGenLocalResources(InRayGenLocalResources)
        , MissLocalResources(InMissLocalResources)
        , HitGroupResources(InHitGroupResources)
        , NumHitGroupResources(InNumHitGroupResources)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetRayTracingBindings(Scene.Get(), PipelineState.Get(), GlobalResources, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
    }

    TRef<RayTracingScene>            Scene;
    TRef<RayTracingPipelineState>    PipelineState;
    const RayTracingShaderResources* GlobalResources;
    const RayTracingShaderResources* RayGenLocalResources;
    const RayTracingShaderResources* MissLocalResources;
    const RayTracingShaderResources* HitGroupResources;
    UInt32 NumHitGroupResources;
};

// Set GraphicsPipelineState RenderCommand
struct SetGraphicsPipelineStateRenderCommand : public RenderCommand
{
    SetGraphicsPipelineStateRenderCommand(GraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetGraphicsPipelineState(PipelineState.Get());
    }

    TRef<GraphicsPipelineState> PipelineState;
};

// Bind ComputePipelineState RenderCommand
struct SetComputePipelineStateRenderCommand : public RenderCommand
{
    SetComputePipelineStateRenderCommand(ComputePipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetComputePipelineState(PipelineState.Get());
    }

    TRef<ComputePipelineState> PipelineState;
};

// Set UseShaderResourceViews RenderCommand
struct Set32BitShaderConstantsRenderCommand : public RenderCommand
{
    Set32BitShaderConstantsRenderCommand(Shader* InShader, const Void* InShader32BitConstants, UInt32 InNum32BitConstants)
        : Shader(InShader)
        , Shader32BitConstants(InShader32BitConstants)
        , Num32BitConstants(InNum32BitConstants)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.Set32BitShaderConstants(Shader.Get(), Shader32BitConstants, Num32BitConstants);
    }

    TRef<Shader> Shader;
    const Void*  Shader32BitConstants;
    UInt32       Num32BitConstants;
};

// Set ShaderResourceViewRenderCommand
struct SetShaderResourceViewRenderCommand : public RenderCommand
{
    SetShaderResourceViewRenderCommand(Shader* InShader, ShaderResourceView* InShaderResourceView, UInt32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceView(InShaderResourceView)
        , ParameterIndex(InParameterIndex)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetShaderResourceView(Shader.Get(), ShaderResourceView.Get(), ParameterIndex);
    }

    TRef<Shader>             Shader;
    TRef<ShaderResourceView> ShaderResourceView;
    UInt32                   ParameterIndex;
};

// Set ShaderResourceViewsRenderCommand
struct SetShaderResourceViewsRenderCommand : public RenderCommand
{
    SetShaderResourceViewsRenderCommand(Shader* InShader, ShaderResourceView** InShaderResourceViews, UInt32 InNumShaderResourceViews, UInt32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceViews(InShaderResourceViews)
        , NumShaderResourceViews(InNumShaderResourceViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SetShaderResourceViewsRenderCommand()
    {
        for (UInt32 i = 0; i < NumShaderResourceViews; i++)
        {
            SafeRelease(ShaderResourceViews[i]);
        }

        ShaderResourceViews = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetShaderResourceViews(Shader.Get(), ShaderResourceViews, NumShaderResourceViews, ParameterIndex);
    }

    TRef<Shader>         Shader;
    ShaderResourceView** ShaderResourceViews;
    UInt32 NumShaderResourceViews;
    UInt32 ParameterIndex;
};

// Set UnorderedAccessViewRenderCommand
struct SetUnorderedAccessViewRenderCommand : public RenderCommand
{
    SetUnorderedAccessViewRenderCommand(Shader* InShader, UnorderedAccessView* InUnorderedAccessView, UInt32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessView(InUnorderedAccessView)
        , ParameterIndex(InParameterIndex)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetUnorderedAccessView(Shader.Get(), UnorderedAccessView.Get(), ParameterIndex);
    }

    TRef<Shader>              Shader;
    TRef<UnorderedAccessView> UnorderedAccessView;
    UInt32                    ParameterIndex;
};

// Set UnorderedAccessViewsRenderCommand
struct SetUnorderedAccessViewsRenderCommand : public RenderCommand
{
    SetUnorderedAccessViewsRenderCommand(Shader* InShader, UnorderedAccessView** InUnorderedAccessViews, UInt32 InNumUnorderedAccessViews, UInt32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , NumUnorderedAccessViews(InNumUnorderedAccessViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SetUnorderedAccessViewsRenderCommand()
    {
        for (UInt32 i = 0; i < NumUnorderedAccessViews; i++)
        {
            SafeRelease(UnorderedAccessViews[i]);
        }

        UnorderedAccessViews = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetUnorderedAccessViews(Shader.Get(), UnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
    }

    TRef<Shader>          Shader;
    UnorderedAccessView** UnorderedAccessViews;
    UInt32 NumUnorderedAccessViews;
    UInt32 ParameterIndex;
};

// Set ConstantBufferRenderCommand
struct SetConstantBufferRenderCommand : public RenderCommand
{
    SetConstantBufferRenderCommand(Shader* InShader, ConstantBuffer* InConstantBuffer, UInt32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffer(InConstantBuffer)
        , ParameterIndex(InParameterIndex)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetConstantBuffer(Shader.Get(), ConstantBuffer.Get(), ParameterIndex);
    }

    TRef<Shader>         Shader;
    TRef<ConstantBuffer> ConstantBuffer;
    UInt32               ParameterIndex;
};

// Set ConstantBuffersRenderCommand
struct SetConstantBuffersRenderCommand : public RenderCommand
{
    SetConstantBuffersRenderCommand(Shader* InShader, ConstantBuffer** InConstantBuffers, UInt32 InNumConstantBuffers, UInt32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffers(InConstantBuffers)
        , NumConstantBuffers(InNumConstantBuffers)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SetConstantBuffersRenderCommand()
    {
        for (UInt32 i = 0; i < NumConstantBuffers; i++)
        {
            SafeRelease(ConstantBuffers[i]);
        }

        ConstantBuffers = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetConstantBuffers(Shader.Get(), ConstantBuffers, NumConstantBuffers, ParameterIndex);
    }

    TRef<Shader>     Shader;
    ConstantBuffer** ConstantBuffers;
    UInt32 NumConstantBuffers;
    UInt32 ParameterIndex;
};

// Set SamplerStateRenderCommand
struct SetSamplerStateRenderCommand : public RenderCommand
{
    SetSamplerStateRenderCommand(Shader* InShader, SamplerState* InSamplerState, UInt32 InParameterIndex)
        : Shader(InShader)
        , SamplerState(InSamplerState)
        , ParameterIndex(InParameterIndex)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetSamplerState(Shader.Get(), SamplerState.Get(), ParameterIndex);
    }

    TRef<Shader>       Shader;
    TRef<SamplerState> SamplerState;
    UInt32             ParameterIndex;
};

// Set SamplerStatesRenderCommand
struct SetSamplerStatesRenderCommand : public RenderCommand
{
    SetSamplerStatesRenderCommand(Shader* InShader, SamplerState** InSamplerStates, UInt32 InNumSamplerStates, UInt32 InParameterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , NumSamplerStates(InNumSamplerStates)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SetSamplerStatesRenderCommand()
    {
        for (UInt32 i = 0; i < NumSamplerStates; i++)
        {
            SafeRelease(SamplerStates[i]);
        }

        SamplerStates = nullptr;
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.SetSamplerStates(Shader.Get(), SamplerStates, NumSamplerStates, ParameterIndex);
    }

    TRef<Shader>   Shader;
    SamplerState** SamplerStates;
    UInt32 NumSamplerStates;
    UInt32 ParameterIndex;
};

// Resolve Texture RenderCommand
struct ResolveTextureRenderCommand : public RenderCommand
{
    ResolveTextureRenderCommand(Texture* InDestination, Texture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.ResolveTexture(Destination.Get(), Source.Get());
    }

    TRef<Texture> Destination;
    TRef<Texture> Source;
};

// Update Buffer RenderCommand
struct UpdateBufferRenderCommand : public RenderCommand
{
    UpdateBufferRenderCommand(Buffer* InDestination, UInt64 InDestinationOffsetInBytes, UInt64 InSizeInBytes, const Void* InSourceData)
        : Destination(InDestination)
        , DestinationOffsetInBytes(InDestinationOffsetInBytes)
        , SizeInBytes(InSizeInBytes)
        , SourceData(InSourceData)
    {
        Assert(InSourceData != nullptr);
        Assert(InSizeInBytes != 0);
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.UpdateBuffer(Destination.Get(), DestinationOffsetInBytes, SizeInBytes, SourceData);
    }

    TRef<Buffer> Destination;
    UInt64 DestinationOffsetInBytes;
    UInt64 SizeInBytes;
    const Void* SourceData;
};

// Update Texture2D RenderCommand
struct UpdateTexture2DRenderCommand : public RenderCommand
{
    UpdateTexture2DRenderCommand(Texture2D* InDestination, UInt32 InWidth, UInt32 InHeight, UInt32 InMipLevel, const Void* InSourceData)
        : Destination(InDestination)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevel(InMipLevel)
        , SourceData(InSourceData)
    {
        Assert(InSourceData != nullptr);
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.UpdateTexture2D(Destination.Get(), Width, Height, MipLevel, SourceData);
    }

    TRef<Texture2D> Destination;
    UInt32 Width;
    UInt32 Height;
    UInt32 MipLevel;
    const Void*	SourceData;
};

// Copy Buffer RenderCommand
struct CopyBufferRenderCommand : public RenderCommand
{
    CopyBufferRenderCommand(Buffer* InDestination, Buffer* InSource, const CopyBufferInfo& InCopyBufferInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyBufferInfo(InCopyBufferInfo)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.CopyBuffer(Destination.Get(), Source.Get(), CopyBufferInfo);
    }

    TRef<Buffer> Destination;
    TRef<Buffer> Source;
    CopyBufferInfo CopyBufferInfo;
};

// Copy Texture RenderCommand
struct CopyTextureRenderCommand : public RenderCommand
{
    CopyTextureRenderCommand(Texture* InDestination, Texture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.CopyTexture(Destination.Get(), Source.Get());
    }

    TRef<Texture> Destination;
    TRef<Texture> Source;
};

// Copy Texture RenderCommand
struct CopyTextureRegionRenderCommand : public RenderCommand
{
    CopyTextureRegionRenderCommand(Texture* InDestination, Texture* InSource, const CopyTextureInfo& InCopyTextureInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyTextureInfo(InCopyTextureInfo)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.CopyTextureRegion(Destination.Get(), Source.Get(), CopyTextureInfo);
    }

    TRef<Texture> Destination;
    TRef<Texture> Source;
    CopyTextureInfo CopyTextureInfo;
};

// Destroy Resource RenderCommand
struct DiscardResourceRenderCommand : public RenderCommand
{
    DiscardResourceRenderCommand(Resource* InResource)
        : Resource(InResource)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.DiscardResource(Resource.Get());
    }

    TRef<Resource> Resource;
};

// Build RayTracing Geoemtry RenderCommand
struct BuildRayTracingGeometryRenderCommand : public RenderCommand
{
    BuildRayTracingGeometryRenderCommand(RayTracingGeometry* InRayTracingGeometry, VertexBuffer* InVertexBuffer, IndexBuffer* InIndexBuffer, Bool InUpdate)
        : RayTracingGeometry(InRayTracingGeometry)
        , VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
        , Update(InUpdate)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.BuildRayTracingGeometry(RayTracingGeometry.Get(), VertexBuffer.Get(), IndexBuffer.Get(), Update);
    }

    TRef<RayTracingGeometry> RayTracingGeometry;
    TRef<VertexBuffer> VertexBuffer;
    TRef<IndexBuffer>  IndexBuffer;
    Bool Update;
};

// Build RayTracing Scene RenderCommand
struct BuildRayTracingSceneRenderCommand : public RenderCommand
{
    BuildRayTracingSceneRenderCommand(RayTracingScene* InRayTracingScene, const RayTracingGeometryInstance* InInstances, UInt32 InNumInstances, Bool InUpdate)
        : RayTracingScene(InRayTracingScene)
        , Instances(InInstances)
        , NumInstances(InNumInstances)
        , Update(InUpdate)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.BuildRayTracingScene(RayTracingScene.Get(), Instances, NumInstances, Update);
    }

    TRef<RayTracingScene> RayTracingScene;
    const RayTracingGeometryInstance* Instances;
    UInt32 NumInstances;
    Bool Update;
};

// GenerateMips RenderCommand
struct GenerateMipsRenderCommand : public RenderCommand
{
    GenerateMipsRenderCommand(Texture* InTexture)
        : Texture(InTexture)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.GenerateMips(Texture.Get());
    }

    TRef<Texture> Texture;
};

// TransitionTexture RenderCommand
struct TransitionTextureRenderCommand : public RenderCommand
{
    TransitionTextureRenderCommand(Texture* InTexture, EResourceState InBeforeState, EResourceState InAfterState)
        : Texture(InTexture)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.TransitionTexture(Texture.Get(), BeforeState, AfterState);
    }

    TRef<Texture> Texture;
    EResourceState BeforeState;
    EResourceState AfterState;
};

// TransitionBuffer RenderCommand
struct TransitionBufferRenderCommand : public RenderCommand
{
    TransitionBufferRenderCommand(Buffer* InBuffer, EResourceState InBeforeState, EResourceState InAfterState)
        : Buffer(InBuffer)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.TransitionBuffer(Buffer.Get(), BeforeState, AfterState);
    }

    TRef<Buffer> Buffer;
    EResourceState BeforeState;
    EResourceState AfterState;
};

// UnorderedAccessTextureBarrier RenderCommand
struct UnorderedAccessTextureBarrierRenderCommand : public RenderCommand
{
    UnorderedAccessTextureBarrierRenderCommand(Texture* InTexture)
        : Texture(InTexture)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.UnorderedAccessTextureBarrier(Texture.Get());
    }

    TRef<Texture> Texture;
};

// UnorderedAccessBufferBarrier RenderCommand
struct UnorderedAccessBufferBarrierRenderCommand : public RenderCommand
{
    UnorderedAccessBufferBarrierRenderCommand(Buffer* InBuffer)
        : Buffer(InBuffer)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.UnorderedAccessBufferBarrier(Buffer.Get());
    }

    TRef<Buffer> Buffer;
};

// Draw RenderCommand
struct DrawRenderCommand : public RenderCommand
{
    DrawRenderCommand(UInt32 InVertexCount, UInt32 InStartVertexLocation)
        : VertexCount(InVertexCount)
        , StartVertexLocation(InStartVertexLocation)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.Draw(VertexCount, StartVertexLocation);
    }

    UInt32 VertexCount;
    UInt32 StartVertexLocation;
};

// DrawIndexed RenderCommand
struct DrawIndexedRenderCommand : public RenderCommand
{
    DrawIndexedRenderCommand(UInt32 InIndexCount, UInt32 InStartIndexLocation, UInt32 InBaseVertexLocation)
        : IndexCount(InIndexCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    UInt32 IndexCount;
    UInt32 StartIndexLocation;
    Int32  BaseVertexLocation;
};

// DrawInstanced RenderCommand
struct DrawInstancedRenderCommand : public RenderCommand
{
    DrawInstancedRenderCommand(UInt32 InVertexCountPerInstance, UInt32 InInstanceCount, UInt32 InStartVertexLocation, UInt32 InStartInstanceLocation)
        : VertexCountPerInstance(InVertexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartVertexLocation(InStartVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    UInt32 VertexCountPerInstance;
    UInt32 InstanceCount;
    UInt32 StartVertexLocation;
    UInt32 StartInstanceLocation;
};

// DrawIndexedInstanced RenderCommand
struct DrawIndexedInstancedRenderCommand : public RenderCommand
{
    DrawIndexedInstancedRenderCommand(UInt32 InIndexCountPerInstance, UInt32 InInstanceCount, UInt32 InStartIndexLocation, UInt32 InBaseVertexLocation, UInt32 InStartInstanceLocation)
        : IndexCountPerInstance(InIndexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    UInt32 IndexCountPerInstance;
    UInt32 InstanceCount;
    UInt32 StartIndexLocation;
    UInt32 BaseVertexLocation;
    UInt32 StartInstanceLocation;
};

// Dispatch Compute RenderCommand
struct DispatchComputeRenderCommand : public RenderCommand
{
    DispatchComputeRenderCommand(UInt32 InThreadGroupCountX, UInt32 InThreadGroupCountY, UInt32 InThreadGroupCountZ)
        : ThreadGroupCountX(InThreadGroupCountX)
        , ThreadGroupCountY(InThreadGroupCountY)
        , ThreadGroupCountZ(InThreadGroupCountZ)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    UInt32 ThreadGroupCountX;
    UInt32 ThreadGroupCountY;
    UInt32 ThreadGroupCountZ;
};

// Dispatch Rays RenderCommand
struct DispatchRaysRenderCommand : public RenderCommand
{
    DispatchRaysRenderCommand(
        RayTracingScene* InScene, 
        RayTracingPipelineState* InPipelineState, 
        UInt32 InWidth, 
        UInt32 InHeight, 
        UInt32 InDepth)
        : Scene(InScene)
        , PipelineState(InPipelineState)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.DispatchRays(Scene.Get(), PipelineState.Get(), Width, Height, Depth);
    }

    TRef<RayTracingScene>         Scene;
    TRef<RayTracingPipelineState> PipelineState;
    UInt32 Width;
    UInt32 Height;
    UInt32 Depth;
};

// InsertCommandListMarker RenderCommand
struct InsertCommandListMarkerRenderCommand : public RenderCommand
{
    InsertCommandListMarkerRenderCommand(const std::string& InMarker)
        : Marker(InMarker)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        Debug::OutputDebugString(Marker + '\n');
        LOG_INFO(Marker);

        CmdContext.InsertMarker(Marker);
    }

    std::string Marker;
};

// DebugBreak RenderCommand
struct DebugBreakRenderCommand : public RenderCommand
{
    virtual void Execute(ICommandContext& CmdContext) override
    {
        UNREFERENCED_VARIABLE(CmdContext);
        Debug::DebugBreak();
    }
};

// BeginExternalCapture RenderCommand
struct BeginExternalCaptureRenderCommand : public RenderCommand
{
    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.BeginExternalCapture();
    }
};

// EndExternalCapture RenderCommand
struct EndExternalCaptureRenderCommand : public RenderCommand
{
    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.EndExternalCapture();
    }
};