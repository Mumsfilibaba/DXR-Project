#pragma once
#include "RHITypes.h"
#include "IRHICommandContext.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHIViewport.h"

#include "Core/Debug/Debug.h"
#include "Core/Memory/Memory.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/ArrayView.h"

#define DECLARE_RHICOMMAND(RHICommandName) class RHICommandName final : public TRHICommand<RHICommandName>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommand

class CRHICommand
{
public:
    virtual ~CRHICommand() = default;

    virtual void Execute(IRHICommandContext& CommandContext)           = 0;
    virtual void ExecuteAndRelease(IRHICommandContext& CommandContext) = 0;

    CRHICommand* NextCommand = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TRHICommand

template<typename CommandType>
class TRHICommand : public CRHICommand
{
public:
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
// CRHICommandBeginTimeStamp

DECLARE_RHICOMMAND(CRHICommandBeginTimeStamp)
{
public:
    FORCEINLINE CRHICommandBeginTimeStamp(CRHITimestampQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginTimeStamp(Query, Index);
    }

    CRHITimestampQuery* Query;
    uint32              Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandEndTimeStamp

DECLARE_RHICOMMAND(CRHICommandEndTimeStamp)
{
public:
    FORCEINLINE CRHICommandEndTimeStamp(CRHITimestampQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndTimeStamp(Query, Index);
    }

    CRHITimestampQuery* Query;
    uint32              Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearRenderTargetTexture

DECLARE_RHICOMMAND(CRHICommandClearRenderTargetTexture)
{
public:
    FORCEINLINE CRHICommandClearRenderTargetTexture(CRHITexture2D* InTexture, const SColorF& InClearColor)
        : Texture(InTexture)
        , ClearColor(InClearColor)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        //CommandContext.ClearRenderTargetView(RenderTargetView, ClearColor);
    }

    CRHITexture2D* Texture;
    SColorF        ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearRenderTargetView

DECLARE_RHICOMMAND(CRHICommandClearRenderTargetView)
{
public:
    FORCEINLINE CRHICommandClearRenderTargetView(CRHIRenderTargetView* InRenderTargetView, const SColorF& InClearColor)
        : RenderTargetView(InRenderTargetView)
        , ClearColor(InClearColor)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearRenderTargetView(RenderTargetView, ClearColor);
    }

    CRHIRenderTargetView* RenderTargetView;
    SColorF               ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearDepthStencilTexture

DECLARE_RHICOMMAND(CRHICommandClearDepthStencilTexture)
{
public:
    FORCEINLINE CRHICommandClearDepthStencilTexture(CRHITexture2D* InTexture, const SRHIDepthStencil& InClearValue)
        : Texture(InTexture)
        , ClearValue(InClearValue)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        //CommandContext.ClearDepthStencilView(DepthStencilView, ClearValue);
    }

    CRHITexture2D*   Texture;
    SRHIDepthStencil ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearDepthStencilView

DECLARE_RHICOMMAND(CRHICommandClearDepthStencilView)
{
public:
    FORCEINLINE CRHICommandClearDepthStencilView(CRHIDepthStencilView* InDepthStencilView, const SRHIDepthStencil& InClearValue)
        : DepthStencilView(InDepthStencilView)
        , ClearValue(InClearValue)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearDepthStencilView(DepthStencilView, ClearValue);
    }

    CRHIDepthStencilView* DepthStencilView;
    SRHIDepthStencil         ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessTextureFloat

DECLARE_RHICOMMAND(CRHICommandClearUnorderedAccessTextureFloat)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessTextureFloat(CRHITexture2D* InTexture, const SColorF& InClearColor)
        : Texture(InTexture)
        , ClearColor(InClearColor)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        //CommandContext.ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
    }

    CRHITexture2D* Texture;
    SColorF        ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessViewFloat

DECLARE_RHICOMMAND(CRHICommandClearUnorderedAccessViewFloat)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* InUnorderedAccessView, const SColorF& InClearColor)
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor(InClearColor)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
    }

