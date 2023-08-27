#pragma once
#include "RHITypes.h"
#include "IRHICommandContext.h"
#include "RHIResources.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Debug.h"
#include "Core/Containers/ArrayView.h"

#define DECLARE_RHICOMMAND(RHICommandName) struct RHICommandName final : public TRHICommand<RHICommandName>

class FRHICommandList;

struct FRHICommand
{
    virtual ~FRHICommand() = default;

    virtual void ExecuteAndRelease(IRHICommandContext& CommandContext) = 0;

    FRHICommand* NextCommand = nullptr;
};

template<typename CommandType>
struct TRHICommand : public FRHICommand
{
    TRHICommand() = default;
    ~TRHICommand() = default;

    virtual void ExecuteAndRelease(IRHICommandContext& CommandContext) override final
    {
        CommandType* Command = static_cast<CommandType*>(this);
        Command->Execute(CommandContext);
        Command->~CommandType();
    }
};

template<typename LambdaType>
struct TRHICommandExecuteLambda : public TRHICommand<TRHICommandExecuteLambda<LambdaType>>
{
    FORCEINLINE TRHICommandExecuteLambda(LambdaType InLambda)
        : Lambda(InLambda)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext&)
    {
        Lambda();
    }

    LambdaType Lambda;
};

DECLARE_RHICOMMAND(FRHICommandExecuteCommandList)
{
    FORCEINLINE FRHICommandExecuteCommandList(FRHICommandList* InCommandList)
        : CommandList(InCommandList)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext);

    FRHICommandList* CommandList;
};

DECLARE_RHICOMMAND(FRHICommandBeginTimeStamp)
{
    FORCEINLINE FRHICommandBeginTimeStamp(FRHITimestampQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIBeginTimeStamp(Query, Index);
    }

    FRHITimestampQuery* Query;
    uint32              Index;
};

DECLARE_RHICOMMAND(FRHICommandEndTimeStamp)
{
    FORCEINLINE FRHICommandEndTimeStamp(FRHITimestampQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIEndTimeStamp(Query, Index);
    }

    FRHITimestampQuery* Query;
    uint32              Index;
};

DECLARE_RHICOMMAND(FRHICommandClearRenderTargetView)
{
    FORCEINLINE FRHICommandClearRenderTargetView(const FRHIRenderTargetView& InRenderTargetView, const FVector4& InClearColor)
        : RenderTargetView(InRenderTargetView)
        , ClearColor(InClearColor)
    {
        CHECK(RenderTargetView.Texture != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIClearRenderTargetView(RenderTargetView, ClearColor);
    }

    FRHIRenderTargetView RenderTargetView;
    FVector4             ClearColor;
};

DECLARE_RHICOMMAND(FRHICommandClearDepthStencilView)
{
    FORCEINLINE FRHICommandClearDepthStencilView(const FRHIDepthStencilView& InDepthStencilView, const float InDepth, const uint8 InStencil)
        : DepthStencilView(InDepthStencilView)
        , Depth(InDepth)
        , Stencil(InStencil)
    {
        CHECK(DepthStencilView.Texture != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIClearDepthStencilView(DepthStencilView, Depth, Stencil);
    }

    FRHIDepthStencilView DepthStencilView;
    const float          Depth;
    const uint8          Stencil;
};

DECLARE_RHICOMMAND(FRHICommandClearUnorderedAccessViewFloat)
{
    FORCEINLINE FRHICommandClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* InUnorderedAccessView, const FVector4& InClearColor)
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor(InClearColor)
    {
        CHECK(InUnorderedAccessView != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
    }

    FRHIUnorderedAccessView* UnorderedAccessView;
    FVector4                 ClearColor;
};

DECLARE_RHICOMMAND(FRHICommandBeginRenderPass)
{
    FRHICommandBeginRenderPass(const FRHIRenderPassDesc& InRenderPassDesc)
        : RenderPassDesc(InRenderPassDesc)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIBeginRenderPass(RenderPassDesc);
    }

    FRHIRenderPassDesc RenderPassDesc;
};

DECLARE_RHICOMMAND(FRHICommandEndRenderPass)
{
    FRHICommandEndRenderPass() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIEndRenderPass();
    }
};

