#pragma once
#include "RHITypes.h"
#include "IRHICommandContext.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"

#include "Core/Debug/Debug.h"
#include "Core/Memory/Memory.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/ArrayView.h"

// Base render command
struct SRHIRenderCommand
{
    virtual ~SRHIRenderCommand() = default;

    virtual void Execute(IRHICommandContext&) = 0;

    FORCEINLINE void operator()(IRHICommandContext& CmdContext)
    {
        Execute(CmdContext);
    }

    SRHIRenderCommand* NextCmd = nullptr;
};

// BeginTimeStamp RenderCommand
struct SRHIBeginTimeStampRenderCommand : public SRHIRenderCommand
{
    SRHIBeginTimeStampRenderCommand(CRHITimestampQuery* InProfiler, uint32 InIndex)
        : Profiler(InProfiler)
        , Index(InIndex)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.BeginTimeStamp(Profiler.Get(), Index);
    }

    TSharedRef<CRHITimestampQuery> Profiler;
    uint32 Index;
};

// EndTimeStamp RenderCommand
struct SRHIEndTimeStampRenderCommand : public SRHIRenderCommand
{
    SRHIEndTimeStampRenderCommand(CRHITimestampQuery* InProfiler, uint32 InIndex)
        : Profiler(InProfiler)
        , Index(InIndex)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.EndTimeStamp(Profiler.Get(), Index);
    }

    TSharedRef<CRHITimestampQuery> Profiler;
    uint32 Index;
};

// Clear RenderTarget RenderCommand
struct SRHIClearRenderTargetViewRenderCommand : public SRHIRenderCommand
{
    SRHIClearRenderTargetViewRenderCommand(CRHIRenderTargetView* InRenderTargetView, const SColorF& InClearColor)
        : RenderTargetView(InRenderTargetView)
        , ClearColor(InClearColor)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.ClearRenderTargetView(RenderTargetView.Get(), ClearColor);
    }

    TSharedRef<CRHIRenderTargetView> RenderTargetView;
    SColorF ClearColor;
};

// Clear DepthStencil RenderCommand
struct SRHIClearDepthStencilViewRenderCommand : public SRHIRenderCommand
{
    SRHIClearDepthStencilViewRenderCommand(CRHIDepthStencilView* InDepthStencilView, const SDepthStencil& InClearValue)
        : DepthStencilView(InDepthStencilView)
        , ClearValue(InClearValue)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.ClearDepthStencilView(DepthStencilView.Get(), ClearValue);
    }

    TSharedRef<CRHIDepthStencilView> DepthStencilView;
    SDepthStencil ClearValue;
};

// Clear UnorderedAccessView RenderCommand
struct SRHIClearUnorderedAccessViewFloatRenderCommand : public SRHIRenderCommand
{
    SRHIClearUnorderedAccessViewFloatRenderCommand(CRHIUnorderedAccessView* InUnorderedAccessView, const SColorF& InClearColor)
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor(InClearColor)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.ClearUnorderedAccessViewFloat(UnorderedAccessView.Get(), ClearColor);
    }

    TSharedRef<CRHIUnorderedAccessView> UnorderedAccessView;
    SColorF ClearColor;
};

// SetShadingRate RenderCommand
struct SRHISetShadingRateRenderCommand : public SRHIRenderCommand
{
    SRHISetShadingRateRenderCommand(EShadingRate ShadingRate)
        : ShadingRate(ShadingRate)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetShadingRate(ShadingRate);
    }

    EShadingRate ShadingRate;
};

// SetShadingRateImage RenderCommand
struct SRHISetShadingRateImageRenderCommand : public SRHIRenderCommand
{
    SRHISetShadingRateImageRenderCommand(CRHITexture2D* InShadingImage)
        : ShadingImage(InShadingImage)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetShadingRateImage(ShadingImage.Get());
    }

    TSharedRef<CRHITexture2D> ShadingImage;
};

