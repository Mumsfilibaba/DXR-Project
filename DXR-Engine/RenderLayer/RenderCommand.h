#pragma once
#include "RenderingCore.h"
#include "ICommandContext.h"
#include "Resources.h"
#include "ResourceViews.h"

#include "Memory/Memory.h"

#include "Debug/Debug.h"

#include "Core/Application/Log.h"
#include "Core/Containers/ArrayView.h"

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

// BeginTimeStamp RenderCommand
struct BeginTimeStampRenderCommand : public RenderCommand
{
    BeginTimeStampRenderCommand(GPUProfiler* InProfiler, uint32 InIndex)
        : Profiler(InProfiler)
        , Index(InIndex)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.BeginTimeStamp(Profiler.Get(), Index);
    }

    TRef<GPUProfiler> Profiler;
    uint32 Index;
};

// EndTimeStamp RenderCommand
struct EndTimeStampRenderCommand : public RenderCommand
{
    EndTimeStampRenderCommand(GPUProfiler* InProfiler, uint32 InIndex)
        : Profiler(InProfiler)
        , Index(InIndex)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.EndTimeStamp(Profiler.Get(), Index);
    }

    TRef<GPUProfiler> Profiler;
    uint32 Index;
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
    SetViewportRenderCommand(float InWidth, float InHeight, float InMinDepth, float InMaxDepth, float InX, float InY)
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

    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;
    float x;
    float y;
};

// Set ScissorRect RenderCommand
struct SetScissorRectRenderCommand : public RenderCommand
{
    SetScissorRectRenderCommand(float InWidth, float InHeight, float InX, float InY)
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