DECLARE_RHICOMMAND(FRHICommandSetViewport)
{
    FORCEINLINE FRHICommandSetViewport(const FRHIViewportRegion& InViewportRegion)
        : ViewportRegion(InViewportRegion)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetViewport(ViewportRegion);
    }

    FRHIViewportRegion ViewportRegion;
};

DECLARE_RHICOMMAND(FRHICommandSetScissorRect)
{
    FORCEINLINE FRHICommandSetScissorRect(const FRHIScissorRegion& InScissorRegion)
        : ScissorRegion(InScissorRegion)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetScissorRect(ScissorRegion);
    }

    FRHIScissorRegion ScissorRegion;
};

DECLARE_RHICOMMAND(FRHICommandSetBlendFactor)
{
    FORCEINLINE FRHICommandSetBlendFactor(const FVector4& InColor)
        : Color(InColor)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetBlendFactor(Color);
    }

    FVector4 Color;
};

DECLARE_RHICOMMAND(FRHICommandSetVertexBuffers)
{
    FORCEINLINE FRHICommandSetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , StartSlot(InStartSlot)
    { 
        for (FRHIBuffer* const Buffer : VertexBuffers)
        {
            if (Buffer)
            {
                CHECK(Buffer->GetDesc().IsVertexBuffer());
            }
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetVertexBuffers(VertexBuffers, StartSlot);
    }

    TArrayView<FRHIBuffer* const> VertexBuffers;
    uint32                        StartSlot;
};

DECLARE_RHICOMMAND(FRHICommandSetIndexBuffer)
{
    FORCEINLINE FRHICommandSetIndexBuffer(FRHIBuffer* InIndexBuffer, EIndexFormat InIndexFormat)
        : IndexBuffer(InIndexBuffer)
        , IndexFormat(InIndexFormat)
    { 
        if (InIndexBuffer)
        {
            CHECK(InIndexBuffer->GetDesc().IsIndexBuffer());
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetIndexBuffer(IndexBuffer, IndexFormat);
    }

    FRHIBuffer*  IndexBuffer;
    EIndexFormat IndexFormat;
};

DECLARE_RHICOMMAND(FRHICommandSetPrimitiveTopology)
{
    FORCEINLINE FRHICommandSetPrimitiveTopology(EPrimitiveTopology InPrimitiveTopologyType)
        : PrimitiveTopologyType(InPrimitiveTopologyType)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetPrimitiveTopology(PrimitiveTopologyType);
    }

    EPrimitiveTopology PrimitiveTopologyType;
};

DECLARE_RHICOMMAND(FRHICommandSetGraphicsPipelineState)
{
    FORCEINLINE FRHICommandSetGraphicsPipelineState(FRHIGraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetGraphicsPipelineState(PipelineState);
    }

    FRHIGraphicsPipelineState* PipelineState;
};

DECLARE_RHICOMMAND(FRHICommandSetComputePipelineState)
{
    FORCEINLINE FRHICommandSetComputePipelineState(FRHIComputePipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetComputePipelineState(PipelineState);
    }

    FRHIComputePipelineState* PipelineState;
};

DECLARE_RHICOMMAND(FRHICommandSet32BitShaderConstants)
{
    FORCEINLINE FRHICommandSet32BitShaderConstants(FRHIShader* InShader, const void* InShader32BitConstants, uint32 InNum32BitConstants)
        : Shader(InShader)
        , Shader32BitConstants(InShader32BitConstants)
        , Num32BitConstants(InNum32BitConstants)
    { 
        CHECK(InNum32BitConstants <= FRHILimits::MaxShaderConstants);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISet32BitShaderConstants(Shader, Shader32BitConstants, Num32BitConstants);
    }

    FRHIShader* Shader;
    const void* Shader32BitConstants;
    uint32      Num32BitConstants;
};

DECLARE_RHICOMMAND(FRHICommandSetShaderResourceView)
{
    FORCEINLINE FRHICommandSetShaderResourceView(FRHIShader* InShader, FRHIShaderResourceView* InShaderResourceView, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceView(InShaderResourceView)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetShaderResourceView(Shader, ShaderResourceView, ParameterIndex);
    }

    FRHIShader*             Shader;
    FRHIShaderResourceView* ShaderResourceView;
    uint32                  ParameterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetShaderResourceViews)
{
    FORCEINLINE FRHICommandSetShaderResourceViews(FRHIShader* InShader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 InStartParameterIndex)
        : Shader(InShader)
        , ShaderResourceViews(InShaderResourceViews)
        , StartParameterIndex(InStartParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetShaderResourceViews(Shader, ShaderResourceViews, StartParameterIndex);
    }

    FRHIShader*                               Shader;
    TArrayView<FRHIShaderResourceView* const> ShaderResourceViews;
    uint32                                    StartParameterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetUnorderedAccessView)
{
    FORCEINLINE FRHICommandSetUnorderedAccessView(FRHIShader* InShader, FRHIUnorderedAccessView* InUnorderedAccessView, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessView(InUnorderedAccessView)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetUnorderedAccessView(Shader, UnorderedAccessView, ParameterIndex);
    }

    FRHIShader*              Shader;
    FRHIUnorderedAccessView* UnorderedAccessView;
    uint32                   ParameterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetUnorderedAccessViews)
{
    FORCEINLINE FRHICommandSetUnorderedAccessViews(FRHIShader* InShader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 InStartParameterIndex)
        : Shader(InShader)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , StartParameterIndex(InStartParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetUnorderedAccessViews(Shader, UnorderedAccessViews, StartParameterIndex);
    }

    FRHIShader*                                Shader;
    TArrayView<FRHIUnorderedAccessView* const> UnorderedAccessViews;
    uint32                                     StartParameterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetConstantBuffer)
{
    FORCEINLINE FRHICommandSetConstantBuffer(FRHIShader* InShader, FRHIBuffer* InConstantBuffer, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffer(InConstantBuffer)
        , ParameterIndex(InParameterIndex)
    {
        if (ConstantBuffer)
        {
            CHECK(ConstantBuffer->GetDesc().IsConstantBuffer());
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetConstantBuffer(Shader, ConstantBuffer, ParameterIndex);
    }

    FRHIShader* Shader;
    FRHIBuffer* ConstantBuffer;
    uint32      ParameterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetConstantBuffers)
{
    FORCEINLINE FRHICommandSetConstantBuffers(FRHIShader* InShader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 InStartParameterIndex)
        : Shader(InShader)
        , ConstantBuffers(InConstantBuffers)
        , StartParameterIndex(InStartParameterIndex)
    { 
        for (FRHIBuffer* const Buffer : ConstantBuffers)
        {
            if (Buffer)
            {
                CHECK(Buffer->GetDesc().IsConstantBuffer());
            }
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetConstantBuffers(Shader, ConstantBuffers, StartParameterIndex);
    }

    FRHIShader*                   Shader;
    TArrayView<FRHIBuffer* const> ConstantBuffers;
    uint32                        StartParameterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetSamplerState)
{
    FORCEINLINE FRHICommandSetSamplerState(FRHIShader* InShader, FRHISamplerState* InSamplerState, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerState(InSamplerState)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetSamplerState(Shader, SamplerState, ParameterIndex);
    }

    FRHIShader*       Shader;
    FRHISamplerState* SamplerState;
    uint32            ParameterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetSamplerStates)
{
    FORCEINLINE FRHICommandSetSamplerStates(FRHIShader* InShader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 InStartParameterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , StartParameterIndex(InStartParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetSamplerStates(Shader, SamplerStates, StartParameterIndex);
    }

    FRHIShader*                         Shader;
    TArrayView<FRHISamplerState* const> SamplerStates;
    uint32                              StartParameterIndex;
};

DECLARE_RHICOMMAND(FRHICommandUpdateBuffer)
{
    FORCEINLINE FRHICommandUpdateBuffer(FRHIBuffer* InDst, const FBufferRegion& InBufferRegion, const void* InSrcData)
        : Dst(InDst)
        , SrcData(InSrcData)
        , BufferRegion(InBufferRegion)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIUpdateBuffer(Dst, BufferRegion, SrcData);
    }

    FRHIBuffer*   Dst;
    const void*   SrcData;
    FBufferRegion BufferRegion;
};

DECLARE_RHICOMMAND(FRHICommandUpdateTexture2D)
{
    FORCEINLINE FRHICommandUpdateTexture2D(
        FRHITexture*            InDst,
        const FTextureRegion2D& InTextureRegion,
        uint32                  InMipLevel,
        const void*             InSrcData,
        uint32                  InSrcRowPitch)
        : Dst(InDst)
        , TextureRegion(InTextureRegion)
        , MipLevel(InMipLevel)
        , SrcData(InSrcData)
        , SrcRowPitch(InSrcRowPitch)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIUpdateTexture2D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
    }

    FRHITexture*     Dst;
    FTextureRegion2D TextureRegion;
    uint32           MipLevel;
    const void*      SrcData;
    uint32           SrcRowPitch;
};

DECLARE_RHICOMMAND(FRHICommandResolveTexture)
{
    FORCEINLINE FRHICommandResolveTexture(FRHITexture* InDst, FRHITexture* InSrc)
        : Dst(InDst)
        , Src(InSrc)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIResolveTexture(Dst, Src);
    }

    FRHITexture* Dst;
    FRHITexture* Src;
};

DECLARE_RHICOMMAND(FRHICommandCopyBuffer)
{
    FORCEINLINE FRHICommandCopyBuffer(FRHIBuffer* InDst, FRHIBuffer* InSrc, const FRHIBufferCopyDesc& InCopyBufferInfo)
        : Dst(InDst)
        , Src(InSrc)
        , CopyBufferInfo(InCopyBufferInfo)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHICopyBuffer(Dst, Src, CopyBufferInfo);
    }

    FRHIBuffer*        Dst;
    FRHIBuffer*        Src;
    FRHIBufferCopyDesc CopyBufferInfo;
};

DECLARE_RHICOMMAND(FRHICommandCopyTexture)
{
    FORCEINLINE FRHICommandCopyTexture(FRHITexture* InDestination, FRHITexture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHICopyTexture(Destination, Source);
    }

    FRHITexture* Destination;
    FRHITexture* Source;
};

DECLARE_RHICOMMAND(FRHICommandCopyTextureRegion)
{
    FORCEINLINE FRHICommandCopyTextureRegion(FRHITexture* InDst, FRHITexture* InSrc, const FRHITextureCopyDesc& InCopyInfo)
        : Dst(InDst)
        , Src(InSrc)
        , CopyInfo(InCopyInfo)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHICopyTextureRegion(Dst, Src, CopyInfo);
    }

    FRHITexture*        Dst;
    FRHITexture*        Src;
    FRHITextureCopyDesc CopyInfo;
};