// Set Viewport RenderCommand
struct SRHISetViewportRenderCommand : public SRHIRenderCommand
{
    SRHISetViewportRenderCommand(float InWidth, float InHeight, float InMinDepth, float InMaxDepth, float InX, float InY)
        : Width(InWidth)
        , Height(InHeight)
        , MinDepth(InMinDepth)
        , MaxDepth(InMaxDepth)
        , x(InX)
        , y(InY)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
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
struct SRHISetScissorRectRenderCommand : public SRHIRenderCommand
{
    SRHISetScissorRectRenderCommand(float InWidth, float InHeight, float InX, float InY)
        : Width(InWidth)
        , Height(InHeight)
        , x(InX)
        , y(InY)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetScissorRect(Width, Height, x, y);
    }

    float Width;
    float Height;
    float x;
    float y;
};

// Set BlendFactor RenderCommand
struct SRHISetBlendFactorRenderCommand : public SRHIRenderCommand
{
    SRHISetBlendFactorRenderCommand(const SColorF& InColor)
        : Color(InColor)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetBlendFactor(Color);
    }

    SColorF Color;
};

// BeginRenderPass RenderCommand
struct SRHIBeginRenderPassRenderCommand : public SRHIRenderCommand
{
    SRHIBeginRenderPassRenderCommand()
    {
        // Empty for now
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.BeginRenderPass();
    }
};

// End RenderCommand
struct SRHIEndRenderPassRenderCommand : public SRHIRenderCommand
{
    SRHIEndRenderPassRenderCommand()
    {
        // Empty for now
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.EndRenderPass();
    }
};

// Bind PrimitiveTopology RenderCommand
struct SRHISetPrimitiveTopologyRenderCommand : public SRHIRenderCommand
{
    SRHISetPrimitiveTopologyRenderCommand(EPrimitiveTopology InPrimitiveTopologyType)
        : PrimitiveTopologyType(InPrimitiveTopologyType)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetPrimitiveTopology(PrimitiveTopologyType);
    }

    EPrimitiveTopology PrimitiveTopologyType;
};

// Set VertexBuffers RenderCommand
struct SRHISetVertexBuffersRenderCommand : public SRHIRenderCommand
{
    SRHISetVertexBuffersRenderCommand(CRHIVertexBuffer** InVertexBuffers, uint32 InVertexBufferCount, uint32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    {
    }

    ~SRHISetVertexBuffersRenderCommand()
    {
        for (uint32 i = 0; i < VertexBufferCount; i++)
        {
            SafeRelease(VertexBuffers[i]);
        }

        VertexBuffers = nullptr;
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetVertexBuffers(VertexBuffers, VertexBufferCount, StartSlot);
    }

    CRHIVertexBuffer** VertexBuffers;

    uint32 VertexBufferCount;
    uint32 StartSlot;
};

// Set IndexBuffer RenderCommand
struct SRHISetIndexBufferRenderCommand : public SRHIRenderCommand
{
    SRHISetIndexBufferRenderCommand(const TSharedRef<CRHIIndexBuffer>& InIndexBuffer)
        : IndexBuffer(InIndexBuffer)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetIndexBuffer(IndexBuffer.Get());
    }

    TSharedRef<CRHIIndexBuffer> IndexBuffer;
};

// Set RenderTargets RenderCommand
struct SRHISetRenderTargetsRenderCommand : public SRHIRenderCommand
{
    SRHISetRenderTargetsRenderCommand(CRHIRenderTargetView** InRenderTargetViews, uint32 InRenderTargetViewCount, const TSharedRef<CRHIDepthStencilView>& InDepthStencilView)
        : RenderTargetViews(InRenderTargetViews)
        , RenderTargetViewCount(InRenderTargetViewCount)
        , DepthStencilView(InDepthStencilView)
    {
    }

    ~SRHISetRenderTargetsRenderCommand()
    {
        for (uint32 i = 0; i < RenderTargetViewCount; i++)
        {
            SafeRelease(RenderTargetViews[i]);
        }

        RenderTargetViews = nullptr;
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetRenderTargets(RenderTargetViews, RenderTargetViewCount, DepthStencilView.Get());
    }

    CRHIRenderTargetView** RenderTargetViews;
    uint32 RenderTargetViewCount;

