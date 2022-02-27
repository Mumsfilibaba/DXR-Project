#pragma once
#include "RHITypes.h"
#include "IRHICommandContext.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"

#include "Core/Debug/Debug.h"
#include "Core/Memory/Memory.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/ArrayView.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommand

// Base render command
class CRHICommand
{
public:
    virtual ~CRHICommand() = default;

    virtual void Execute(IRHICommandContext&) = 0;

    FORCEINLINE void operator()(IRHICommandContext& CmdContext)
    {
        Execute(CmdContext);
    }

    CRHICommand* NextCmd = nullptr;
};

// BeginTimeStamp RHICommand
class CRHIBeginTimeStampCommand : public CRHICommand
{
public:
    CRHIBeginTimeStampCommand(CRHITimestampQuery* InProfiler, uint32 InIndex)
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

// EndTimeStamp RHICommand
class CRHIEndTimeStampCommand : public CRHICommand
{
public:
    CRHIEndTimeStampCommand(CRHITimestampQuery* InProfiler, uint32 InIndex)
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

// Clear RenderTarget RHICommand
class CRHIClearRenderTargetViewCommand : public CRHICommand
{
public:
    CRHIClearRenderTargetViewCommand(CRHIRenderTargetView* InRenderTargetView, const SColorF& InClearColor)
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

// Clear DepthStencil RHICommand
class CRHIClearDepthStencilViewCommand : public CRHICommand
{
public:
    CRHIClearDepthStencilViewCommand(CRHIDepthStencilView* InDepthStencilView, const SDepthStencil& InClearValue)
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

// Clear UnorderedAccessView RHICommand
class CRHIClearUnorderedAccessViewFloatCommand : public CRHICommand
{
public:
    CRHIClearUnorderedAccessViewFloatCommand(CRHIUnorderedAccessView* InUnorderedAccessView, const SColorF& InClearColor)
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

// SetShadingRate RHICommand
class CRHISetShadingRateCommand : public CRHICommand
{
public:
    CRHISetShadingRateCommand(ERHIShadingRate ShadingRate)
        : ShadingRate(ShadingRate)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetShadingRate(ShadingRate);
    }

    ERHIShadingRate ShadingRate;
};

// SetShadingRateImage RHICommand
class CRHISetShadingRateImageCommand : public CRHICommand
{
public:
    CRHISetShadingRateImageCommand(CRHITexture2D* InShadingImage)
        : ShadingImage(InShadingImage)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetShadingRateImage(ShadingImage.Get());
    }

    TSharedRef<CRHITexture2D> ShadingImage;
};

// Set Viewport RHICommand
class CRHISetViewportCommand : public CRHICommand
{
public:
    CRHISetViewportCommand(float InWidth, float InHeight, float InMinDepth, float InMaxDepth, float InX, float InY)
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

// Set ScissorRect RHICommand
class CRHISetScissorRectCommand : public CRHICommand
{
public:
    CRHISetScissorRectCommand(float InWidth, float InHeight, float InX, float InY)
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

// Set BlendFactor RHICommand
class CRHISetBlendFactorCommand : public CRHICommand
{
public:
    CRHISetBlendFactorCommand(const SColorF& InColor)
        : Color(InColor)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetBlendFactor(Color);
    }

    SColorF Color;
};

// BeginRenderPass RHICommand
class CRHIBeginRenderPassCommand : public CRHICommand
{
public:
    CRHIBeginRenderPassCommand()
    {
        // Empty for now
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.BeginRenderPass();
    }
};

// End RHICommand
class CRHIEndRenderPassCommand : public CRHICommand
{
public:
    CRHIEndRenderPassCommand()
    {
        // Empty for now
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.EndRenderPass();
    }
};

// Bind PrimitiveTopology RHICommand
class CRHISetPrimitiveTopologyCommand : public CRHICommand
{
public:
    CRHISetPrimitiveTopologyCommand(EPrimitiveTopology InPrimitiveTopologyType)
        : PrimitiveTopologyType(InPrimitiveTopologyType)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetPrimitiveTopology(PrimitiveTopologyType);
    }