DECLARE_RHICOMMAND(FRHICommandDestroyResource)
{
    FORCEINLINE FRHICommandDestroyResource(IRefCounted* InResource)
        : Resource(MakeSharedRef<IRefCounted>(InResource))
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIDestroyResource(Resource.Get());
    }

    TSharedRef<IRefCounted> Resource;
};

DECLARE_RHICOMMAND(FRHICommandDiscardContents)
{
    FORCEINLINE FRHICommandDiscardContents(FRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIDiscardContents(Texture);
    }

    FRHITexture* Texture;
};

DECLARE_RHICOMMAND(FRHICommandBuildRayTracingGeometry)
{
    FORCEINLINE FRHICommandBuildRayTracingGeometry(
        FRHIRayTracingGeometry* InRayTracingGeometry,
        FRHIBuffer*             InVertexBuffer,
        uint32                  InNumVertices,
        FRHIBuffer*             InIndexBuffer,
        uint32                  InNumIndices,
        EIndexFormat            InIndexFormat,
        bool                    bInUpdate)
        : RayTracingGeometry(InRayTracingGeometry)
        , VertexBuffer(InVertexBuffer)
        , NumVertices(InNumVertices)
        , IndexBuffer(InIndexBuffer)
        , NumIndices(InNumIndices)
        , IndexFormat(InIndexFormat)
        , bUpdate(bInUpdate)
    { 
        CHECK(VertexBuffer && VertexBuffer->GetDesc().IsVertexBuffer());
        CHECK(IndexBuffer  && IndexBuffer->GetDesc().IsIndexBuffer());
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIBuildRayTracingGeometry(RayTracingGeometry, VertexBuffer, NumVertices, IndexBuffer, NumIndices, IndexFormat, bUpdate);
    }

    FRHIRayTracingGeometry* RayTracingGeometry;
    FRHIBuffer*             VertexBuffer;
    uint32                  NumVertices;
    FRHIBuffer*             IndexBuffer;
    uint32                  NumIndices;
    EIndexFormat            IndexFormat;
    bool                    bUpdate;
};