    float Width;
    float Height;
    float x;
    float y;
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
    SetVertexBuffersRenderCommand(VertexBuffer** InVertexBuffers, uint32 InVertexBufferCount, uint32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    {
    }

    ~SetVertexBuffersRenderCommand()
    {
        for (uint32 i = 0; i < VertexBufferCount; i++)
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
    uint32 VertexBufferCount;
    uint32 StartSlot;
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
    SetRenderTargetsRenderCommand(RenderTargetView** InRenderTargetViews, uint32 InRenderTargetViewCount, DepthStencilView* InDepthStencilView)
        : RenderTargetViews(InRenderTargetViews)
        , RenderTargetViewCount(InRenderTargetViewCount)
        , DepthStencilView(InDepthStencilView)
    {
    }

    ~SetRenderTargetsRenderCommand()
    {
        for (uint32 i = 0; i < RenderTargetViewCount; i++)
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
    uint32 RenderTargetViewCount;
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
        uint32 InNumHitGroupResources)
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
    uint32 NumHitGroupResources;
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
    Set32BitShaderConstantsRenderCommand(Shader* InShader, const void* InShader32BitConstants, uint32 InNum32BitConstants)
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
    const void*  Shader32BitConstants;
    uint32       Num32BitConstants;
};

// Set ShaderResourceViewRenderCommand
struct SetShaderResourceViewRenderCommand : public RenderCommand
{
    SetShaderResourceViewRenderCommand(Shader* InShader, ShaderResourceView* InShaderResourceView, uint32 InParameterIndex)
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
    uint32                   ParameterIndex;
};

// Set ShaderResourceViewsRenderCommand
struct SetShaderResourceViewsRenderCommand : public RenderCommand
{
    SetShaderResourceViewsRenderCommand(Shader* InShader, ShaderResourceView** InShaderResourceViews, uint32 InNumShaderResourceViews, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceViews(InShaderResourceViews)
        , NumShaderResourceViews(InNumShaderResourceViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SetShaderResourceViewsRenderCommand()
    {
        for (uint32 i = 0; i < NumShaderResourceViews; i++)
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
    uint32 NumShaderResourceViews;
    uint32 ParameterIndex;
};

// Set UnorderedAccessViewRenderCommand
struct SetUnorderedAccessViewRenderCommand : public RenderCommand
{
    SetUnorderedAccessViewRenderCommand(Shader* InShader, UnorderedAccessView* InUnorderedAccessView, uint32 InParameterIndex)
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
    uint32                    ParameterIndex;
};

// Set UnorderedAccessViewsRenderCommand
struct SetUnorderedAccessViewsRenderCommand : public RenderCommand
{
    SetUnorderedAccessViewsRenderCommand(Shader* InShader, UnorderedAccessView** InUnorderedAccessViews, uint32 InNumUnorderedAccessViews, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , NumUnorderedAccessViews(InNumUnorderedAccessViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SetUnorderedAccessViewsRenderCommand()
    {
        for (uint32 i = 0; i < NumUnorderedAccessViews; i++)
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
    uint32 NumUnorderedAccessViews;
    uint32 ParameterIndex;
};

// Set ConstantBufferRenderCommand
struct SetConstantBufferRenderCommand : public RenderCommand
{
    SetConstantBufferRenderCommand(Shader* InShader, ConstantBuffer* InConstantBuffer, uint32 InParameterIndex)
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
    uint32               ParameterIndex;
};

// Set ConstantBuffersRenderCommand
struct SetConstantBuffersRenderCommand : public RenderCommand
{
    SetConstantBuffersRenderCommand(Shader* InShader, ConstantBuffer** InConstantBuffers, uint32 InNumConstantBuffers, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffers(InConstantBuffers)
        , NumConstantBuffers(InNumConstantBuffers)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SetConstantBuffersRenderCommand()
    {
        for (uint32 i = 0; i < NumConstantBuffers; i++)
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
    uint32 NumConstantBuffers;
    uint32 ParameterIndex;
};

// Set SamplerStateRenderCommand
struct SetSamplerStateRenderCommand : public RenderCommand
{
    SetSamplerStateRenderCommand(Shader* InShader, SamplerState* InSamplerState, uint32 InParameterIndex)
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
    uint32             ParameterIndex;
};

// Set SamplerStatesRenderCommand
struct SetSamplerStatesRenderCommand : public RenderCommand
{
    SetSamplerStatesRenderCommand(Shader* InShader, SamplerState** InSamplerStates, uint32 InNumSamplerStates, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , NumSamplerStates(InNumSamplerStates)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SetSamplerStatesRenderCommand()
    {
        for (uint32 i = 0; i < NumSamplerStates; i++)
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
    uint32 NumSamplerStates;
    uint32 ParameterIndex;
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
    UpdateBufferRenderCommand(Buffer* InDestination, uint64 InDestinationOffsetInBytes, uint64 InSizeInBytes, const void* InSourceData)
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
    uint64 DestinationOffsetInBytes;
    uint64 SizeInBytes;
    const void* SourceData;
};

// Update Texture2D RenderCommand
struct UpdateTexture2DRenderCommand : public RenderCommand
{
    UpdateTexture2DRenderCommand(Texture2D* InDestination, uint32 InWidth, uint32 InHeight, uint32 InMipLevel, const void* InSourceData)
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
    uint32 Width;
    uint32 Height;
    uint32 MipLevel;
    const void*	SourceData;
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
    BuildRayTracingGeometryRenderCommand(RayTracingGeometry* InRayTracingGeometry, VertexBuffer* InVertexBuffer, IndexBuffer* InIndexBuffer, bool InUpdate)
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
    bool Update;
};

// Build RayTracing Scene RenderCommand
struct BuildRayTracingSceneRenderCommand : public RenderCommand
{
    BuildRayTracingSceneRenderCommand(RayTracingScene* InRayTracingScene, const RayTracingGeometryInstance* InInstances, uint32 InNumInstances, bool InUpdate)
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
    uint32 NumInstances;
    bool Update;
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
    DrawRenderCommand(uint32 InVertexCount, uint32 InStartVertexLocation)
        : VertexCount(InVertexCount)
        , StartVertexLocation(InStartVertexLocation)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.Draw(VertexCount, StartVertexLocation);
    }

    uint32 VertexCount;
    uint32 StartVertexLocation;
};

// DrawIndexed RenderCommand
struct DrawIndexedRenderCommand : public RenderCommand
{
    DrawIndexedRenderCommand(uint32 InIndexCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation)
        : IndexCount(InIndexCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    uint32 IndexCount;
    uint32 StartIndexLocation;
    int32  BaseVertexLocation;
};

// DrawInstanced RenderCommand
struct DrawInstancedRenderCommand : public RenderCommand
{
    DrawInstancedRenderCommand(uint32 InVertexCountPerInstance, uint32 InInstanceCount, uint32 InStartVertexLocation, uint32 InStartInstanceLocation)
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

    uint32 VertexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartVertexLocation;
    uint32 StartInstanceLocation;
};

// DrawIndexedInstanced RenderCommand
struct DrawIndexedInstancedRenderCommand : public RenderCommand
{
    DrawIndexedInstancedRenderCommand(uint32 InIndexCountPerInstance, uint32 InInstanceCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation, uint32 InStartInstanceLocation)
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

    uint32 IndexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartIndexLocation;
    uint32 BaseVertexLocation;
    uint32 StartInstanceLocation;
};

// Dispatch Compute RenderCommand
struct DispatchComputeRenderCommand : public RenderCommand
{
    DispatchComputeRenderCommand(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
        : ThreadGroupCountX(InThreadGroupCountX)
        , ThreadGroupCountY(InThreadGroupCountY)
        , ThreadGroupCountZ(InThreadGroupCountZ)
    {
    }

    virtual void Execute(ICommandContext& CmdContext) override
    {
        CmdContext.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    uint32 ThreadGroupCountX;
    uint32 ThreadGroupCountY;
    uint32 ThreadGroupCountZ;
};

// Dispatch Rays RenderCommand
struct DispatchRaysRenderCommand : public RenderCommand
{
    DispatchRaysRenderCommand(
        RayTracingScene* InScene, 
        RayTracingPipelineState* InPipelineState, 
        uint32 InWidth, 
        uint32 InHeight, 
        uint32 InDepth)
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
    uint32 Width;
    uint32 Height;
    uint32 Depth;
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