    EPrimitiveTopology PrimitiveTopologyType;
};

// Set VertexBuffers RHICommand
class CRHISetVertexBuffersCommand : public CRHICommand
{
public:
    CRHISetVertexBuffersCommand(CRHIVertexBuffer** InVertexBuffers, uint32 InVertexBufferCount, uint32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    {
    }

    ~CRHISetVertexBuffersCommand()
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

// Set IndexBuffer RHICommand
class CRHISetIndexBufferCommand : public CRHICommand
{
public:
    CRHISetIndexBufferCommand(const TSharedRef<CRHIIndexBuffer>& InIndexBuffer)
        : IndexBuffer(InIndexBuffer)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetIndexBuffer(IndexBuffer.Get());
    }

    TSharedRef<CRHIIndexBuffer> IndexBuffer;
};

// Set RenderTargets RHICommand
class CRHISetRenderTargetsCommand : public CRHICommand
{
public:
    CRHISetRenderTargetsCommand(CRHIRenderTargetView** InRenderTargetViews, uint32 InRenderTargetViewCount, const TSharedRef<CRHIDepthStencilView>& InDepthStencilView)
        : RenderTargetViews(InRenderTargetViews)
        , RenderTargetViewCount(InRenderTargetViewCount)
        , DepthStencilView(InDepthStencilView)
    {
    }