DECLARE_RHICOMMAND(FRHICommandBuildRayTracingScene)
{
    FORCEINLINE FRHICommandBuildRayTracingScene(
        FRHIRayTracingScene* InScene,
        const TArrayView<const FRHIRayTracingGeometryInstance>& InInstances,
        bool bInUpdate)
        : Scene(InScene)
        , Instances(InInstances)
        , bUpdate(bInUpdate)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIBuildRayTracingScene(Scene, Instances, bUpdate);
    }

    FRHIRayTracingScene*                             Scene;
    TArrayView<const FRHIRayTracingGeometryInstance> Instances;
    bool                                             bUpdate;
};

DECLARE_RHICOMMAND(FRHICommandSetRayTracingBindings)
{
    FORCEINLINE FRHICommandSetRayTracingBindings(
        FRHIRayTracingScene*              InRayTracingScene,
        FRHIRayTracingPipelineState*      InPipelineState,
        const FRayTracingShaderResources* InGlobalResource,
        const FRayTracingShaderResources* InRayGenLocalResources,
        const FRayTracingShaderResources* InMissLocalResources,
        const FRayTracingShaderResources* InHitGroupResources,
        uint32                            InNumHitGroupResources)
        : RayTracingScene(InRayTracingScene)
        , PipelineState(InPipelineState)
        , GlobalResource(InGlobalResource)
        , RayGenLocalResources(InRayGenLocalResources)
        , MissLocalResources(InMissLocalResources)
        , HitGroupResources(InHitGroupResources)
        , NumHitGroupResources(InNumHitGroupResources)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHISetRayTracingBindings(RayTracingScene, PipelineState, GlobalResource, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
    }

    FRHIRayTracingScene*              RayTracingScene;
    FRHIRayTracingPipelineState*      PipelineState;
    const FRayTracingShaderResources* GlobalResource;
    const FRayTracingShaderResources* RayGenLocalResources;
    const FRayTracingShaderResources* MissLocalResources;
    const FRayTracingShaderResources* HitGroupResources;
    uint32                            NumHitGroupResources;
};