    TSharedRef<CRHIDepthStencilView> DepthStencilView;
};

// SetRayTracingBindings RenderCommand
struct SRHISetRayTracingBindingsRenderCommand : public SRHIRenderCommand
{
    SRHISetRayTracingBindingsRenderCommand(
        CRHIRayTracingScene* InRayTracingScene,
        CRHIRayTracingPipelineState* InPipelineState,
        const SRayTracingShaderResources* InGlobalResources,
        const SRayTracingShaderResources* InRayGenLocalResources,
        const SRayTracingShaderResources* InMissLocalResources,
        const SRayTracingShaderResources* InHitGroupResources,
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

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetRayTracingBindings(Scene.Get(), PipelineState.Get(), GlobalResources, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
    }

    TSharedRef<CRHIRayTracingScene>         Scene;
    TSharedRef<CRHIRayTracingPipelineState> PipelineState;

    const SRayTracingShaderResources* GlobalResources;
    const SRayTracingShaderResources* RayGenLocalResources;
    const SRayTracingShaderResources* MissLocalResources;
    const SRayTracingShaderResources* HitGroupResources;

    uint32 NumHitGroupResources;
};

// Set GraphicsPipelineState RenderCommand
struct SRHISetGraphicsPipelineStateRenderCommand : public SRHIRenderCommand
{
    SRHISetGraphicsPipelineStateRenderCommand(CRHIGraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetGraphicsPipelineState(PipelineState.Get());
    }

    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
};

// Bind ComputePipelineState RenderCommand
struct SRHISetComputePipelineStateRenderCommand : public SRHIRenderCommand
{
    SRHISetComputePipelineStateRenderCommand(CRHIComputePipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetComputePipelineState(PipelineState.Get());
    }

    TSharedRef<CRHIComputePipelineState> PipelineState;
};

// Set UseShaderResourceViews RenderCommand
struct SRHISet32BitShaderConstantsRenderCommand : public SRHIRenderCommand
{
    SRHISet32BitShaderConstantsRenderCommand(CRHIShader* InShader, const void* InShader32BitConstants, uint32 InNum32BitConstants)
        : Shader(InShader)
        , Shader32BitConstants(InShader32BitConstants)
        , Num32BitConstants(InNum32BitConstants)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.Set32BitShaderConstants(Shader.Get(), Shader32BitConstants, Num32BitConstants);
    }

    TSharedRef<CRHIShader> Shader;

    const void* Shader32BitConstants;
    uint32      Num32BitConstants;
};

// Set ShaderResourceViewRenderCommand
struct SRHISetShaderResourceViewRenderCommand : public SRHIRenderCommand
{
    SRHISetShaderResourceViewRenderCommand(CRHIShader* InShader, CRHIShaderResourceView* InShaderResourceView, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceView(InShaderResourceView)
        , ParameterIndex(InParameterIndex)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetShaderResourceView(Shader.Get(), ShaderResourceView.Get(), ParameterIndex);
    }

    TSharedRef<CRHIShader>             Shader;
    TSharedRef<CRHIShaderResourceView> ShaderResourceView;

    uint32 ParameterIndex;
};