    CRHIUnorderedAccessView* UnorderedAccessView;
    SColorF                  ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShadingRate

DECLARE_RHICOMMAND(CRHICommandSetShadingRate)
{
public:
    FORCEINLINE CRHICommandSetShadingRate(ERHIShadingRate ShadingRate)
        : ShadingRate(ShadingRate)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShadingRate(ShadingRate);
    }

    ERHIShadingRate ShadingRate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShadingRateTexture

DECLARE_RHICOMMAND(CRHICommandSetShadingRateTexture)
{
public:
    FORCEINLINE CRHICommandSetShadingRateTexture(CRHITexture2D* InShadingImage)
        : ShadingImage(InShadingImage)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShadingRateTexture(ShadingImage);
    }

    CRHITexture2D* ShadingImage;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetViewport

DECLARE_RHICOMMAND(CRHICommandSetViewport)
{
public:
    FORCEINLINE CRHICommandSetViewport(float InWidth, float InHeight, float InMinDepth, float InMaxDepth, float InX, float InY)
        : Width(InWidth)
        , Height(InHeight)
        , MinDepth(InMinDepth)
        , MaxDepth(InMaxDepth)
        , x(InX)
        , y(InY)
    {
    }

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
// CRHICommandSetScissorRect

DECLARE_RHICOMMAND(CRHICommandSetScissorRect)
{
public:
    FORCEINLINE CRHICommandSetScissorRect(float InWidth, float InHeight, float InX, float InY)
        : Width(InWidth)
        , Height(InHeight)
        , x(InX)
        , y(InY)
    {
    }

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
// CRHICommandSetBlendFactor

DECLARE_RHICOMMAND(CRHICommandSetBlendFactor)
{
public:
    FORCEINLINE CRHICommandSetBlendFactor(const SColorF& InColor)
        : Color(InColor)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetBlendFactor(Color);
    }

    SColorF Color;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandBeginRenderPass

DECLARE_RHICOMMAND(CRHICommandBeginRenderPass)
{
public:
    FORCEINLINE CRHICommandBeginRenderPass()
    {
        // Empty for now
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginRenderPass();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandEndRenderPass

DECLARE_RHICOMMAND(CRHICommandEndRenderPass)
{
public:
    FORCEINLINE CRHICommandEndRenderPass()
    {
        // Empty for now
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndRenderPass();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetPrimitiveTopology

DECLARE_RHICOMMAND(CRHICommandSetPrimitiveTopology)
{
public:
    FORCEINLINE CRHICommandSetPrimitiveTopology(ERHIPrimitiveTopology InPrimitiveTopologyType)
        : PrimitiveTopologyType(InPrimitiveTopologyType)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetPrimitiveTopology(PrimitiveTopologyType);
    }

    ERHIPrimitiveTopology PrimitiveTopologyType;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetVertexBuffers

DECLARE_RHICOMMAND(CRHICommandSetVertexBuffers)
{
public:
    FORCEINLINE CRHICommandSetVertexBuffers(CRHIBuffer** InVertexBuffers, uint32 InVertexBufferCount, uint32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    {
    }

    FORCEINLINE ~CRHICommandSetVertexBuffers()
    {
        VertexBuffers = nullptr;
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetVertexBuffers(VertexBuffers, VertexBufferCount, StartSlot);
    }

    CRHIBuffer** VertexBuffers;
    uint32       VertexBufferCount;
    uint32       StartSlot;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetIndexBuffer

DECLARE_RHICOMMAND(CRHICommandSetIndexBuffer)
{
public:
    FORCEINLINE CRHICommandSetIndexBuffer(CRHIBuffer* InIndexBuffer, ERHIIndexFormat InIndexFormat)
        : IndexBuffer(InIndexBuffer)
        , IndexFormat(InIndexFormat)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetIndexBuffer(IndexBuffer, IndexFormat);
    }

    CRHIBuffer*     IndexBuffer;
    ERHIIndexFormat IndexFormat;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetRenderTargets

DECLARE_RHICOMMAND(CRHICommandSetRenderTargets)
{
public:
    FORCEINLINE CRHICommandSetRenderTargets(CRHIRenderTargetView** InRenderTargetViews, uint32 InRenderTargetViewCount, CRHIDepthStencilView* InDepthStencilView)
        : RenderTargetViews(InRenderTargetViews)
        , RenderTargetViewCount(InRenderTargetViewCount)
        , DepthStencilView(InDepthStencilView)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetRenderTargets(RenderTargetViews, RenderTargetViewCount, DepthStencilView);
    }

    CRHIRenderTargetView** RenderTargetViews;
    uint32                 RenderTargetViewCount;
    CRHIDepthStencilView*  DepthStencilView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetRayTracingBindings

DECLARE_RHICOMMAND(CRHICommandSetRayTracingBindings)
{
public:
    FORCEINLINE CRHICommandSetRayTracingBindings(
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

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetRayTracingBindings(Scene, PipelineState, GlobalResources, RayGenLocalResources, MissLocalResources, HitGroupResources, NumHitGroupResources);
    }

    CRHIRayTracingScene*              Scene;
    CRHIRayTracingPipelineState*      PipelineState;
    const SRayTracingShaderResources* GlobalResources;
    const SRayTracingShaderResources* RayGenLocalResources;
    const SRayTracingShaderResources* MissLocalResources;
    const SRayTracingShaderResources* HitGroupResources;
    uint32                            NumHitGroupResources;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetGraphicsPipelineState

DECLARE_RHICOMMAND(CRHICommandSetGraphicsPipelineState)
{
public:
    FORCEINLINE CRHICommandSetGraphicsPipelineState(CRHIGraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetGraphicsPipelineState(PipelineState);
    }

    CRHIGraphicsPipelineState* PipelineState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetComputePipelineState

DECLARE_RHICOMMAND(CRHICommandSetComputePipelineState)
{
public:
    FORCEINLINE CRHICommandSetComputePipelineState(CRHIComputePipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetComputePipelineState(PipelineState);
    }

    CRHIComputePipelineState* PipelineState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSet32BitShaderConstants

DECLARE_RHICOMMAND(CRHICommandSet32BitShaderConstants)
{
public:
    FORCEINLINE CRHICommandSet32BitShaderConstants(CRHIShader* InShader, const void* InShader32BitConstants, uint32 InNum32BitConstants)
        : Shader(InShader)
        , Shader32BitConstants(InShader32BitConstants)
        , Num32BitConstants(InNum32BitConstants)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Set32BitShaderConstants(Shader, Shader32BitConstants, Num32BitConstants);
    }

    CRHIShader* Shader;
    const void* Shader32BitConstants;
    uint32      Num32BitConstants;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShaderResourceTexture

DECLARE_RHICOMMAND(CRHICommandSetShaderResourceTexture)
{
public:
    FORCEINLINE CRHICommandSetShaderResourceTexture(CRHIShader* InShader, CRHITexture* InTexture, uint32 InParameterIndex)
        : Shader(InShader)
        , Texture(InTexture)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        //CommandContext.SetShaderResourceView(Shader, ShaderResourceView, ParameterIndex);
    }

    CRHIShader*  Shader;
    CRHITexture* Texture;
    uint32       ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShaderResourceView

DECLARE_RHICOMMAND(CRHICommandSetShaderResourceView)
{
public:
    FORCEINLINE CRHICommandSetShaderResourceView(CRHIShader* InShader, CRHIShaderResourceView* InShaderResourceView, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceView(InShaderResourceView)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderResourceView(Shader, ShaderResourceView, ParameterIndex);
    }

    CRHIShader*             Shader;
    CRHIShaderResourceView* ShaderResourceView;
    uint32                  ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShaderResourceViews

DECLARE_RHICOMMAND(CRHICommandSetShaderResourceViews)
{
public:
    FORCEINLINE CRHICommandSetShaderResourceViews(CRHIShader* InShader, CRHIShaderResourceView** InShaderResourceViews, uint32 InNumShaderResourceViews, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceViews(InShaderResourceViews)
        , NumShaderResourceViews(InNumShaderResourceViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderResourceViews(Shader, ShaderResourceViews, NumShaderResourceViews, ParameterIndex);
    }

    CRHIShader*              Shader;
    CRHIShaderResourceView** ShaderResourceViews;
    uint32                   NumShaderResourceViews;
    uint32                   ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetUnorderedAccessTexture

DECLARE_RHICOMMAND(CRHICommandSetUnorderedAccessTexture)
{
public:
    FORCEINLINE CRHICommandSetUnorderedAccessTexture(CRHIShader* InShader, CRHITexture* InTexture, uint32 InParameterIndex)
        : Shader(InShader)
        , Texture(InTexture)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        //CommandContext.SetUnorderedAccessView(Shader, UnorderedAccessView, ParameterIndex);
    }

    CRHIShader*  Shader;
    CRHITexture* Texture;
    uint32       ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetUnorderedAccessView

DECLARE_RHICOMMAND(CRHICommandSetUnorderedAccessView)
{
public:
    FORCEINLINE CRHICommandSetUnorderedAccessView(CRHIShader* InShader, CRHIUnorderedAccessView* InUnorderedAccessView, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessView(InUnorderedAccessView)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetUnorderedAccessView(Shader, UnorderedAccessView, ParameterIndex);
    }

    CRHIShader*              Shader;
    CRHIUnorderedAccessView* UnorderedAccessView;
    uint32                   ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetUnorderedAccessViews

DECLARE_RHICOMMAND(CRHICommandSetUnorderedAccessViews)
{
public:
    FORCEINLINE CRHICommandSetUnorderedAccessViews(CRHIShader* InShader, CRHIUnorderedAccessView** InUnorderedAccessViews, uint32 InNumUnorderedAccessViews, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , NumUnorderedAccessViews(InNumUnorderedAccessViews)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetUnorderedAccessViews(Shader, UnorderedAccessViews, NumUnorderedAccessViews, ParameterIndex);
    }

    CRHIShader*               Shader;
    CRHIUnorderedAccessView** UnorderedAccessViews;
    uint32                    NumUnorderedAccessViews;
    uint32                    ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetConstantBuffer

DECLARE_RHICOMMAND(CRHICommandSetConstantBuffer)
{
public:
    FORCEINLINE CRHICommandSetConstantBuffer(CRHIShader* InShader, CRHIBuffer* InConstantBuffer, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffer(InConstantBuffer)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetConstantBuffer(Shader, ConstantBuffer, ParameterIndex);
    }

    CRHIShader* Shader;
    CRHIBuffer* ConstantBuffer;
    uint32      ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetConstantBuffers

DECLARE_RHICOMMAND(CRHICommandSetConstantBuffers)
{
public:
    FORCEINLINE CRHICommandSetConstantBuffers(CRHIShader* InShader, CRHIBuffer** InConstantBuffers, uint32 InNumConstantBuffers, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffers(InConstantBuffers)
        , NumConstantBuffers(InNumConstantBuffers)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetConstantBuffers(Shader, ConstantBuffers, NumConstantBuffers, ParameterIndex);
    }

    CRHIShader*  Shader;
    CRHIBuffer** ConstantBuffers;
    uint32       NumConstantBuffers;
    uint32       ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetSamplerState

DECLARE_RHICOMMAND(CRHICommandSetSamplerState)
{
public:
    FORCEINLINE CRHICommandSetSamplerState(CRHIShader* InShader, CRHISamplerState* InSamplerState, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerState(InSamplerState)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetSamplerState(Shader, SamplerState, ParameterIndex);
    }

    CRHIShader*       Shader;
    CRHISamplerState* SamplerState;
    uint32            ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetSamplerStates

DECLARE_RHICOMMAND(CRHICommandSetSamplerStates)
{
public:
    FORCEINLINE CRHICommandSetSamplerStates(CRHIShader* InShader, CRHISamplerState** InSamplerStates, uint32 InNumSamplerStates, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , NumSamplerStates(InNumSamplerStates)
        , ParameterIndex(InParameterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetSamplerStates(Shader, SamplerStates, NumSamplerStates, ParameterIndex);
    }

    CRHIShader*        Shader;
    CRHISamplerState** SamplerStates;
    uint32             NumSamplerStates;
    uint32             ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandResolveTexture

DECLARE_RHICOMMAND(CRHICommandResolveTexture)
{
public:
    FORCEINLINE CRHICommandResolveTexture(CRHITexture* InDestination, CRHITexture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ResolveTexture(Destination, Source);
    }

    CRHITexture* Destination;
    CRHITexture* Source;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUpdateBuffer

DECLARE_RHICOMMAND(CRHICommandUpdateBuffer)
{
public:
    FORCEINLINE CRHICommandUpdateBuffer(CRHIBuffer* InDestination, uint64 InDestinationOffsetInBytes, uint64 InSizeInBytes, const void* InSourceData)
        : Destination(InDestination)
        , DestinationOffsetInBytes(InDestinationOffsetInBytes)
        , SizeInBytes(InSizeInBytes)
        , SourceData(InSourceData)
    {
        Assert(InSourceData  != nullptr);
        Assert(InSizeInBytes != 0);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateBuffer(Destination, DestinationOffsetInBytes, SizeInBytes, SourceData);
    }

    CRHIBuffer* Destination;
    uint64      DestinationOffsetInBytes;
    uint64      SizeInBytes;
    const void* SourceData;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUpdateTexture2D

DECLARE_RHICOMMAND(CRHICommandUpdateTexture2D)
{
public:
    FORCEINLINE CRHICommandUpdateTexture2D(CRHITexture2D* InDestination, uint32 InWidth, uint32 InHeight, uint32 InMipLevel, const void* InSourceData)
        : Destination(InDestination)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevel(InMipLevel)
        , SourceData(InSourceData)
    {
        Assert(InSourceData != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateTexture2D(Destination, Width, Height, MipLevel, SourceData);
    }

    CRHITexture2D* Destination;
    uint32         Width;
    uint32         Height;
    uint32         MipLevel;
    const void*    SourceData;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandCopyBuffer

DECLARE_RHICOMMAND(CRHICommandCopyBuffer)
{
public:
    FORCEINLINE CRHICommandCopyBuffer(CRHIBuffer* InDestination, CRHIBuffer* InSource, const SRHICopyBufferInfo& InCopyBufferInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyBufferInfo(InCopyBufferInfo)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyBuffer(Destination, Source, CopyBufferInfo);
    }

    CRHIBuffer*        Destination;
    CRHIBuffer*        Source;
    SRHICopyBufferInfo CopyBufferInfo;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandCopyTexture

DECLARE_RHICOMMAND(CRHICommandCopyTexture)
{
public:
    FORCEINLINE CRHICommandCopyTexture(CRHITexture* InDestination, CRHITexture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTexture(Destination, Source);
    }

    CRHITexture* Destination;
    CRHITexture* Source;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandCopyTextureRegion

DECLARE_RHICOMMAND(CRHICommandCopyTextureRegion)
{
public:
    FORCEINLINE CRHICommandCopyTextureRegion(CRHITexture* InDestination, CRHITexture* InSource, const SRHICopyTextureInfo& InCopyTextureInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyTextureInfo(InCopyTextureInfo)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTextureRegion(Destination, Source, CopyTextureInfo);
    }

    CRHITexture*        Destination;
    CRHITexture*        Source;
    SRHICopyTextureInfo CopyTextureInfo;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDestroyResource

DECLARE_RHICOMMAND(CRHICommandDestroyResource)
{
public:
    FORCEINLINE CRHICommandDestroyResource(CRHIObject* InResource)
        : Resource(InResource)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DestroyResource(Resource);
    }

    CRHIObject* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDiscardContents

DECLARE_RHICOMMAND(CRHICommandDiscardContents)
{
public:
    FORCEINLINE CRHICommandDiscardContents(CRHIResource* InResource)
        : Resource(InResource)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DiscardContents(Resource);
    }

    CRHIResource* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandBuildRayTracingGeometry

DECLARE_RHICOMMAND(CRHICommandBuildRayTracingGeometry)
{
public:
    FORCEINLINE CRHICommandBuildRayTracingGeometry(CRHIRayTracingGeometry* InRayTracingGeometry, CRHIBuffer* InVertexBuffer, CRHIBuffer* InIndexBuffer, bool bInUpdate)
        : RayTracingGeometry(InRayTracingGeometry)
        , VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
        , bUpdate(bInUpdate)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildRayTracingGeometry(RayTracingGeometry, VertexBuffer, IndexBuffer, bUpdate);
    }

    CRHIRayTracingGeometry* RayTracingGeometry;
    CRHIBuffer*             VertexBuffer;
    CRHIBuffer*             IndexBuffer;
    bool                    bUpdate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandBuildRayTracingScene

DECLARE_RHICOMMAND(CRHICommandBuildRayTracingScene)
{
public:
    FORCEINLINE CRHICommandBuildRayTracingScene(CRHIRayTracingScene* InRayTracingScene, const SRHIRayTracingGeometryInstance* InInstances, uint32 InNumInstances, bool bInUpdate)
        : RayTracingScene(InRayTracingScene)
        , Instances(InInstances)
        , NumInstances(InNumInstances)
        , bUpdate(bInUpdate)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildRayTracingScene(RayTracingScene, Instances, NumInstances, bUpdate);
    }

    CRHIRayTracingScene*                  RayTracingScene;
    const SRHIRayTracingGeometryInstance* Instances;
    uint32                                NumInstances;
    bool                                  bUpdate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandGenerateMips

DECLARE_RHICOMMAND(CRHICommandGenerateMips)
{
public:
    FORCEINLINE CRHICommandGenerateMips(CRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.GenerateMips(Texture);
    }

    CRHITexture* Texture;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandTransitionTexture

DECLARE_RHICOMMAND(CRHICommandTransitionTexture)
{
public:
    FORCEINLINE CRHICommandTransitionTexture(CRHITexture* InTexture, ERHIResourceState InBeforeState, ERHIResourceState InAfterState)
        : Texture(InTexture)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.TransitionTexture(Texture, BeforeState, AfterState);
    }

    CRHITexture*      Texture;
    ERHIResourceState BeforeState;
    ERHIResourceState AfterState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandTransitionBuffer

DECLARE_RHICOMMAND(CRHICommandTransitionBuffer)
{
public:
    FORCEINLINE CRHICommandTransitionBuffer(CRHIBuffer* InBuffer, ERHIResourceState InBeforeState, ERHIResourceState InAfterState)
        : Buffer(InBuffer)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.TransitionBuffer(Buffer, BeforeState, AfterState);
    }

    CRHIBuffer*       Buffer;
    ERHIResourceState BeforeState;
    ERHIResourceState AfterState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUnorderedAccessTextureBarrier

DECLARE_RHICOMMAND(CRHICommandUnorderedAccessTextureBarrier)
{
public:
    FORCEINLINE CRHICommandUnorderedAccessTextureBarrier(CRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UnorderedAccessTextureBarrier(Texture);
    }

    CRHITexture* Texture;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUnorderedAccessBufferBarrier

DECLARE_RHICOMMAND(CRHICommandUnorderedAccessBufferBarrier)
{
public:
    FORCEINLINE CRHICommandUnorderedAccessBufferBarrier(CRHIBuffer* InBuffer)
        : Buffer(InBuffer)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UnorderedAccessBufferBarrier(Buffer);
    }

    CRHIBuffer* Buffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDraw

DECLARE_RHICOMMAND(CRHICommandDraw)
{
public:
    FORCEINLINE CRHICommandDraw(uint32 InVertexCount, uint32 InStartVertexLocation)
        : VertexCount(InVertexCount)
        , StartVertexLocation(InStartVertexLocation)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Draw(VertexCount, StartVertexLocation);
    }

    uint32 VertexCount;
    uint32 StartVertexLocation;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDrawIndexed

DECLARE_RHICOMMAND(CRHICommandDrawIndexed)
{
public:
    FORCEINLINE CRHICommandDrawIndexed(uint32 InIndexCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation)
        : IndexCount(InIndexCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    }

    uint32 IndexCount;
    uint32 StartIndexLocation;
    uint32 BaseVertexLocation;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDrawInstanced

DECLARE_RHICOMMAND(CRHICommandDrawInstanced)
{
public:
    FORCEINLINE CRHICommandDrawInstanced(uint32 InVertexCountPerInstance, uint32 InInstanceCount, uint32 InStartVertexLocation, uint32 InStartInstanceLocation)
        : VertexCountPerInstance(InVertexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartVertexLocation(InStartVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
    }

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
// CRHICommandDrawIndexedInstanced

DECLARE_RHICOMMAND(CRHICommandDrawIndexedInstanced)
{
public:
    FORCEINLINE CRHICommandDrawIndexedInstanced(uint32 InIndexCountPerInstance, uint32 InInstanceCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation, uint32 InStartInstanceLocation)
        : IndexCountPerInstance(InIndexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
    }

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
// CRHICommandDispatch

DECLARE_RHICOMMAND(CRHICommandDispatch)
{
public:
    FORCEINLINE CRHICommandDispatch(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
        : ThreadGroupCountX(InThreadGroupCountX)
        , ThreadGroupCountY(InThreadGroupCountY)
        , ThreadGroupCountZ(InThreadGroupCountZ)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    uint32 ThreadGroupCountX;
    uint32 ThreadGroupCountY;
    uint32 ThreadGroupCountZ;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDispatchRays

DECLARE_RHICOMMAND(CRHICommandDispatchRays)
{
public:
    FORCEINLINE CRHICommandDispatchRays(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
        : Scene(InScene)
        , PipelineState(InPipelineState)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DispatchRays(Scene, PipelineState, Width, Height, Depth);
    }

    CRHIRayTracingScene*         Scene;
    CRHIRayTracingPipelineState* PipelineState;
    uint32                       Width;
    uint32                       Height;
    uint32                       Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandInsertMarker

DECLARE_RHICOMMAND(CRHICommandInsertMarker)
{
public:
    FORCEINLINE CRHICommandInsertMarker(const String& InMarker)
        : Marker(InMarker)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CDebug::OutputDebugString(Marker + '\n');
        LOG_INFO(Marker);

        CommandContext.InsertMarker(Marker);
    }

    String Marker;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDebugBreak

DECLARE_RHICOMMAND(CRHICommandDebugBreak)
{
public:
    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        UNREFERENCED_VARIABLE(CommandContext);
        CDebug::DebugBreak();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandBeginExternalCapture

DECLARE_RHICOMMAND(CRHICommandBeginExternalCapture)
{
public:
    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginExternalCapture();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandEndExternalCapture

DECLARE_RHICOMMAND(CRHICommandEndExternalCapture)
{
public:
    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndExternalCapture();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandPresentViewport

DECLARE_RHICOMMAND(CRHICommandPresentViewport)
{
public:
    FORCEINLINE CRHICommandPresentViewport(CRHIViewport* InViewport, bool bInVerticalSync)
        : Viewport(InViewport)
        , bVerticalSync(bInVerticalSync)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.PresentViewport(Viewport, bVerticalSync);
    }

    CRHIViewport* Viewport;
    bool          bVerticalSync;
};
