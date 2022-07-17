#pragma once
#include "RHITypes.h"
#include "IRHICommandContext.h"
#include "RHIResources.h"

#include "Core/Debug/Debug.h"
#include "Core/Memory/Memory.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/ArrayView.h"

#define DECLARE_RHICOMMAND(RHICommandName) struct RHICommandName final : public TRHICommand<RHICommandName>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommand

struct FRHICommand
{
    virtual ~FRHICommand() = default;

    virtual void ExecuteAndRelease(IRHICommandContext& CommandContext) = 0;

    FRHICommand* NextCommand = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TRHICommand

template<typename CommandType>
struct TRHICommand : public FRHICommand
{
    TRHICommand()  = default;
    ~TRHICommand() = default;

    virtual void ExecuteAndRelease(IRHICommandContext& CommandContext) override final
    {
        CommandType* Command = static_cast<CommandType*>(this);
        Command->Execute(CommandContext);
        Command->~CommandType();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandBeginTimeStamp

DECLARE_RHICOMMAND(FRHICommandBeginTimeStamp)
{
    FORCEINLINE FRHICommandBeginTimeStamp(FRHITimestampQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginTimeStamp(Query, Index);
    }

    FRHITimestampQuery* Query;
    uint32              Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandEndTimeStamp

DECLARE_RHICOMMAND(FRHICommandEndTimeStamp)
{
    FORCEINLINE FRHICommandEndTimeStamp(FRHITimestampQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndTimeStamp(Query, Index);
    }

    FRHITimestampQuery* Query;
    uint32              Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandClearRenderTargetView

DECLARE_RHICOMMAND(FRHICommandClearRenderTargetView)
{
    FORCEINLINE FRHICommandClearRenderTargetView(const FRHIRenderTargetView& InRenderTargetView, const TStaticArray<float, 4>& InClearColor)
        : RenderTargetView(InRenderTargetView)
        , ClearColor(InClearColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearRenderTargetView(RenderTargetView, ClearColor);
    }

    FRHIRenderTargetView   RenderTargetView;
    TStaticArray<float, 4> ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandClearDepthStencilView

DECLARE_RHICOMMAND(FRHICommandClearDepthStencilView)
{
    FORCEINLINE FRHICommandClearDepthStencilView(const FRHIDepthStencilView& InDepthStencilView, const float InDepth, const uint8 InStencil)
        : DepthStencilView(InDepthStencilView)
        , Depth(InDepth)
        , Stencil(InStencil)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearDepthStencilView(DepthStencilView, Depth, Stencil);
    }

    FRHIDepthStencilView DepthStencilView;
    const float          Depth;
    const uint8          Stencil;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandClearUnorderedAccessViewFloat

DECLARE_RHICOMMAND(FRHICommandClearUnorderedAccessViewFloat)
{
    FORCEINLINE FRHICommandClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* InUnorderedAccessView, const TStaticArray<float, 4>&InClearColor)
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor(InClearColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
    }

    FRHIUnorderedAccessView* UnorderedAccessView;
    TStaticArray<float, 4>   ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandBeginRenderPass

DECLARE_RHICOMMAND(FRHICommandBeginRenderPass)
{
    FRHICommandBeginRenderPass(const FRHIRenderPassInitializer& InRenderPassInitializer)
        : RenderPassInitializer(InRenderPassInitializer)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginRenderPass(RenderPassInitializer);
    }

    FRHIRenderPassInitializer RenderPassInitializer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandEndRenderPass

DECLARE_RHICOMMAND(FRHICommandEndRenderPass)
{
    FRHICommandEndRenderPass() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndRenderPass();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetViewport

DECLARE_RHICOMMAND(FRHICommandSetViewport)
{
    FORCEINLINE FRHICommandSetViewport(float InWidth, float InHeight, float InMinDepth, float InMaxDepth, float InX, float InY)
        : Width(InWidth)
        , Height(InHeight)
        , MinDepth(InMinDepth)
        , MaxDepth(InMaxDepth)
        , x(InX)
        , y(InY)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetViewport(Width, Height, MinDepth, MaxDepth, x, y);
    }

    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;
    float x;
    float y;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetScissorRect

DECLARE_RHICOMMAND(FRHICommandSetScissorRect)
{
    FORCEINLINE FRHICommandSetScissorRect(float InWidth, float InHeight, float InX, float InY)
        : Width(InWidth)
        , Height(InHeight)
        , x(InX)
        , y(InY)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetScissorRect(Width, Height, x, y);
    }

    float Width;
    float Height;
    float x;
    float y;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetBlendFactor

DECLARE_RHICOMMAND(FRHICommandSetBlendFactor)
{
    FORCEINLINE FRHICommandSetBlendFactor(const TStaticArray<float, 4>&InColor)
        : Color(InColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetBlendFactor(Color);
    }

    TStaticArray<float, 4> Color;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetVertexBuffers

DECLARE_RHICOMMAND(FRHICommandSetVertexBuffers)
{
    FORCEINLINE FRHICommandSetVertexBuffers(FRHIVertexBuffer* const* InVertexBuffers, uint32 InVertexBufferCount, uint32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetVertexBuffers(VertexBuffers, VertexBufferCount, StartSlot);
    }

    FRHIVertexBuffer* const* VertexBuffers;
    uint32                   VertexBufferCount;
    uint32                   StartSlot;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetIndexBuffer

DECLARE_RHICOMMAND(FRHICommandSetIndexBuffer)
{
    FORCEINLINE FRHICommandSetIndexBuffer(FRHIIndexBuffer* InIndexBuffer)
        : IndexBuffer(InIndexBuffer)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetIndexBuffer(IndexBuffer);
    }

    FRHIIndexBuffer* IndexBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetPrimitiveTopology

DECLARE_RHICOMMAND(FRHICommandSetPrimitiveTopology)
{
    FORCEINLINE FRHICommandSetPrimitiveTopology(EPrimitiveTopology InPrimitiveTopologyType)
        : PrimitiveTopologyType(InPrimitiveTopologyType)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetPrimitiveTopology(PrimitiveTopologyType);
    }

    EPrimitiveTopology PrimitiveTopologyType;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetGraphicsPipelineState

DECLARE_RHICOMMAND(FRHICommandSetGraphicsPipelineState)
{
    FORCEINLINE FRHICommandSetGraphicsPipelineState(FRHIGraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetGraphicsPipelineState(PipelineState);
    }

    FRHIGraphicsPipelineState* PipelineState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetComputePipelineState

DECLARE_RHICOMMAND(FRHICommandSetComputePipelineState)
{
    FORCEINLINE FRHICommandSetComputePipelineState(FRHIComputePipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetComputePipelineState(PipelineState);
    }

    FRHIComputePipelineState* PipelineState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSet32BitShaderConstants

DECLARE_RHICOMMAND(FRHICommandSet32BitShaderConstants)
{
    FORCEINLINE FRHICommandSet32BitShaderConstants(FRHIShader* InShader, const void* InShader32BitConstants, uint32 InNum32BitConstants)
        : Shader(InShader)
        , Shader32BitConstants(InShader32BitConstants)
        , Num32BitConstants(InNum32BitConstants)
    { 
        Check(InNum32BitConstants <= kRHIMaxShaderConstants);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Set32BitShaderConstants(Shader, Shader32BitConstants, Num32BitConstants);
    }

    FRHIShader* Shader;
    const void* Shader32BitConstants;
    uint32      Num32BitConstants;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetShaderResourceView

DECLARE_RHICOMMAND(FRHICommandSetShaderResourceView)
{
    FORCEINLINE FRHICommandSetShaderResourceView(FRHIShader* InShader, FRHIShaderResourceView* InShaderResourceView, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceView(InShaderResourceView)
        , ParameterIndex(InParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderResourceView(Shader, ShaderResourceView, ParameterIndex);
    }

    FRHIShader*             Shader;
    FRHIShaderResourceView* ShaderResourceView;
    uint32                  ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetShaderResourceViews

DECLARE_RHICOMMAND(FRHICommandSetShaderResourceViews)
{
    FORCEINLINE FRHICommandSetShaderResourceViews( FRHIShader* InShader
                                                 , FRHIShaderResourceView* const* InShaderResourceViews
                                                 , uint32 InNumShaderResourceViews
                                                 , uint32 InStartParameterIndex)
        : Shader(InShader)
        , ShaderResourceViews(InShaderResourceViews)
        , NumShaderResourceViews(InNumShaderResourceViews)
        , StartParameterIndex(InStartParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderResourceViews(Shader, ShaderResourceViews, NumShaderResourceViews, StartParameterIndex);
    }

    FRHIShader*                    Shader;
    FRHIShaderResourceView* const* ShaderResourceViews;
    uint32                         NumShaderResourceViews;
    uint32                         StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetUnorderedAccessView

DECLARE_RHICOMMAND(FRHICommandSetUnorderedAccessView)
{
    FORCEINLINE FRHICommandSetUnorderedAccessView(FRHIShader* InShader, FRHIUnorderedAccessView* InUnorderedAccessView, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessView(InUnorderedAccessView)
        , ParameterIndex(InParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetUnorderedAccessView(Shader, UnorderedAccessView, ParameterIndex);
    }

    FRHIShader*              Shader;
    FRHIUnorderedAccessView* UnorderedAccessView;
    uint32                   ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetUnorderedAccessViews

DECLARE_RHICOMMAND(FRHICommandSetUnorderedAccessViews)
{
    FORCEINLINE FRHICommandSetUnorderedAccessViews( FRHIShader* InShader
                                                  , FRHIUnorderedAccessView* const* InUnorderedAccessViews
                                                  , uint32 InNumUnorderedAccessViews
                                                  , uint32 InStartParameterIndex)
        : Shader(InShader)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , NumUnorderedAccessViews(InNumUnorderedAccessViews)
        , StartParameterIndex(InStartParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetUnorderedAccessViews(Shader, UnorderedAccessViews, NumUnorderedAccessViews, StartParameterIndex);
    }

    FRHIShader*                     Shader;
    FRHIUnorderedAccessView* const* UnorderedAccessViews;
    uint32                          NumUnorderedAccessViews;
    uint32                          StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetConstantBuffer

DECLARE_RHICOMMAND(FRHICommandSetConstantBuffer)
{
    FORCEINLINE FRHICommandSetConstantBuffer(FRHIShader* InShader, FRHIConstantBuffer* InConstantBuffer, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffer(InConstantBuffer)
        , ParameterIndex(InParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetConstantBuffer(Shader, ConstantBuffer, ParameterIndex);
    }

    FRHIShader*         Shader;
    FRHIConstantBuffer* ConstantBuffer;
    uint32              ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetConstantBuffers

DECLARE_RHICOMMAND(FRHICommandSetConstantBuffers)
{
    FORCEINLINE FRHICommandSetConstantBuffers( FRHIShader* InShader
                                             , FRHIConstantBuffer* const* InConstantBuffers
                                             , uint32 InNumConstantBuffers
                                             , uint32 InStartParameterIndex)
        : Shader(InShader)
        , ConstantBuffers(InConstantBuffers)
        , NumConstantBuffers(InNumConstantBuffers)
        , StartParameterIndex(InStartParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetConstantBuffers(Shader, ConstantBuffers, NumConstantBuffers, StartParameterIndex);
    }

    FRHIShader*                Shader;
    FRHIConstantBuffer* const* ConstantBuffers;
    uint32                     NumConstantBuffers;
    uint32                     StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetSamplerState

DECLARE_RHICOMMAND(FRHICommandSetSamplerState)
{
    FORCEINLINE FRHICommandSetSamplerState(FRHIShader* InShader, FRHISamplerState* InSamplerState, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerState(InSamplerState)
        , ParameterIndex(InParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetSamplerState(Shader, SamplerState, ParameterIndex);
    }

    FRHIShader*       Shader;
    FRHISamplerState* SamplerState;
    uint32            ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetSamplerStates

DECLARE_RHICOMMAND(FRHICommandSetSamplerStates)
{
    FORCEINLINE FRHICommandSetSamplerStates(FRHIShader* InShader, FRHISamplerState* const* InSamplerStates, uint32 InNumSamplerStates, uint32 InStartParameterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , NumSamplerStates(InNumSamplerStates)
        , StartParameterIndex(InStartParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetSamplerStates(Shader, SamplerStates, NumSamplerStates, StartParameterIndex);
    }

    FRHIShader*              Shader;
    FRHISamplerState* const* SamplerStates;
    uint32                   NumSamplerStates;
    uint32                   StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandUpdateBuffer

DECLARE_RHICOMMAND(FRHICommandUpdateBuffer)
{
    FORCEINLINE FRHICommandUpdateBuffer(FRHIBuffer* InDst, uint32 InDstOffset, uint32 InSize, const void* InSrcData)
        : Dst(InDst)
        , DstOffset(InDstOffset)
        , Size(InSize)
        , SrcData(InSrcData)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateBuffer(Dst, DstOffset, Size, SrcData);
    }

    FRHIBuffer* Dst;
    uint32      DstOffset;
    uint32      Size;
    const void* SrcData;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandUpdateTexture2D

DECLARE_RHICOMMAND(FRHICommandUpdateTexture2D)
{
    FORCEINLINE FRHICommandUpdateTexture2D(FRHITexture2D* InDst, uint16 InWidth, uint16 InHeight, uint16 InMipLevel, const void* InSrcData)
        : Dst(InDst)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevel(InMipLevel)
        , SrcData(InSrcData)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateTexture2D(Dst, Width, Height, MipLevel, SrcData);
    }

    FRHITexture2D* Dst;
    uint16         Width;
    uint16         Height;
    uint16         MipLevel;
    const void*    SrcData;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandResolveTexture

DECLARE_RHICOMMAND(FRHICommandResolveTexture)
{
    FORCEINLINE FRHICommandResolveTexture(FRHITexture* InDst, FRHITexture* InSrc)
        : Dst(InDst)
        , Src(InSrc)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ResolveTexture(Dst, Src);
    }

    FRHITexture* Dst;
    FRHITexture* Src;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandResolveTexture

DECLARE_RHICOMMAND(FRHICommandCopyBuffer)
{
    FORCEINLINE FRHICommandCopyBuffer(FRHIBuffer* InDst, FRHIBuffer* InSrc, const FRHICopyBufferInfo& InCopyBufferInfo)
        : Dst(InDst)
        , Src(InSrc)
        , CopyBufferInfo(InCopyBufferInfo)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyBuffer(Dst, Src, CopyBufferInfo);
    }

    FRHIBuffer*        Dst;
    FRHIBuffer*        Src;
    FRHICopyBufferInfo CopyBufferInfo;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandCopyTexture

DECLARE_RHICOMMAND(FRHICommandCopyTexture)
{
    FORCEINLINE FRHICommandCopyTexture(FRHITexture* InDestination, FRHITexture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTexture(Destination, Source);
    }

    FRHITexture* Destination;
    FRHITexture* Source;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandCopyTextureRegion

DECLARE_RHICOMMAND(FRHICommandCopyTextureRegion)
{
    FORCEINLINE FRHICommandCopyTextureRegion(FRHITexture* InDst, FRHITexture* InSrc, const FRHICopyTextureInfo& InCopyInfo)
        : Dst(InDst)
        , Src(InSrc)
        , CopyInfo(InCopyInfo)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTextureRegion(Dst, Src, CopyInfo);
    }

    FRHITexture*        Dst;
    FRHITexture*        Src;
    FRHICopyTextureInfo CopyInfo;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDestroyResource

DECLARE_RHICOMMAND(FRHICommandDestroyResource)
{
    FORCEINLINE FRHICommandDestroyResource(IRefCounted* InResource)
        : Resource(MakeSharedRef<IRefCounted>(InResource))
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DestroyResource(Resource.Get());
    }

    TSharedRef<IRefCounted> Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDiscardContents

DECLARE_RHICOMMAND(FRHICommandDiscardContents)
{
    FORCEINLINE FRHICommandDiscardContents(FRHITexture* InTexture)
        : Texture(InTexture)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DiscardContents(Texture);
    }

    FRHITexture* Texture;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandBuildRayTracingGeometry

DECLARE_RHICOMMAND(FRHICommandBuildRayTracingGeometry)
{
    FORCEINLINE FRHICommandBuildRayTracingGeometry( FRHIRayTracingGeometry* InGeometry
                                                  , FRHIVertexBuffer* InVertexBuffer
                                                  , FRHIIndexBuffer* InIndexBuffer
                                                  , bool bInUpdate)
        : Geometry(InGeometry)
        , VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
        , bUpdate(bInUpdate)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildRayTracingGeometry(Geometry, VertexBuffer, IndexBuffer, bUpdate);
    }

    FRHIRayTracingGeometry* Geometry;
    FRHIVertexBuffer*       VertexBuffer;
    FRHIIndexBuffer*        IndexBuffer; 
    bool                    bUpdate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandBuildRayTracingScene

DECLARE_RHICOMMAND(FRHICommandBuildRayTracingScene)
{
    FORCEINLINE FRHICommandBuildRayTracingScene( FRHIRayTracingScene* InScene
                                               , const TArrayView<const FRHIRayTracingGeometryInstance>& InInstances
                                               , bool bInUpdate)
        : Scene(InScene)
        , Instances(InInstances)
        , bUpdate(bInUpdate)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildRayTracingScene(Scene, Instances, bUpdate);
    }

    FRHIRayTracingScene*                             Scene;
    TArrayView<const FRHIRayTracingGeometryInstance> Instances;
    bool                                             bUpdate;
};


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandSetRayTracingBindings

DECLARE_RHICOMMAND(FRHICommandSetRayTracingBindings)
{
    FORCEINLINE FRHICommandSetRayTracingBindings( FRHIRayTracingScene* InRayTracingScene
                                                , FRHIRayTracingPipelineState* InPipelineState
                                                , const FRayTracingShaderResources* InGlobalResource
                                                , const FRayTracingShaderResources* InRayGenLocalResources
                                                , const FRayTracingShaderResources* InMissLocalResources
                                                , const FRayTracingShaderResources* InHitGroupResources
                                                , uint32 InNumHitGroupResources)
        : RayTracingScene(InRayTracingScene)
        , PipelineState(InPipelineState)
        , GlobalResource(InGlobalResource)
        , RayGenLocalResources(InRayGenLocalResources)
        , MissLocalResources(InMissLocalResources)
        , HitGroupResources(InHitGroupResources)
        , NumHitGroupResources(InNumHitGroupResources)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetRayTracingBindings( RayTracingScene
                                            , PipelineState
                                            , GlobalResource
                                            , RayGenLocalResources
                                            , MissLocalResources
                                            , HitGroupResources
                                            , NumHitGroupResources);
    }

    FRHIRayTracingScene*              RayTracingScene;
    FRHIRayTracingPipelineState*      PipelineState;
    const FRayTracingShaderResources* GlobalResource;
    const FRayTracingShaderResources* RayGenLocalResources;
    const FRayTracingShaderResources* MissLocalResources;
    const FRayTracingShaderResources* HitGroupResources;
    uint32                            NumHitGroupResources;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandGenerateMips

DECLARE_RHICOMMAND(FRHICommandGenerateMips)
{
    FORCEINLINE FRHICommandGenerateMips(FRHITexture* InTexture)
        : Texture(InTexture)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.GenerateMips(Texture);
    }

    FRHITexture* Texture;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandTransitionTexture

DECLARE_RHICOMMAND(FRHICommandTransitionTexture)
{
    FORCEINLINE FRHICommandTransitionTexture(FRHITexture* InTexture, EResourceAccess InBeforeState, EResourceAccess InAfterState)
        : Texture(InTexture)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.TransitionTexture(Texture, BeforeState, AfterState);
    }

    FRHITexture*      Texture;
    EResourceAccess BeforeState;
    EResourceAccess AfterState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandTransitionBuffer

DECLARE_RHICOMMAND(FRHICommandTransitionBuffer)
{
    FORCEINLINE FRHICommandTransitionBuffer(FRHIBuffer* InBuffer, EResourceAccess InBeforeState, EResourceAccess InAfterState)
        : Buffer(InBuffer)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.TransitionBuffer(Buffer, BeforeState, AfterState);
    }

    FRHIBuffer*       Buffer;
    EResourceAccess BeforeState;
    EResourceAccess AfterState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandUnorderedAccessTextureBarrier

DECLARE_RHICOMMAND(FRHICommandUnorderedAccessTextureBarrier)
{
    FORCEINLINE FRHICommandUnorderedAccessTextureBarrier(FRHITexture* InTexture)
        : Texture(InTexture)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UnorderedAccessTextureBarrier(Texture);
    }

    FRHITexture* Texture;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandUnorderedAccessBufferBarrier

DECLARE_RHICOMMAND(FRHICommandUnorderedAccessBufferBarrier)
{
    FORCEINLINE FRHICommandUnorderedAccessBufferBarrier(FRHIBuffer* InBuffer)
        : Buffer(InBuffer)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UnorderedAccessBufferBarrier(Buffer);
    }

    FRHIBuffer* Buffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDraw

DECLARE_RHICOMMAND(FRHICommandDraw)
{
    FORCEINLINE FRHICommandDraw(uint32 InVertexCount, uint32 InStartVertexLocation)
        : VertexCount(InVertexCount)
        , StartVertexLocation(InStartVertexLocation)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Draw(VertexCount, StartVertexLocation);
    }

    uint32 VertexCount;
    uint32 StartVertexLocation;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDrawIndexed

DECLARE_RHICOMMAND(FRHICommandDrawIndexed)
{
    FORCEINLINE FRHICommandDrawIndexed(uint32 InIndexCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation)
        : IndexCount(InIndexCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    uint32 IndexCount;
    uint32 StartIndexLocation;
    uint32 BaseVertexLocation;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDrawInstanced

DECLARE_RHICOMMAND(FRHICommandDrawInstanced)
{
    FORCEINLINE FRHICommandDrawInstanced(uint32 InVertexCountPerInstance, uint32 InInstanceCount, uint32 InStartVertexLocation, uint32 InStartInstanceLocation)
        : VertexCountPerInstance(InVertexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartVertexLocation(InStartVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    uint32 VertexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartVertexLocation;
    uint32 StartInstanceLocation;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDrawIndexedInstanced

DECLARE_RHICOMMAND(FRHICommandDrawIndexedInstanced)
{
    FORCEINLINE FRHICommandDrawIndexedInstanced(uint32 InIndexCountPerInstance
                                                , uint32 InInstanceCount
                                                , uint32 InStartIndexLocation
                                                , uint32 InBaseVertexLocation
                                                , uint32 InStartInstanceLocation)
        : IndexCountPerInstance(InIndexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    uint32 IndexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartIndexLocation;
    uint32 BaseVertexLocation;
    uint32 StartInstanceLocation;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDispatch

DECLARE_RHICOMMAND(FRHICommandDispatch)
{
    FORCEINLINE FRHICommandDispatch(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
        : ThreadGroupCountX(InThreadGroupCountX)
        , ThreadGroupCountY(InThreadGroupCountY)
        , ThreadGroupCountZ(InThreadGroupCountZ)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    uint32 ThreadGroupCountX;
    uint32 ThreadGroupCountY;
    uint32 ThreadGroupCountZ;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDispatchRays

DECLARE_RHICOMMAND(FRHICommandDispatchRays)
{
    FORCEINLINE FRHICommandDispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
        : Scene(InScene)
        , PipelineState(InPipelineState)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DispatchRays(Scene, PipelineState, Width, Height, Depth);
    }

    FRHIRayTracingScene*         Scene;
    FRHIRayTracingPipelineState* PipelineState;
    uint32                       Width;
    uint32                       Height;
    uint32                       Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandInsertMarker

DECLARE_RHICOMMAND(FRHICommandInsertMarker)
{
    FORCEINLINE FRHICommandInsertMarker(const FString& InMarker)
        : Marker(InMarker)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        if (FDebug::IsDebuggerPresent())
        {
            FDebug::OutputDebugString(Marker + '\n');
        }

        LOG_INFO("%s", Marker.CStr());

        CommandContext.InsertMarker(Marker);
    }

    FString Marker;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandDebugBreak

DECLARE_RHICOMMAND(FRHICommandDebugBreak)
{
    FRHICommandDebugBreak() = default;

    FORCEINLINE void Execute(IRHICommandContext&)
    {
        if (FDebug::IsDebuggerPresent())
        {
            PlatformDebugBreak();
        }
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandBeginExternalCapture

DECLARE_RHICOMMAND(FRHICommandBeginExternalCapture)
{
    FRHICommandBeginExternalCapture() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginExternalCapture();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHICommandEndExternalCapture

DECLARE_RHICOMMAND(FRHICommandEndExternalCapture)
{
    FRHICommandEndExternalCapture() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndExternalCapture();
    }
};