// Set ShaderResourceViewsRenderCommand
struct SRHISetShaderResourceViewsRenderCommand : public SRHIRenderCommand
{
    SRHISetShaderResourceViewsRenderCommand(CRHIShader* InShader, CRHIShaderResourceView** InShaderResourceViews, uint32 InNumShaderResourceViews, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceViews(InShaderResourceViews)
        , NumShaderResourceViews(InNumShaderResourceViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SRHISetShaderResourceViewsRenderCommand()
    {
        for (uint32 i = 0; i < NumShaderResourceViews; i++)
        {
            SafeRelease(ShaderResourceViews[i]);
        }

        ShaderResourceViews = nullptr;
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetShaderResourceViews(Shader.Get(), ShaderResourceViews, NumShaderResourceViews, ParameterIndex);
    }

    TSharedRef<CRHIShader>   Shader;
    CRHIShaderResourceView** ShaderResourceViews;

    uint32 NumShaderResourceViews;
    uint32 ParameterIndex;
};

// Set UnorderedAccessViewRenderCommand
struct SRHISetUnorderedAccessViewRenderCommand : public SRHIRenderCommand
{
    SRHISetUnorderedAccessViewRenderCommand(CRHIShader* InShader, CRHIUnorderedAccessView* InUnorderedAccessView, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessView(InUnorderedAccessView)
        , ParameterIndex(InParameterIndex)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetUnorderedAccessView(Shader.Get(), UnorderedAccessView.Get(), ParameterIndex);
    }

    TSharedRef<CRHIShader>              Shader;
    TSharedRef<CRHIUnorderedAccessView> UnorderedAccessView;

    uint32 ParameterIndex;
};

// Set UnorderedAccessViewsRenderCommand
struct SRHISetUnorderedAccessViewsRenderCommand : public SRHIRenderCommand
{
    SRHISetUnorderedAccessViewsRenderCommand(CRHIShader* InShader, CRHIUnorderedAccessView** InUnorderedAccessViews, uint32 InNumUnorderedAccessViews, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , NumUnorderedAccessViews(InNumUnorderedAccessViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SRHISetUnorderedAccessViewsRenderCommand()
    {
        for (uint32 i = 0; i < NumUnorderedAccessViews; i++)
        {
            SafeRelease(UnorderedAccessViews[i]);
        }

        UnorderedAccessViews = nullptr;
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetUnorderedAccessViews(Shader.Get(), UnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
    }

    TSharedRef<CRHIShader>    Shader;
    CRHIUnorderedAccessView** UnorderedAccessViews;

    uint32 NumUnorderedAccessViews;
    uint32 ParameterIndex;
};

// Set ConstantBufferRenderCommand
struct SRHISetConstantBufferRenderCommand : public SRHIRenderCommand
{
    SRHISetConstantBufferRenderCommand(CRHIShader* InShader, CRHIConstantBuffer* InConstantBuffer, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffer(InConstantBuffer)
        , ParameterIndex(InParameterIndex)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetConstantBuffer(Shader.Get(), ConstantBuffer.Get(), ParameterIndex);
    }

    TSharedRef<CRHIShader>         Shader;
    TSharedRef<CRHIConstantBuffer> ConstantBuffer;

    uint32 ParameterIndex;
};

// Set ConstantBuffersRenderCommand
struct SRHISetConstantBuffersRenderCommand : public SRHIRenderCommand
{
    SRHISetConstantBuffersRenderCommand(CRHIShader* InShader, CRHIConstantBuffer** InConstantBuffers, uint32 InNumConstantBuffers, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffers(InConstantBuffers)
        , NumConstantBuffers(InNumConstantBuffers)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SRHISetConstantBuffersRenderCommand()
    {
        for (uint32 i = 0; i < NumConstantBuffers; i++)
        {
            SafeRelease(ConstantBuffers[i]);
        }

        ConstantBuffers = nullptr;
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetConstantBuffers(Shader.Get(), ConstantBuffers, NumConstantBuffers, ParameterIndex);
    }

    TSharedRef<CRHIShader> Shader;
    CRHIConstantBuffer** ConstantBuffers;

    uint32 NumConstantBuffers;
    uint32 ParameterIndex;
};

// Set SamplerStateRenderCommand
struct SRHISetSamplerStateRenderCommand : public SRHIRenderCommand
{
    SRHISetSamplerStateRenderCommand(CRHIShader* InShader, CRHISamplerState* InSamplerState, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerState(InSamplerState)
        , ParameterIndex(InParameterIndex)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetSamplerState(Shader.Get(), SamplerState.Get(), ParameterIndex);
    }

    TSharedRef<CRHIShader>       Shader;
    TSharedRef<CRHISamplerState> SamplerState;

    uint32 ParameterIndex;
};

// Set SamplerStatesRenderCommand
struct SRHISetSamplerStatesRenderCommand : public SRHIRenderCommand
{
    SRHISetSamplerStatesRenderCommand(CRHIShader* InShader, CRHISamplerState** InSamplerStates, uint32 InNumSamplerStates, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , NumSamplerStates(InNumSamplerStates)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~SRHISetSamplerStatesRenderCommand()
    {
        for (uint32 i = 0; i < NumSamplerStates; i++)
        {
            SafeRelease(SamplerStates[i]);
        }

        SamplerStates = nullptr;
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetSamplerStates(Shader.Get(), SamplerStates, NumSamplerStates, ParameterIndex);
    }

    TSharedRef<CRHIShader> Shader;
    CRHISamplerState** SamplerStates;

    uint32 NumSamplerStates;
    uint32 ParameterIndex;
};

// Resolve Texture RenderCommand
struct SRHIResolveTextureRenderCommand : public SRHIRenderCommand
{
    SRHIResolveTextureRenderCommand(CRHITexture* InDestination, CRHITexture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.ResolveTexture(Destination.Get(), Source.Get());
    }

    TSharedRef<CRHITexture> Destination;
    TSharedRef<CRHITexture> Source;
};

// Update Buffer RenderCommand
struct SRHIUpdateBufferRenderCommand : public SRHIRenderCommand
{
    SRHIUpdateBufferRenderCommand(CRHIBuffer* InDestination, uint64 InDestinationOffsetInBytes, uint64 InSizeInBytes, const void* InSourceData)
        : Destination(InDestination)
        , DestinationOffsetInBytes(InDestinationOffsetInBytes)
        , SizeInBytes(InSizeInBytes)
        , SourceData(InSourceData)
    {
        Assert(InSourceData != nullptr);
        Assert(InSizeInBytes != 0);
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.UpdateBuffer(Destination.Get(), DestinationOffsetInBytes, SizeInBytes, SourceData);
    }

    TSharedRef<CRHIBuffer> Destination;
    uint64 DestinationOffsetInBytes;
    uint64 SizeInBytes;
    const void* SourceData;
};

// Update Texture2D RenderCommand
struct SRHIUpdateTexture2DRenderCommand : public SRHIRenderCommand
{
    SRHIUpdateTexture2DRenderCommand(CRHITexture2D* InDestination, uint32 InWidth, uint32 InHeight, uint32 InMipLevel, const void* InSourceData)
        : Destination(InDestination)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevel(InMipLevel)
        , SourceData(InSourceData)
    {
        Assert(InSourceData != nullptr);
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.UpdateTexture2D(Destination.Get(), Width, Height, MipLevel, SourceData);
    }

    TSharedRef<CRHITexture2D> Destination;
    uint32 Width;
    uint32 Height;
    uint32 MipLevel;
    const void* SourceData;
};

// Copy Buffer RenderCommand
struct SRHICopyBufferRenderCommand : public SRHIRenderCommand
{
    SRHICopyBufferRenderCommand(CRHIBuffer* InDestination, CRHIBuffer* InSource, const SCopyBufferInfo& InCopyBufferInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyBufferInfo(InCopyBufferInfo)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.CopyBuffer(Destination.Get(), Source.Get(), CopyBufferInfo);
    }

    TSharedRef<CRHIBuffer> Destination;
    TSharedRef<CRHIBuffer> Source;
    SCopyBufferInfo CopyBufferInfo;
};

// Copy Texture RenderCommand
struct SRHICopyTextureRenderCommand : public SRHIRenderCommand
{
    SRHICopyTextureRenderCommand(CRHITexture* InDestination, CRHITexture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.CopyTexture(Destination.Get(), Source.Get());
    }

    TSharedRef<CRHITexture> Destination;
    TSharedRef<CRHITexture> Source;
};

// Copy Texture RenderCommand
struct SRHICopyTextureRegionRenderCommand : public SRHIRenderCommand
{
    SRHICopyTextureRegionRenderCommand(CRHITexture* InDestination, CRHITexture* InSource, const SCopyTextureInfo& InCopyTextureInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyTextureInfo(InCopyTextureInfo)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.CopyTextureRegion(Destination.Get(), Source.Get(), CopyTextureInfo);
    }

    TSharedRef<CRHITexture> Destination;
    TSharedRef<CRHITexture> Source;
    SCopyTextureInfo CopyTextureInfo;
};

// Destroy Resource RenderCommand
struct SRHIDestroyResourceRenderCommand : public SRHIRenderCommand
{
    SRHIDestroyResourceRenderCommand(CRHIObject* InResource)
        : Resource(InResource)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.DestroyResource(Resource.Get());
    }

    TSharedRef<CRHIObject> Resource;
};

// Discard Resource RenderCommand
struct SRHIDiscardResourceRenderCommand : public SRHIRenderCommand
{
    SRHIDiscardResourceRenderCommand(CRHIResource* InResource)
        : Resource(InResource)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.DiscardResource(Resource.Get());
    }

    TSharedRef<CRHIResource> Resource;
};

// Build RayTracing Geometry RenderCommand
struct SRHIBuildRayTracingGeometryRenderCommand : public SRHIRenderCommand
{
    SRHIBuildRayTracingGeometryRenderCommand(CRHIRayTracingGeometry* InRayTracingGeometry, CRHIVertexBuffer* InVertexBuffer, CRHIIndexBuffer* InIndexBuffer, bool bInUpdate)
        : RayTracingGeometry(InRayTracingGeometry)
        , VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
        , bUpdate(bInUpdate)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.BuildRayTracingGeometry(RayTracingGeometry.Get(), VertexBuffer.Get(), IndexBuffer.Get(), bUpdate);
    }

    TSharedRef<CRHIRayTracingGeometry> RayTracingGeometry;
    TSharedRef<CRHIVertexBuffer> VertexBuffer;
    TSharedRef<CRHIIndexBuffer>  IndexBuffer;

    bool bUpdate;
};

// Build RayTracing Scene RenderCommand
struct SRHIBuildRayTracingSceneRenderCommand : public SRHIRenderCommand
{
    SRHIBuildRayTracingSceneRenderCommand(CRHIRayTracingScene* InRayTracingScene, const SRayTracingGeometryInstance* InInstances, uint32 InNumInstances, bool bInUpdate)
        : RayTracingScene(InRayTracingScene)
        , Instances(InInstances)
        , NumInstances(InNumInstances)
        , bUpdate(bInUpdate)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.BuildRayTracingScene(RayTracingScene.Get(), Instances, NumInstances, bUpdate);
    }

    TSharedRef<CRHIRayTracingScene> RayTracingScene;
    const SRayTracingGeometryInstance* Instances;
    uint32 NumInstances;
    bool bUpdate;
};

// GenerateMips RenderCommand
struct SRHIGenerateMipsRenderCommand : public SRHIRenderCommand
{
    SRHIGenerateMipsRenderCommand(CRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.GenerateMips(Texture.Get());
    }

    TSharedRef<CRHITexture> Texture;
};

// TransitionTexture RenderCommand
struct SRHITransitionTextureRenderCommand : public SRHIRenderCommand
{
    SRHITransitionTextureRenderCommand(CRHITexture* InTexture, EResourceState InBeforeState, EResourceState InAfterState)
        : Texture(InTexture)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.TransitionTexture(Texture.Get(), BeforeState, AfterState);
    }

    TSharedRef<CRHITexture> Texture;
    EResourceState BeforeState;
    EResourceState AfterState;
};

// TransitionBuffer RenderCommand
struct SRHITransitionBufferRenderCommand : public SRHIRenderCommand
{
    SRHITransitionBufferRenderCommand(CRHIBuffer* InBuffer, EResourceState InBeforeState, EResourceState InAfterState)
        : Buffer(InBuffer)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.TransitionBuffer(Buffer.Get(), BeforeState, AfterState);
    }

    TSharedRef<CRHIBuffer> Buffer;
    EResourceState BeforeState;
    EResourceState AfterState;
};

// UnorderedAccessTextureBarrier RenderCommand
struct SRHIUnorderedAccessTextureBarrierRenderCommand : public SRHIRenderCommand
{
    SRHIUnorderedAccessTextureBarrierRenderCommand(CRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.UnorderedAccessTextureBarrier(Texture.Get());
    }