DECLARE_RHICOMMAND(FRHICommandGenerateMips)
{
    FORCEINLINE FRHICommandGenerateMips(FRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIGenerateMips(Texture);
    }

    FRHITexture* Texture;
};

DECLARE_RHICOMMAND(FRHICommandTransitionTexture)
{
    FORCEINLINE FRHICommandTransitionTexture(FRHITexture* InTexture, EResourceAccess InBeforeState, EResourceAccess InAfterState)
        : Texture(InTexture)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHITransitionTexture(Texture, BeforeState, AfterState);
    }

    FRHITexture*    Texture;
    EResourceAccess BeforeState;
    EResourceAccess AfterState;
};

DECLARE_RHICOMMAND(FRHICommandTransitionBuffer)
{
    FORCEINLINE FRHICommandTransitionBuffer(FRHIBuffer* InBuffer, EResourceAccess InBeforeState, EResourceAccess InAfterState)
        : Buffer(InBuffer)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHITransitionBuffer(Buffer, BeforeState, AfterState);
    }

    FRHIBuffer*     Buffer;
    EResourceAccess BeforeState;
    EResourceAccess AfterState;
};

DECLARE_RHICOMMAND(FRHICommandUnorderedAccessTextureBarrier)
{
    FORCEINLINE FRHICommandUnorderedAccessTextureBarrier(FRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIUnorderedAccessTextureBarrier(Texture);
    }

    FRHITexture* Texture;
};

DECLARE_RHICOMMAND(FRHICommandUnorderedAccessBufferBarrier)
{
    FORCEINLINE FRHICommandUnorderedAccessBufferBarrier(FRHIBuffer* InBuffer)
        : Buffer(InBuffer)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIUnorderedAccessBufferBarrier(Buffer);
    }

    FRHIBuffer* Buffer;
};