    ~CRHISetRenderTargetsCommand()
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

// SetRayTracingBindings RHICommand
class CRHISetRayTracingBindingsCommand : public CRHICommand
{
public:
    CRHISetRayTracingBindingsCommand(
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

// Set GraphicsPipelineState RHICommand
class CRHISetGraphicsPipelineStateCommand : public CRHICommand
{
public:
    CRHISetGraphicsPipelineStateCommand(CRHIGraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetGraphicsPipelineState(PipelineState.Get());
    }

    TSharedRef<CRHIGraphicsPipelineState> PipelineState;
};

// Bind ComputePipelineState RHICommand
class CRHISetComputePipelineStateCommand : public CRHICommand
{
public:
    CRHISetComputePipelineStateCommand(CRHIComputePipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.SetComputePipelineState(PipelineState.Get());
    }

    TSharedRef<CRHIComputePipelineState> PipelineState;
};

// Set UseShaderResourceViews RHICommand
class CRHISet32BitShaderConstantsCommand : public CRHICommand
{
public:
    CRHISet32BitShaderConstantsCommand(CRHIShader* InShader, const void* InShader32BitConstants, uint32 InNum32BitConstants)
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

// Set ShaderResourceViewCommand
class CRHISetShaderResourceViewCommand : public CRHICommand
{
public:
    CRHISetShaderResourceViewCommand(CRHIShader* InShader, CRHIShaderResourceView* InShaderResourceView, uint32 InParameterIndex)
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

// Set ShaderResourceViewsCommand
class CRHISetShaderResourceViewsCommand : public CRHICommand
{
public:
    CRHISetShaderResourceViewsCommand(CRHIShader* InShader, CRHIShaderResourceView** InShaderResourceViews, uint32 InNumShaderResourceViews, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceViews(InShaderResourceViews)
        , NumShaderResourceViews(InNumShaderResourceViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~CRHISetShaderResourceViewsCommand()
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

// Set UnorderedAccessViewCommand
class CRHISetUnorderedAccessViewCommand : public CRHICommand
{
public:
    CRHISetUnorderedAccessViewCommand(CRHIShader* InShader, CRHIUnorderedAccessView* InUnorderedAccessView, uint32 InParameterIndex)
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

// Set UnorderedAccessViewsCommand
class CRHISetUnorderedAccessViewsCommand : public CRHICommand
{
public:
    CRHISetUnorderedAccessViewsCommand(CRHIShader* InShader, CRHIUnorderedAccessView** InUnorderedAccessViews, uint32 InNumUnorderedAccessViews, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , NumUnorderedAccessViews(InNumUnorderedAccessViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~CRHISetUnorderedAccessViewsCommand()
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

// Set ConstantBufferCommand
class CRHISetConstantBufferCommand : public CRHICommand
{
public:
    CRHISetConstantBufferCommand(CRHIShader* InShader, CRHIConstantBuffer* InConstantBuffer, uint32 InParameterIndex)
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

// Set ConstantBuffersCommand
class CRHISetConstantBuffersCommand : public CRHICommand
{
public:
    CRHISetConstantBuffersCommand(CRHIShader* InShader, CRHIConstantBuffer** InConstantBuffers, uint32 InNumConstantBuffers, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffers(InConstantBuffers)
        , NumConstantBuffers(InNumConstantBuffers)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~CRHISetConstantBuffersCommand()
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

// Set SamplerStateCommand
class CRHISetSamplerStateCommand : public CRHICommand
{
public:
    CRHISetSamplerStateCommand(CRHIShader* InShader, CRHISamplerState* InSamplerState, uint32 InParameterIndex)
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

// Set SamplerStatesCommand
class CRHISetSamplerStatesCommand : public CRHICommand
{
public:
    CRHISetSamplerStatesCommand(CRHIShader* InShader, CRHISamplerState** InSamplerStates, uint32 InNumSamplerStates, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , NumSamplerStates(InNumSamplerStates)
        , ParameterIndex(InParameterIndex)
    {
    }

    ~CRHISetSamplerStatesCommand()
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

// Resolve Texture RHICommand
class CRHIResolveTextureCommand : public CRHICommand
{
public:
    CRHIResolveTextureCommand(CRHITexture* InDestination, CRHITexture* InSource)
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

// Update Buffer RHICommand
class CRHIUpdateBufferCommand : public CRHICommand
{
public:
    CRHIUpdateBufferCommand(CRHIBuffer* InDestination, uint64 InDestinationOffsetInBytes, uint64 InSizeInBytes, const void* InSourceData)
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

// Update Texture2D RHICommand
class CRHIUpdateTexture2DCommand : public CRHICommand
{
public:
    CRHIUpdateTexture2DCommand(CRHITexture2D* InDestination, uint32 InWidth, uint32 InHeight, uint32 InMipLevel, const void* InSourceData)
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

// Copy Buffer RHICommand
class CRHICopyBufferCommand : public CRHICommand
{
public:
    CRHICopyBufferCommand(CRHIBuffer* InDestination, CRHIBuffer* InSource, const SRHICopyBufferInfo& InCopyBufferInfo)
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
    SRHICopyBufferInfo CopyBufferInfo;
};

// Copy Texture RHICommand
class CRHICopyTextureCommand : public CRHICommand
{
public:
    CRHICopyTextureCommand(CRHITexture* InDestination, CRHITexture* InSource)
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

// Copy Texture RHICommand
class CRHICopyTextureRegionCommand : public CRHICommand
{
public:
    CRHICopyTextureRegionCommand(CRHITexture* InDestination, CRHITexture* InSource, const SRHICopyTextureInfo& InCopyTextureInfo)
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
    SRHICopyTextureInfo CopyTextureInfo;
};

// Destroy Resource RHICommand
class CRHIDestroyResourceCommand : public CRHICommand
{
public:
    CRHIDestroyResourceCommand(CRHIObject* InResource)
        : Resource(InResource)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.DestroyResource(Resource.Get());
    }

    TSharedRef<CRHIObject> Resource;
};

// Discard Resource RHICommand
class CRHIDiscardContentsCommand : public CRHICommand
{
public:
    CRHIDiscardContentsCommand(CRHIResource* InResource)
        : Resource(InResource)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.DiscardContents(Resource.Get());
    }

    TSharedRef<CRHIResource> Resource;
};

// Build RayTracing Geometry RHICommand
class CRHIBuildRayTracingGeometryCommand : public CRHICommand
{
public:
    CRHIBuildRayTracingGeometryCommand(CRHIRayTracingGeometry* InRayTracingGeometry, CRHIVertexBuffer* InVertexBuffer, CRHIIndexBuffer* InIndexBuffer, bool bInUpdate)
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

// Build RayTracing Scene RHICommand
class CRHIBuildRayTracingSceneCommand : public CRHICommand
{
public:
    CRHIBuildRayTracingSceneCommand(CRHIRayTracingScene* InRayTracingScene, const SRayTracingGeometryInstance* InInstances, uint32 InNumInstances, bool bInUpdate)
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

// GenerateMips RHICommand
class CRHIGenerateMipsCommand : public CRHICommand
{
public:
    CRHIGenerateMipsCommand(CRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.GenerateMips(Texture.Get());
    }

    TSharedRef<CRHITexture> Texture;
};

// TransitionTexture RHICommand
class CRHITransitionTextureCommand : public CRHICommand
{
public:
    CRHITransitionTextureCommand(CRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState)
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
    ERHIResourceState BeforeState;
    ERHIResourceState AfterState;
};

// TransitionBuffer RHICommand
class CRHITransitionBufferCommand : public CRHICommand
{
public:
    CRHITransitionBufferCommand(CRHIBuffer* InBuffer, ERHIResourceState InBeforeState, ERHIResourceState InAfterState)
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
    ERHIResourceState BeforeState;
    ERHIResourceState AfterState;
};

// UnorderedAccessTextureBarrier RHICommand
class CRHIUnorderedAccessTextureBarrierCommand : public CRHICommand
{
public:
    CRHIUnorderedAccessTextureBarrierCommand(CRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.UnorderedAccessTextureBarrier(Texture.Get());
    }

    TSharedRef<CRHITexture> Texture;
};

// UnorderedAccessBufferBarrier RHICommand
class CRHIUnorderedAccessBufferBarrierCommand : public CRHICommand
{
public:
    CRHIUnorderedAccessBufferBarrierCommand(CRHIBuffer* InBuffer)
        : Buffer(InBuffer)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.UnorderedAccessBufferBarrier(Buffer.Get());
    }

    TSharedRef<CRHIBuffer> Buffer;
};

// Draw RHICommand
class CRHIDrawCommand : public CRHICommand
{
public:
    CRHIDrawCommand(uint32 InVertexCount, uint32 InStartVertexLocation)
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

// DrawIndexed RHICommand
class CRHIDrawIndexedCommand : public CRHICommand
{
public:
    CRHIDrawIndexedCommand(uint32 InIndexCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation)
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

// DrawInstanced RHICommand
class CRHIDrawInstancedCommand : public CRHICommand
{
public:
    CRHIDrawInstancedCommand(uint32 InVertexCountPerInstance, uint32 InInstanceCount, uint32 InStartVertexLocation, uint32 InStartInstanceLocation)
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

// DrawIndexedInstanced RHICommand
class CRHIDrawIndexedInstancedCommand : public CRHICommand
{
public:
    CRHIDrawIndexedInstancedCommand(uint32 InIndexCountPerInstance, uint32 InInstanceCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation, uint32 InStartInstanceLocation)
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

// Dispatch Compute RHICommand
class CRHIDispatchComputeCommand : public CRHICommand
{
public:
    CRHIDispatchComputeCommand(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
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

// Dispatch Rays RHICommand
class CRHIDispatchRaysCommand : public CRHICommand
{
public:
    CRHIDispatchRaysCommand(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
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

// InsertCommandListMarker RHICommand
class CRHIInsertCommandListMarkerCommand : public CRHICommand
{
public:
    CRHIInsertCommandListMarkerCommand(const String& InMarker)
        : Marker(InMarker)
    {
    }

    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CDebug::OutputDebugString(Marker + '\n');
        LOG_INFO(Marker);

        CmdContext.InsertMarker(Marker);
    }

    String Marker;
};

// DebugBreak RHICommand
class CRHIDebugBreakCommand : public CRHICommand
{
public:
    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        UNREFERENCED_VARIABLE(CmdContext);
        CDebug::DebugBreak();
    }
};

// BeginExternalCapture RHICommand
class CRHIBeginExternalCaptureCommand : public CRHICommand
{
public:
    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.BeginExternalCapture();
    }
};

// EndExternalCapture RHICommand
class CRHIEndExternalCaptureCommand : public CRHICommand
{
public:
    virtual void Execute(IRHICommandContext& CmdContext) override
    {
        CmdContext.EndExternalCapture();
    }
};