    TSharedRef<CRHITexture> Texture;
};

// UnorderedAccessBufferBarrier RenderCommand
struct SRHIUnorderedAccessBufferBarrierRenderCommand : public SRHIRenderCommand
{
    SRHIUnorderedAccessBufferBarrierRenderCommand(CRHIBuffer* InBuffer)
        : Buffer(InBuffer)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.UnorderedAccessBufferBarrier(Buffer.Get());
    }

    TSharedRef<CRHIBuffer> Buffer;
};

// Draw RenderCommand
struct SRHIDrawRenderCommand : public SRHIRenderCommand
{
    SRHIDrawRenderCommand(uint32 InVertexCount, uint32 InStartVertexLocation)
        : VertexCount(InVertexCount)
        , StartVertexLocation(InStartVertexLocation)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.Draw(VertexCount, StartVertexLocation);
    }

    uint32 VertexCount;
    uint32 StartVertexLocation;
};

// DrawIndexed RenderCommand
struct SRHIDrawIndexedRenderCommand : public SRHIRenderCommand
{
    SRHIDrawIndexedRenderCommand(uint32 InIndexCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation)
        : IndexCount(InIndexCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    uint32 IndexCount;
    uint32 StartIndexLocation;
    int32  BaseVertexLocation;
};

// DrawInstanced RenderCommand
struct SRHIDrawInstancedRenderCommand : public SRHIRenderCommand
{
    SRHIDrawInstancedRenderCommand(uint32 InVertexCountPerInstance, uint32 InInstanceCount, uint32 InStartVertexLocation, uint32 InStartInstanceLocation)
        : VertexCountPerInstance(InVertexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartVertexLocation(InStartVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    uint32 VertexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartVertexLocation;
    uint32 StartInstanceLocation;
};

// DrawIndexedInstanced RenderCommand
struct SRHIDrawIndexedInstancedRenderCommand : public SRHIRenderCommand
{
    SRHIDrawIndexedInstancedRenderCommand(uint32 InIndexCountPerInstance, uint32 InInstanceCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation, uint32 InStartInstanceLocation)
        : IndexCountPerInstance(InIndexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
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
struct SRHIDispatchComputeRenderCommand : public SRHIRenderCommand
{
    SRHIDispatchComputeRenderCommand(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
        : ThreadGroupCountX(InThreadGroupCountX)
        , ThreadGroupCountY(InThreadGroupCountY)
        , ThreadGroupCountZ(InThreadGroupCountZ)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    uint32 ThreadGroupCountX;
    uint32 ThreadGroupCountY;
    uint32 ThreadGroupCountZ;
};

// Dispatch Rays RenderCommand
struct SRHIDispatchRaysRenderCommand : public SRHIRenderCommand
{
    SRHIDispatchRaysRenderCommand(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
        : Scene(InScene)
        , PipelineState(InPipelineState)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.DispatchRays(Scene.Get(), PipelineState.Get(), Width, Height, Depth);
    }

    TSharedRef<CRHIRayTracingScene>         Scene;
    TSharedRef<CRHIRayTracingPipelineState> PipelineState;
    uint32 Width;
    uint32 Height;
    uint32 Depth;
};

// InsertCommandListMarker RenderCommand
struct SRHIInsertCommandListMarkerRenderCommand : public SRHIRenderCommand
{
    SRHIInsertCommandListMarkerRenderCommand(const CString& InMarker)
        : Marker(InMarker)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CDebug::OutputDebugString(Marker + '\n');
        LOG_INFO(Marker);

        CmdContext.InsertMarker(Marker);
    }

    CString Marker;
};

// DebugBreak RenderCommand
struct SRHIDebugBreakRenderCommand : public SRHIRenderCommand
{
    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        UNREFERENCED_VARIABLE(CmdContext);
        CDebug::DebugBreak();
    }
};

// BeginExternalCapture RenderCommand
struct SRHIBeginExternalCaptureRenderCommand : public SRHIRenderCommand
{
    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.BeginExternalCapture();
    }
};

// EndExternalCapture RenderCommand
struct SRHIEndExternalCaptureRenderCommand : public SRHIRenderCommand
{
    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.EndExternalCapture();
    }
};