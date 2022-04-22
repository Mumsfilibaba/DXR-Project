#pragma once
#include "RHITypes.h"
#include "IRHICommandContext.h"
#include "RHIResources.h"

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
    FORCEINLINE CRHICommandBeginTimeStamp(CRHITimeQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginTimeStamp(Query, Index);
    }

    CRHITimeQuery* Query;
    uint32         Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandEndTimeStamp

DECLARE_RHICOMMAND_CLASS(CRHICommandEndTimeStamp)
{
public:
    FORCEINLINE CRHICommandEndTimeStamp(CRHITimeQuery* InQuery, uint32 InIndex)
        : Query(InQuery)
        , Index(InIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndTimeStamp(Query, Index);
    }

    CRHITimeQuery* Query;
    uint32         Index;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearRenderTargetView

DECLARE_RHICOMMAND_CLASS(CRHICommandClearRenderTargetView)
{
public:
    FORCEINLINE CRHICommandClearRenderTargetView(const CRHIRenderTargetView& InRenderTargetView, const TStaticArray<float, 4>& InClearColor)
        : RenderTargetView(InRenderTargetView)
        , ClearColor(InClearColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearRenderTargetView(RenderTargetView, ClearColor);
    }

    CRHIRenderTargetView   RenderTargetView;
    TStaticArray<float, 4> ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearDepthStencilView

DECLARE_RHICOMMAND_CLASS(CRHICommandClearDepthStencilView)
{
public:
    FORCEINLINE CRHICommandClearDepthStencilView(const CRHIDepthStencilView& InDepthStencilView, const float InDepth, const uint8 InStencil)
        : DepthStencilView(InDepthStencilView)
        , Depth(InDepth)
        , Stencil(InStencil)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearDepthStencilView(DepthStencilView, Depth, Stencil);
    }

    CRHIDepthStencilView DepthStencilView;
    const float          Depth;
    const uint8          Stencil;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessTextureFloat

DECLARE_RHICOMMAND_CLASS(CRHICommandClearUnorderedAccessTextureFloat)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessTextureFloat(CRHITexture* InTexture, const TStaticArray<float, 4>& InClearColor)
        : Texture(InTexture)
        , ClearColor(InClearColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessTextureFloat(Texture, ClearColor);
    }

    CRHITexture*           Texture;
    TStaticArray<float, 4> ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessViewFloat

DECLARE_RHICOMMAND_CLASS(CRHICommandClearUnorderedAccessViewFloat)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* InUnorderedAccessView, const TStaticArray<float, 4>& InClearColor)
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor(InClearColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
    }

    CRHIUnorderedAccessView* UnorderedAccessView;
    TStaticArray<float, 4>   ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessTextureUint

DECLARE_RHICOMMAND_CLASS(CRHICommandClearUnorderedAccessTextureUint)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessTextureUint(CRHITexture* InTexture, const TStaticArray<uint32, 4>& InClearColor)
        : Texture(InTexture)
        , ClearColor(InClearColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessTextureUint(Texture, ClearColor);
    }

    CRHITexture*            Texture;
    TStaticArray<uint32, 4> ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandClearUnorderedAccessViewUint

DECLARE_RHICOMMAND_CLASS(CRHICommandClearUnorderedAccessViewUint)
{
public:
    FORCEINLINE CRHICommandClearUnorderedAccessViewUint(CRHIUnorderedAccessView* InUnorderedAccessView, const TStaticArray<uint32, 4>& InClearColor)
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor(InClearColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessViewUint(UnorderedAccessView, ClearColor);
    }

    CRHIUnorderedAccessView* UnorderedAccessView;
    TStaticArray<uint32, 4>  ClearColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandBeginRenderPass

DECLARE_RHICOMMAND_CLASS(CRHICommandBeginRenderPass)
{
public:
    FORCEINLINE CRHICommandBeginRenderPass(const CRHIRenderPassInitializer& InInitializer)
        : Initializer(InInitializer)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginRenderPass(Initializer);
    }

    CRHIRenderPassInitializer Initializer;
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
    FORCEINLINE CRHICommandSetBlendFactor(const TStaticArray<float, 4>& InColor)
        : Color(InColor)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetBlendFactor(Color);
    }

    TStaticArray<float, 4> Color;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetVertexBuffers

DECLARE_RHICOMMAND_CLASS(CRHICommandSetVertexBuffers)
{
public:
    FORCEINLINE CRHICommandSetVertexBuffers(CRHIVertexBuffer* const* InVertexBuffers, uint32 InVertexBufferCount, uint32 InStartSlot)
        : VertexBuffers(InVertexBuffers)
        , VertexBufferCount(InVertexBufferCount)
        , StartSlot(InStartSlot)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetVertexBuffers(VertexBuffers, VertexBufferCount, StartSlot);
    }

    CRHIVertexBuffer* const* VertexBuffers;
    uint32                   VertexBufferCount;
    uint32                   StartSlot;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetIndexBuffer

DECLARE_RHICOMMAND_CLASS(CRHICommandSetIndexBuffer)
{
public:
    FORCEINLINE CRHICommandSetIndexBuffer(CRHIIndexBuffer* InIndexBuffer)
        : IndexBuffer(InIndexBuffer)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetIndexBuffer(IndexBuffer);
    }

    CRHIIndexBuffer* IndexBuffer;
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
    FORCEINLINE CRHICommandSet32BitShaderConstants(CRHIShader* InShader, const SSetShaderConstantsInfo& InShaderConstantsInfo)
        : Shader(InShader)
        , ShaderConstantsInfo(InShaderConstantsInfo)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Set32BitShaderConstants(Shader, ShaderConstantsInfo);
    }

    CRHIShader*             Shader;
    SSetShaderConstantsInfo ShaderConstantsInfo;
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
    FORCEINLINE CRHICommandSetConstantBuffer(CRHIShader* InShader, CRHIConstantBuffer* InConstantBuffer, uint32 InParameterIndex)
        : Shader(InShader)
        , ConstantBuffer(InConstantBuffer)
        , ParameterIndex(InParameterIndex)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetConstantBuffer(Shader, ConstantBuffer, ParameterIndex);
    }

    CRHIShader*         Shader;
    CRHIConstantBuffer* ConstantBuffer;
    uint32              ParameterIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandSetConstantBuffers

