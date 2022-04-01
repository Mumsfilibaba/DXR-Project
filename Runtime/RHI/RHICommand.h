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

#define DECLARE_RHICOMMAND_CLASS(RHICommandName) class RHICommandName final : public TRHICommand<RHICommandName>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommand

class CRHICommand
{
public:
    virtual ~CRHICommand() = default;

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

DECLARE_RHICOMMAND_CLASS(CRHICommandBeginTimeStamp)
{
public:
    FORCEINLINE CRHICommandBeginTimeStamp(CRHITimestampQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginTimeStamp(Query, Index);
    }

    CRHITimestampQuery* Query;
    uint32              Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandEndTimeStamp

DECLARE_RHICOMMAND_CLASS(CRHICommandEndTimeStamp)
{
public:
    FORCEINLINE CRHICommandEndTimeStamp(CRHITimestampQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndTimeStamp(Query, Index);
    }

    CRHITimestampQuery* Query;
    uint32              Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearRenderTargetTexture

DECLARE_RHICOMMAND_CLASS(CRHICommandClearRenderTargetTexture)
{
public:
    FORCEINLINE CRHICommandClearRenderTargetTexture(CRHITexture* InTexture, const float InClearColor[4])
        : Texture(InTexture)
        , ClearColor()
    {
        CMemory::Memcpy(ClearColor, InClearColor, sizeof(ClearColor));
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearRenderTargetTexture(Texture, ClearColor);
    }

    CRHITexture* Texture;
    float          ClearColor[4];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearRenderTargetView

DECLARE_RHICOMMAND_CLASS(CRHICommandClearRenderTargetView)
{
public:
    FORCEINLINE CRHICommandClearRenderTargetView(CRHIRenderTargetView* InRenderTargetView, const float InClearColor[4])
        : RenderTargetView(InRenderTargetView)
        , ClearColor()
    {
        CMemory::Memcpy(ClearColor, InClearColor, sizeof(ClearColor));
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearRenderTargetView(RenderTargetView, ClearColor);
    }

    CRHIRenderTargetView* RenderTargetView;
    float                 ClearColor[4];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearDepthStencilTexture

DECLARE_RHICOMMAND_CLASS(CRHICommandClearDepthStencilTexture)
{
public:
    FORCEINLINE CRHICommandClearDepthStencilTexture(CRHITexture* InTexture, const float InDepth, const uint8 InStencil)
        : Texture(InTexture)
        , Depth(InDepth)
        , Stencil(InStencil)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearDepthStencilTexture(Texture, Depth, Stencil);
    }

    CRHITexture* Texture;
    const float  Depth;
    const uint8  Stencil;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearDepthStencilView

DECLARE_RHICOMMAND_CLASS(CRHICommandClearDepthStencilView)
{
public:
    FORCEINLINE CRHICommandClearDepthStencilView(CRHIDepthStencilView* InDepthStencilView, const float InDepth, const uint8 InStencil)
        : DepthStencilView(InDepthStencilView)
        , Depth(InDepth)
        , Stencil(InStencil)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearDepthStencilView(DepthStencilView, Depth, Stencil);
    }

    CRHIDepthStencilView* DepthStencilView;
    const float           Depth;
    const uint8           Stencil;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessTextureFloat

DECLARE_RHICOMMAND_CLASS(CRHICommandClearUnorderedAccessTextureFloat)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessTextureFloat(CRHITexture* InTexture, const float InClearColor[4])
        : Texture(InTexture)
        , ClearColor()
    {
        CMemory::Memcpy(ClearColor, InClearColor, sizeof(ClearColor));
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessTextureFloat(Texture, ClearColor);
    }

    CRHITexture* Texture;
    float          ClearColor[4];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessViewFloat

DECLARE_RHICOMMAND_CLASS(CRHICommandClearUnorderedAccessViewFloat)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* InUnorderedAccessView, const float InClearColor[4])
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor()
    {
        CMemory::Memcpy(ClearColor, InClearColor, sizeof(ClearColor));
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
    }

    CRHIUnorderedAccessView* UnorderedAccessView;
    float                    ClearColor[4];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessTextureUint

DECLARE_RHICOMMAND_CLASS(CRHICommandClearUnorderedAccessTextureUint)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessTextureUint(CRHITexture* InTexture, const uint32 InClearColor[4])
        : Texture(InTexture)
        , ClearColor()
    {
        CMemory::Memcpy(ClearColor, InClearColor, sizeof(ClearColor));
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessTextureUint(Texture, ClearColor);
    }

    CRHITexture* Texture;
    uint32       ClearColor[4];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessViewUint

DECLARE_RHICOMMAND_CLASS(CRHICommandClearUnorderedAccessViewUint)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessViewUint(CRHIUnorderedAccessView* InUnorderedAccessView, const uint32 InClearColor[4])
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor()
    {
        CMemory::Memcpy(ClearColor, InClearColor, sizeof(ClearColor));
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessViewUint(UnorderedAccessView, ClearColor);
    }

    CRHIUnorderedAccessView* UnorderedAccessView;
    uint32                   ClearColor[4];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShadingRate

DECLARE_RHICOMMAND_CLASS(CRHICommandSetShadingRate)
{
public:
    FORCEINLINE CRHICommandSetShadingRate(EShadingRate ShadingRate)
        : ShadingRate(ShadingRate)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShadingRate(ShadingRate);
    }

    EShadingRate ShadingRate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShadingRateTexture

DECLARE_RHICOMMAND_CLASS(CRHICommandSetShadingRateTexture)
{
public:
    FORCEINLINE CRHICommandSetShadingRateTexture(CRHITexture* InShadingImage)
        : ShadingImage(InShadingImage)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShadingRateTexture(ShadingImage);
    }

    CRHITexture* ShadingImage;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetViewport

DECLARE_RHICOMMAND_CLASS(CRHICommandSetViewport)
{
public:
    FORCEINLINE CRHICommandSetViewport(float InWidth, float InHeight, float InMinDepth, float InMaxDepth, float InX, float InY)
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
// CRHICommandSetScissorRect

DECLARE_RHICOMMAND_CLASS(CRHICommandSetScissorRect)
{
public:
    FORCEINLINE CRHICommandSetScissorRect(float InWidth, float InHeight, float InX, float InY)
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
// CRHICommandSetBlendFactor

DECLARE_RHICOMMAND_CLASS(CRHICommandSetBlendFactor)
{
public:
    FORCEINLINE CRHICommandSetBlendFactor(const SColorF& InColor)
        : Color(InColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetBlendFactor(Color);
    }

    SColorF Color;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandBeginRenderPass

DECLARE_RHICOMMAND_CLASS(CRHICommandBeginRenderPass)
{
public:
    FORCEINLINE CRHICommandBeginRenderPass(const CRHIRenderPass& InRenderPass)
        : RenderPass(InRenderPass)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginRenderPass(RenderPass);
    }

    CRHIRenderPass RenderPass;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandEndRenderPass

DECLARE_RHICOMMAND_CLASS(CRHICommandEndRenderPass)
{
public:

    CRHICommandEndRenderPass() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndRenderPass();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetPrimitiveTopology

DECLARE_RHICOMMAND_CLASS(CRHICommandSetPrimitiveTopology)
{
public:
    FORCEINLINE CRHICommandSetPrimitiveTopology(EPrimitiveTopology InPrimitiveTopologyType)
        : PrimitiveTopologyType(InPrimitiveTopologyType)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetPrimitiveTopology(PrimitiveTopologyType);
    }

    EPrimitiveTopology PrimitiveTopologyType;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetVertexBuffers

DECLARE_RHICOMMAND_CLASS(CRHICommandSetVertexBuffers)
{
public:
    FORCEINLINE CRHICommandSetVertexBuffers(CRHIBuffer** InVertexBuffers, uint32 InVertexBufferCount, uint32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandSetIndexBuffer)
{
public:
    FORCEINLINE CRHICommandSetIndexBuffer(CRHIBuffer* InIndexBuffer, EIndexFormat InIndexFormat)
        : IndexBuffer(InIndexBuffer)
        , IndexFormat(InIndexFormat)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetIndexBuffer(IndexBuffer, IndexFormat);
    }

    CRHIBuffer*     IndexBuffer;
    EIndexFormat IndexFormat;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetRayTracingBindings

DECLARE_RHICOMMAND_CLASS(CRHICommandSetRayTracingBindings)
{
public:
    FORCEINLINE CRHICommandSetRayTracingBindings( CRHIRayTracingScene* InRayTracingScene
                                                , CRHIRayTracingPipelineState* InPipelineState
                                                , const SRHIRayTracingShaderResources* InGlobalResources
                                                , const SRHIRayTracingShaderResources* InRayGenLocalResources
                                                , const SRHIRayTracingShaderResources* InMissLocalResources
                                                , const SRHIRayTracingShaderResources* InHitGroupResources
                                                , uint32 InNumHitGroupResources)
        : Scene(InRayTracingScene)
        , PipelineState(InPipelineState)
        , GlobalResources(InGlobalResources)
        , RayGenLocalResources(InRayGenLocalResources)
        , MissLocalResources(InMissLocalResources)
        , HitGroupResources(InHitGroupResources)
        , NumHitGroupResources(InNumHitGroupResources)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetRayTracingBindings( Scene
                                            , PipelineState
                                            , GlobalResources
                                            , RayGenLocalResources
                                            , MissLocalResources
                                            , HitGroupResources
                                            , NumHitGroupResources);
    }

    CRHIRayTracingScene*                 Scene;
    CRHIRayTracingPipelineState*         PipelineState;
    const SRHIRayTracingShaderResources* GlobalResources;
    const SRHIRayTracingShaderResources* RayGenLocalResources;
    const SRHIRayTracingShaderResources* MissLocalResources;
    const SRHIRayTracingShaderResources* HitGroupResources;
    uint32                               NumHitGroupResources;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetGraphicsPipelineState

DECLARE_RHICOMMAND_CLASS(CRHICommandSetGraphicsPipelineState)
{
public:
    FORCEINLINE CRHICommandSetGraphicsPipelineState(CRHIGraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetGraphicsPipelineState(PipelineState);
    }

    CRHIGraphicsPipelineState* PipelineState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetComputePipelineState

DECLARE_RHICOMMAND_CLASS(CRHICommandSetComputePipelineState)
{
public:
    FORCEINLINE CRHICommandSetComputePipelineState(CRHIComputePipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetComputePipelineState(PipelineState);
    }

    CRHIComputePipelineState* PipelineState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSet32BitShaderConstants

DECLARE_RHICOMMAND_CLASS(CRHICommandSet32BitShaderConstants)
{
public:
    FORCEINLINE CRHICommandSet32BitShaderConstants(CRHIShader* InShader, const void* InShader32BitConstants, uint32 InNum32BitConstants)
        : Shader(InShader)
        , Shader32BitConstants(InShader32BitConstants)
        , Num32BitConstants(InNum32BitConstants)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandSetShaderResourceTexture)
{
public:
    FORCEINLINE CRHICommandSetShaderResourceTexture(CRHIShader* InShader, CRHITexture* InTexture, uint32 InParameterIndex)
        : Shader(InShader)
        , Texture(InTexture)
        , ParameterIndex(InParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderResourceTexture(Shader, Texture, ParameterIndex);
    }

    CRHIShader*  Shader;
    CRHITexture* Texture;
    uint32       ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShaderResourceTextures

DECLARE_RHICOMMAND_CLASS(CRHICommandSetShaderResourceTextures)
{
public:
    FORCEINLINE CRHICommandSetShaderResourceTextures(CRHIShader* InShader, CRHITexture* const* InTextures, uint32 InNumTextures, uint32 InStartParameterIndex)
        : Shader(InShader)
        , Textures(InTextures)
        , NumTextures(InNumTextures)
        , StartParameterIndex(InStartParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderResourceTextures(Shader, Textures, NumTextures, StartParameterIndex);
    }

    CRHIShader*         Shader;
    CRHITexture* const* Textures;
    uint32              NumTextures;
    uint32              StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetShaderResourceView

DECLARE_RHICOMMAND_CLASS(CRHICommandSetShaderResourceView)
{
public:
    FORCEINLINE CRHICommandSetShaderResourceView(CRHIShader* InShader, CRHIShaderResourceView* InShaderResourceView, uint32 InParameterIndex)
        : Shader(InShader)
        , ShaderResourceView(InShaderResourceView)
        , ParameterIndex(InParameterIndex)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandSetShaderResourceViews)
{
public:
    FORCEINLINE CRHICommandSetShaderResourceViews( CRHIShader* InShader
                                                 , CRHIShaderResourceView* const* InShaderResourceViews
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

    CRHIShader*                    Shader;
    CRHIShaderResourceView* const* ShaderResourceViews;
    uint32                         NumShaderResourceViews;
    uint32                         StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetUnorderedAccessTexture

DECLARE_RHICOMMAND_CLASS(CRHICommandSetUnorderedAccessTexture)
{
public:
    FORCEINLINE CRHICommandSetUnorderedAccessTexture(CRHIShader* InShader, CRHITexture* InTexture, uint32 InParameterIndex)
        : Shader(InShader)
        , Texture(InTexture)
        , ParameterIndex(InParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetUnorderedAccessTexture(Shader, Texture, ParameterIndex);
    }

    CRHIShader*  Shader;
    CRHITexture* Texture;
    uint32       ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetUnorderedAccessTextures

DECLARE_RHICOMMAND_CLASS(CRHICommandSetUnorderedAccessTextures)
{
public:
    FORCEINLINE CRHICommandSetUnorderedAccessTextures(CRHIShader* InShader, CRHITexture* const* InTextures, uint32 InNumTextures, uint32 InStartParameterIndex)
        : Shader(InShader)
        , Textures(InTextures)
        , NumTextures(InNumTextures)
        , StartParameterIndex(InStartParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetUnorderedAccessTextures(Shader, Textures, NumTextures, StartParameterIndex);
    }

    CRHIShader*         Shader;
    CRHITexture* const* Textures;
    uint32              NumTextures;
    uint32              StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetUnorderedAccessView

DECLARE_RHICOMMAND_CLASS(CRHICommandSetUnorderedAccessView)
{
public:
    FORCEINLINE CRHICommandSetUnorderedAccessView(CRHIShader* InShader, CRHIUnorderedAccessView* InUnorderedAccessView, uint32 InParameterIndex)
        : Shader(InShader)
        , UnorderedAccessView(InUnorderedAccessView)
        , ParameterIndex(InParameterIndex)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandSetUnorderedAccessViews)
{
public:
    FORCEINLINE CRHICommandSetUnorderedAccessViews( CRHIShader* InShader
                                                  , CRHIUnorderedAccessView* const* InUnorderedAccessViews
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

    CRHIShader*                     Shader;
    CRHIUnorderedAccessView* const* UnorderedAccessViews;
    uint32                          NumUnorderedAccessViews;
    uint32                          StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetConstantBuffer

DECLARE_RHICOMMAND_CLASS(CRHICommandSetConstantBuffer)
{
public:
    FORCEINLINE CRHICommandSetConstantBuffer(CRHIShader* InShader, CRHIBuffer* InConstantBuffer, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffer(InConstantBuffer)
        , ParameterIndex(InParameterIndex)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandSetConstantBuffers)
{
public:
    FORCEINLINE CRHICommandSetConstantBuffers( CRHIShader* InShader
                                             , CRHIBuffer* const* InConstantBuffers
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

    CRHIShader*        Shader;
    CRHIBuffer* const* ConstantBuffers;
    uint32             NumConstantBuffers;
    uint32             StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetSamplerState

DECLARE_RHICOMMAND_CLASS(CRHICommandSetSamplerState)
{
public:
    FORCEINLINE CRHICommandSetSamplerState(CRHIShader* InShader, CRHISamplerState* InSamplerState, uint32 InParameterIndex)
        : Shader(InShader)
        , SamplerState(InSamplerState)
        , ParameterIndex(InParameterIndex)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandSetSamplerStates)
{
public:
    FORCEINLINE CRHICommandSetSamplerStates(CRHIShader* InShader, CRHISamplerState* const* InSamplerStates, uint32 InNumSamplerStates, uint32 InStartParameterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , NumSamplerStates(InNumSamplerStates)
        , StartParameterIndex(InStartParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetSamplerStates(Shader, SamplerStates, NumSamplerStates, StartParameterIndex);
    }

    CRHIShader*              Shader;
    CRHISamplerState* const* SamplerStates;
    uint32                   NumSamplerStates;
    uint32                   StartParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandResolveTexture

DECLARE_RHICOMMAND_CLASS(CRHICommandResolveTexture)
{
public:
    FORCEINLINE CRHICommandResolveTexture(CRHITexture* InDestination, CRHITexture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ResolveTexture(Destination, Source);
    }

    CRHITexture* Destination;
    CRHITexture* Source;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUpdateBuffer

DECLARE_RHICOMMAND_CLASS(CRHICommandUpdateBuffer)
{
public:
    FORCEINLINE CRHICommandUpdateBuffer(CRHIBuffer* InDestination, uint64 InDestinationOffsetInBytes, uint64 InSizeInBytes, const void* InSourceData)
        : Destination(InDestination)
        , DestinationOffsetInBytes(InDestinationOffsetInBytes)
        , SizeInBytes(InSizeInBytes)
        , SourceData(InSourceData)
    {
        Check(InSourceData  != nullptr);
        Check(InSizeInBytes != 0);
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

DECLARE_RHICOMMAND_CLASS(CRHICommandUpdateTexture2D)
{
public:
    FORCEINLINE CRHICommandUpdateTexture2D(CRHITexture* InDestination, uint32 InWidth, uint32 InHeight, uint32 InMipLevel, const void* InSourceData)
        : Destination(InDestination)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevel(InMipLevel)
        , SourceData(InSourceData)
    {
        Check(InSourceData != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateTexture2D(Destination, Width, Height, MipLevel, SourceData);
    }

    CRHITexture* Destination;
    uint32         Width;
    uint32         Height;
    uint32         MipLevel;
    const void*    SourceData;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandCopyBuffer

DECLARE_RHICOMMAND_CLASS(CRHICommandCopyBuffer)
{
public:
    FORCEINLINE CRHICommandCopyBuffer(CRHIBuffer* InDestination, CRHIBuffer* InSource, const SRHICopyBufferInfo& InCopyBufferInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyBufferInfo(InCopyBufferInfo)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandCopyTexture)
{
public:
    FORCEINLINE CRHICommandCopyTexture(CRHITexture* InDestination, CRHITexture* InSource)
        : Destination(InDestination)
        , Source(InSource)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTexture(Destination, Source);
    }

    CRHITexture* Destination;
    CRHITexture* Source;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandCopyTextureRegion

DECLARE_RHICOMMAND_CLASS(CRHICommandCopyTextureRegion)
{
public:
    FORCEINLINE CRHICommandCopyTextureRegion(CRHITexture* InDestination, CRHITexture* InSource, const SRHICopyTextureInfo& InCopyTextureInfo)
        : Destination(InDestination)
        , Source(InSource)
        , CopyTextureInfo(InCopyTextureInfo)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandDestroyResource)
{
public:
    FORCEINLINE CRHICommandDestroyResource(CRHIResource* InResource)
        : Resource(InResource)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DestroyResource(Resource);
    }

    CRHIResource* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDiscardContents

DECLARE_RHICOMMAND_CLASS(CRHICommandDiscardContents)
{
public:
    FORCEINLINE CRHICommandDiscardContents(CRHIResource* InResource)
        : Resource(InResource)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DiscardContents(Resource);
    }

    CRHIResource* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandBuildRayTracingGeometry

DECLARE_RHICOMMAND_CLASS(CRHICommandBuildRayTracingGeometry)
{
public:
    FORCEINLINE CRHICommandBuildRayTracingGeometry( CRHIRayTracingGeometry* InRayTracingGeometry
                                                  , CRHIBuffer* InVertexBuffer
                                                  , CRHIBuffer* InIndexBuffer
                                                  , bool bInUpdate)
        : RayTracingGeometry(InRayTracingGeometry)
        , VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
        , bUpdate(bInUpdate)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandBuildRayTracingScene)
{
public:
    FORCEINLINE CRHICommandBuildRayTracingScene( CRHIRayTracingScene* InRayTracingScene
                                               , const SRHIRayTracingGeometryInstance* InInstances
                                               , uint32 InNumInstances
                                               , bool bInUpdate)
        : RayTracingScene(InRayTracingScene)
        , Instances(InInstances)
        , NumInstances(InNumInstances)
        , bUpdate(bInUpdate)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandGenerateMips)
{
public:
    FORCEINLINE CRHICommandGenerateMips(CRHITexture* InTexture)
        : Texture(InTexture)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.GenerateMips(Texture);
    }

    CRHITexture* Texture;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandTransitionTexture

DECLARE_RHICOMMAND_CLASS(CRHICommandTransitionTexture)
{
public:
    FORCEINLINE CRHICommandTransitionTexture(CRHITexture* InTexture, EResourceAccess InBeforeState, EResourceAccess InAfterState)
        : Texture(InTexture)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.TransitionTexture(Texture, BeforeState, AfterState);
    }

    CRHITexture*      Texture;
    EResourceAccess BeforeState;
    EResourceAccess AfterState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandTransitionBuffer

DECLARE_RHICOMMAND_CLASS(CRHICommandTransitionBuffer)
{
public:
    FORCEINLINE CRHICommandTransitionBuffer(CRHIBuffer* InBuffer, EResourceAccess InBeforeState, EResourceAccess InAfterState)
        : Buffer(InBuffer)
        , BeforeState(InBeforeState)
        , AfterState(InAfterState)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.TransitionBuffer(Buffer, BeforeState, AfterState);
    }

    CRHIBuffer*       Buffer;
    EResourceAccess BeforeState;
    EResourceAccess AfterState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUnorderedAccessTextureBarrier

DECLARE_RHICOMMAND_CLASS(CRHICommandUnorderedAccessTextureBarrier)
{
public:
    FORCEINLINE CRHICommandUnorderedAccessTextureBarrier(CRHITexture* InTexture)
        : Texture(InTexture)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UnorderedAccessTextureBarrier(Texture);
    }

    CRHITexture* Texture;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUnorderedAccessBufferBarrier

DECLARE_RHICOMMAND_CLASS(CRHICommandUnorderedAccessBufferBarrier)
{
public:
    FORCEINLINE CRHICommandUnorderedAccessBufferBarrier(CRHIBuffer* InBuffer)
        : Buffer(InBuffer)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UnorderedAccessBufferBarrier(Buffer);
    }

    CRHIBuffer* Buffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandDraw

DECLARE_RHICOMMAND_CLASS(CRHICommandDraw)
{
public:
    FORCEINLINE CRHICommandDraw(uint32 InVertexCount, uint32 InStartVertexLocation)
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
// CRHICommandDrawIndexed

DECLARE_RHICOMMAND_CLASS(CRHICommandDrawIndexed)
{
public:
    FORCEINLINE CRHICommandDrawIndexed(uint32 InIndexCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation)
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
// CRHICommandDrawInstanced

DECLARE_RHICOMMAND_CLASS(CRHICommandDrawInstanced)
{
public:
    FORCEINLINE CRHICommandDrawInstanced(uint32 InVertexCountPerInstance, uint32 InInstanceCount, uint32 InStartVertexLocation, uint32 InStartInstanceLocation)
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
// CRHICommandDrawIndexedInstanced

DECLARE_RHICOMMAND_CLASS(CRHICommandDrawIndexedInstanced)
{
public:
    FORCEINLINE CRHICommandDrawIndexedInstanced( uint32 InIndexCountPerInstance
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
// CRHICommandDispatch

DECLARE_RHICOMMAND_CLASS(CRHICommandDispatch)
{
public:
    FORCEINLINE CRHICommandDispatch(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
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
// CRHICommandDispatchRays

DECLARE_RHICOMMAND_CLASS(CRHICommandDispatchRays)
{
public:
    FORCEINLINE CRHICommandDispatchRays(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
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

    CRHIRayTracingScene*         Scene;
    CRHIRayTracingPipelineState* PipelineState;
    uint32                       Width;
    uint32                       Height;
    uint32                       Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandInsertMarker

DECLARE_RHICOMMAND_CLASS(CRHICommandInsertMarker)
{
public:
    FORCEINLINE CRHICommandInsertMarker(const String& InMarker)
        : Marker(InMarker)
    { }

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

DECLARE_RHICOMMAND_CLASS(CRHICommandDebugBreak)
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

DECLARE_RHICOMMAND_CLASS(CRHICommandBeginExternalCapture)
{
public:
    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginExternalCapture();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandEndExternalCapture

DECLARE_RHICOMMAND_CLASS(CRHICommandEndExternalCapture)
{
public:
    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndExternalCapture();
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandPresentViewport

DECLARE_RHICOMMAND_CLASS(CRHICommandPresentViewport)
{
public:
    FORCEINLINE CRHICommandPresentViewport(CRHIViewport* InViewport, bool bInVerticalSync)
        : Viewport(InViewport)
        , bVerticalSync(bInVerticalSync)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.PresentViewport(Viewport, bVerticalSync);
    }

    CRHIViewport* Viewport;
    bool          bVerticalSync;
};