DECLARE_RHICOMMAND(FRHICommandDraw)
{
    FORCEINLINE FRHICommandDraw(uint32 InVertexCount, uint32 InStartVertexLocation)
        : VertexCount(InVertexCount)
        , StartVertexLocation(InStartVertexLocation)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIDraw(VertexCount, StartVertexLocation);
    }

    uint32 VertexCount;
    uint32 StartVertexLocation;
};

DECLARE_RHICOMMAND(FRHICommandDrawIndexed)
{
    FORCEINLINE FRHICommandDrawIndexed(uint32 InIndexCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation)
        : IndexCount(InIndexCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIDrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    uint32 IndexCount;
    uint32 StartIndexLocation;
    uint32 BaseVertexLocation;
};

DECLARE_RHICOMMAND(FRHICommandDrawInstanced)
{
    FORCEINLINE FRHICommandDrawInstanced(uint32 InVertexCountPerInstance, uint32 InInstanceCount, uint32 InStartVertexLocation, uint32 InStartInstanceLocation)
        : VertexCountPerInstance(InVertexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartVertexLocation(InStartVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIDrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    uint32 VertexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartVertexLocation;
    uint32 StartInstanceLocation;
};

DECLARE_RHICOMMAND(FRHICommandDrawIndexedInstanced)
{
    FORCEINLINE FRHICommandDrawIndexedInstanced(
        uint32 InIndexCountPerInstance,
        uint32 InInstanceCount,
        uint32 InStartIndexLocation,
        uint32 InBaseVertexLocation,
        uint32 InStartInstanceLocation)
        : IndexCountPerInstance(InIndexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIDrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    uint32 IndexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartIndexLocation;
    uint32 BaseVertexLocation;
    uint32 StartInstanceLocation;
};

DECLARE_RHICOMMAND(FRHICommandDispatch)
{
    FORCEINLINE FRHICommandDispatch(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
        : ThreadGroupCountX(InThreadGroupCountX)
        , ThreadGroupCountY(InThreadGroupCountY)
        , ThreadGroupCountZ(InThreadGroupCountZ)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIDispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    uint32 ThreadGroupCountX;
    uint32 ThreadGroupCountY;
    uint32 ThreadGroupCountZ;
};

DECLARE_RHICOMMAND(FRHICommandDispatchRays)
{
    FORCEINLINE FRHICommandDispatchRays(
        FRHIRayTracingScene*         InScene,
        FRHIRayTracingPipelineState* InPipelineState,
        uint32                       InWidth,
        uint32                       InHeight,
        uint32                       InDepth)
        : Scene(InScene)
        , PipelineState(InPipelineState)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIDispatchRays(Scene, PipelineState, Width, Height, Depth);
    }

    FRHIRayTracingScene*         Scene;
    FRHIRayTracingPipelineState* PipelineState;
    uint32                       Width;
    uint32                       Height;
    uint32                       Depth;
};

DECLARE_RHICOMMAND(FRHICommandInsertMarker)
{
    FORCEINLINE FRHICommandInsertMarker(const FStringView& InMarker)
        : Marker(InMarker)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        if (FDebug::IsDebuggerPresent())
        {
            FDebug::OutputDebugString(FString(Marker) + '\n');
        }

        LOG_INFO("%s", Marker.GetCString());
        CommandContext.RHIInsertMarker(Marker);
    }

    FStringView Marker;
};

DECLARE_RHICOMMAND(FRHICommandDebugBreak)
{
    FRHICommandDebugBreak() = default;

    FORCEINLINE void Execute(IRHICommandContext&)
    {
        if (FDebug::IsDebuggerPresent())
        {
            DEBUG_BREAK();
        }
    }
};

DECLARE_RHICOMMAND(FRHICommandBeginExternalCapture)
{
    FRHICommandBeginExternalCapture() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIBeginExternalCapture();
    }
};

DECLARE_RHICOMMAND(FRHICommandEndExternalCapture)
{
    FRHICommandEndExternalCapture() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIEndExternalCapture();
    }
};

DECLARE_RHICOMMAND(FRHICommandPresentViewport)
{
    FORCEINLINE FRHICommandPresentViewport(FRHIViewport* InViewport, bool bInVerticalSync)
        : Viewport(InViewport)
        , bVerticalSync(bInVerticalSync)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.RHIPresentViewport(Viewport, bVerticalSync);
    }

    FRHIViewport* Viewport;
    bool          bVerticalSync;
};