DECLARE_RHICOMMAND_CLASS(CRHICommandSetConstantBuffers)
{
public:
    FORCEINLINE CRHICommandSetConstantBuffers( CRHIShader* InShader
                                             , CRHIConstantBuffer* const* InConstantBuffers
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

    CRHIShader*                Shader;
    CRHIConstantBuffer* const* ConstantBuffers;
    uint32                     NumConstantBuffers;
    uint32                     StartParameterIndex;
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
// CRHICommandUpdateBuffer

DECLARE_RHICOMMAND_CLASS(CRHICommandUpdateBuffer)
{
public:
    FORCEINLINE CRHICommandUpdateBuffer(CRHIBuffer* InDst, const void* InSrcData, uint32 InDstOffset, uint32 InSize)
        : Dst(InDst)
        , SrcData(InSrcData)
        , DstOffset(InDstOffset)
        , Size(InSize)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateBuffer(Dst, SrcData, DstOffset, Size);
    }

    CRHIBuffer* Dst;
    const void* SrcData;
    uint32      DstOffset;
    uint32      Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUpdateTexture2D

DECLARE_RHICOMMAND_CLASS(CRHICommandUpdateTexture2D)
{
public:
    FORCEINLINE CRHICommandUpdateTexture2D(CRHITexture2D* InDst, const void* InSrcData, uint16 InWidth, uint16 InHeight, uint16 InMipLevel)
        : Dst(InDst)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevel(InMipLevel)
        , SrcData(InSrcData)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateTexture2D(Dst, SrcData, Width, Height, MipLevel);
    }

    CRHITexture2D* Dst;
    const void*    SrcData;
    uint16         Width;
    uint16         Height;
    uint16         MipLevel;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandUpdateTexture2DArray

DECLARE_RHICOMMAND_CLASS(CRHICommandUpdateTexture2DArray)
{
public:
    FORCEINLINE CRHICommandUpdateTexture2DArray( CRHITexture2DArray* InDst
                                               , const void* InSrcData
                                               , uint16 InWidth
                                               , uint16 InHeight
                                               , uint16 InMipLevel
                                               , uint16 InArrayIndex
                                               , uint16 InNumArraySlices)
        : Dst(InDst)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevel(InMipLevel)
        , SrcData(InSrcData)
        , ArrayIndex(InArrayIndex)
        , NumArraySlices(InNumArraySlices)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateTexture2DArray(Dst, SrcData, Width, Height, MipLevel, ArrayIndex, NumArraySlices);
    }

    CRHITexture2DArray* Dst;
    const void*         SrcData;
    uint16              Width;
    uint16              Height;
    uint16              MipLevel;
    uint16              ArrayIndex;
    uint16              NumArraySlices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandResolveTexture

DECLARE_RHICOMMAND_CLASS(CRHICommandResolveTexture)
{
public:
    FORCEINLINE CRHICommandResolveTexture(CRHITexture* InDst, CRHITexture* InSrc)
        : Dst(InDst)
        , Src(InSrc)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ResolveTexture(Dst, Src);
    }

    CRHITexture* Dst;
    CRHITexture* Src;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandResolveTexture

DECLARE_RHICOMMAND_CLASS(CRHICommandCopyBuffer)
{
public:
    FORCEINLINE CRHICommandCopyBuffer(CRHIBuffer* InDst, CRHIBuffer* InSrc)
        : Dst(InDst)
        , Src(InSrc)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyBuffer(Dst, Src);
    }

    CRHIBuffer* Dst;
    CRHIBuffer* Src;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandCopyBufferRegion

DECLARE_RHICOMMAND_CLASS(CRHICommandCopyBufferRegion)
{
public:
    FORCEINLINE CRHICommandCopyBufferRegion(const SCopyBufferRegionInfo& InCopyBufferInfo)
        : CopyBufferInfo(InCopyBufferInfo)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyBufferRegion(CopyBufferInfo);
    }

    SCopyBufferRegionInfo CopyBufferInfo;
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
// CRHICommandCopyTexture2DRegion

DECLARE_RHICOMMAND_CLASS(CRHICommandCopyTexture2DRegion)
{
public:
    FORCEINLINE CRHICommandCopyTexture2DRegion(CRHITexture2D* InDst, CRHITexture2D* InSrc, const SCopyTexture2DRegionInfo& InCopyInfo)
        : Dst(InDst)
        , Src(InSrc)
        , CopyInfo(InCopyInfo)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTexture2DRegion(Dst, Src, CopyInfo);
    }

    CRHITexture2D*           Dst;
    CRHITexture2D*           Src;
    SCopyTexture2DRegionInfo CopyInfo;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandCopyTexture2DArrayRegion

DECLARE_RHICOMMAND_CLASS(CRHICommandCopyTexture2DArrayRegion)
{
public:
    FORCEINLINE CRHICommandCopyTexture2DArrayRegion( CRHITexture2DArray* InDst
                                                   , CRHITexture2DArray* InSrc
                                                   , const SCopyTexture2DArrayRegion& InCopyInfo)
        : Dst(InDst)
        , Src(InSrc)
        , CopyInfo(InCopyInfo)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTexture2DArrayRegion(Dst, Src, CopyInfo);
    }

    CRHITexture2DArray*       Dst;
    CRHITexture2DArray*       Src;
    SCopyTexture2DArrayRegion CopyInfo;
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
    FORCEINLINE CRHICommandBuildRayTracingGeometry(CRHIRayTracingGeometry* InRayTracingGeometry, const SBuildRayTracingGeometryInfo& InBuildDesc)
        : RayTracingGeometry(InRayTracingGeometry)
        , BuildDesc(InBuildDesc)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildRayTracingGeometry(RayTracingGeometry, BuildDesc);
    }

    CRHIRayTracingGeometry*      RayTracingGeometry;
    SBuildRayTracingGeometryInfo BuildDesc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHICommandBuildRayTracingScene

DECLARE_RHICOMMAND_CLASS(CRHICommandBuildRayTracingScene)
{
public:
    FORCEINLINE CRHICommandBuildRayTracingScene(CRHIRayTracingScene* InRayTracingScene, const SBuildRayTracingSceneInfo& InBuildDesc)
        : RayTracingScene(InRayTracingScene)
        , BuildDesc(InBuildDesc)
    { }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildRayTracingScene(RayTracingScene, BuildDesc);
    }

    CRHIRayTracingScene*      RayTracingScene;
    SBuildRayTracingSceneInfo BuildDesc;
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

    CRHITexture*    Texture;
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

    CRHIBuffer*     Buffer;
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

    CRHICommandDebugBreak() = default;

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

    CRHICommandBeginExternalCapture() = default;

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

    CRHICommandEndExternalCapture() = default;

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
