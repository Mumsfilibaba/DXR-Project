#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Debug.h"
#include "Core/Containers/ArrayView.h"
#include "RHI/RHITypes.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"
#include "RHI/IRHICommandContext.h"

extern RHI_API bool GRHIVerboseEventOutput;

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
    virtual ~TRHICommand() = default;

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

DECLARE_RHICOMMAND(FRHICommandBeginFrame)
{
    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginFrame();
    }
};

DECLARE_RHICOMMAND(FRHICommandEndFrame)
{
    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndFrame();
    }
};

DECLARE_RHICOMMAND(FRHICommandBeginQuery)
{
    FORCEINLINE FRHICommandBeginQuery(FRHIQuery* InQuery)
        : Query(InQuery)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginQuery(Query);
    }

    FRHIQuery* Query;
};

DECLARE_RHICOMMAND(FRHICommandEndQuery)
{
    FORCEINLINE FRHICommandEndQuery(FRHIQuery* InQuery)
        : Query(InQuery)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndQuery(Query);
    }

    FRHIQuery* Query;
};

DECLARE_RHICOMMAND(FRHICommandQueryTimestamp)
{
    FORCEINLINE FRHICommandQueryTimestamp(FRHIQuery* InQuery)
        : Query(InQuery)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.QueryTimestamp(Query);
    }

    FRHIQuery* Query;
};

DECLARE_RHICOMMAND(FRHICommandClearRenderTargetView)
{
    FORCEINLINE FRHICommandClearRenderTargetView(FRHIRenderTargetView* InRenderTargetView, const Vector4& InClearColor)
        : RenderTargetView(InRenderTargetView)
        , ClearColor(InClearColor)
    {
        CHECK(InRenderTargetView != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearRenderTargetView(RenderTargetView, ClearColor);
    }

    FRHIRenderTargetView* RenderTargetView;
    Vector4               ClearColor;
};

DECLARE_RHICOMMAND(FRHICommandClearDepthStencilView)
{
    FORCEINLINE FRHICommandClearDepthStencilView(FRHIDepthStencilView* InDepthStencilView, const float InDepth, const uint8 InStencil)
        : DepthStencilView(InDepthStencilView)
        , Depth(InDepth)
        , Stencil(InStencil)
    {
        CHECK(InDepthStencilView != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearDepthStencilView(DepthStencilView, Depth, Stencil);
    }

    FRHIDepthStencilView* DepthStencilView;
    const float           Depth;
    const uint8           Stencil;
};

DECLARE_RHICOMMAND(FRHICommandClearUnorderedAccessViewFloat)
{
    FORCEINLINE FRHICommandClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* InUnorderedAccessView, const Vector4& InClearColor)
        : UnorderedAccessView(InUnorderedAccessView)
        , ClearColor(InClearColor)
    {
        CHECK(InUnorderedAccessView != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessViewFloat(UnorderedAccessView, ClearColor);
    }

    FRHIUnorderedAccessView* UnorderedAccessView;
    Vector4                  ClearColor;
};

DECLARE_RHICOMMAND(FRHICommandClearUnorderedAccessViewUint)
{
    FORCEINLINE FRHICommandClearUnorderedAccessViewUint(FRHIUnorderedAccessView* InUnorderedAccessView, const uint32 InValues[4])
        : UnorderedAccessView(InUnorderedAccessView)
    {
        CHECK(InUnorderedAccessView != nullptr);
        Memory::Memcpy(Values, InValues, sizeof(Values));
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ClearUnorderedAccessViewUint(UnorderedAccessView, Values);
    }

    FRHIUnorderedAccessView* UnorderedAccessView;
    uint32                   Values[4];
};

DECLARE_RHICOMMAND(FRHICommandBeginRenderPass)
{
    FRHICommandBeginRenderPass(const FRHIBeginRenderPassDesc& InBeginRenderPassDesc)
        : BeginRenderPassDesc(InBeginRenderPassDesc)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BeginRenderPass(BeginRenderPassDesc);
    }

    FRHIBeginRenderPassDesc BeginRenderPassDesc;
};

DECLARE_RHICOMMAND(FRHICommandEndRenderPass)
{
    FRHICommandEndRenderPass() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.EndRenderPass();
    }
};

DECLARE_RHICOMMAND(FRHICommandSetViewport)
{
    FORCEINLINE FRHICommandSetViewport(const FViewportRegion& InViewportRegion)
        : ViewportRegion(InViewportRegion)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetViewport(ViewportRegion);
    }

    FViewportRegion ViewportRegion;
};

DECLARE_RHICOMMAND(FRHICommandSetScissorRect)
{
    FORCEINLINE FRHICommandSetScissorRect(const FScissorRegion& InScissorRegion)
        : ScissorRegion(InScissorRegion)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetScissorRect(ScissorRegion);
    }

    FScissorRegion ScissorRegion;
};

DECLARE_RHICOMMAND(FRHICommandSetBlendFactor)
{
    FORCEINLINE FRHICommandSetBlendFactor(const Vector4& InColor)
        : Color(InColor)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetBlendFactor(Color);
    }

    Vector4 Color;
};

DECLARE_RHICOMMAND(FRHICommandSetStencilRef)
{
    FORCEINLINE FRHICommandSetStencilRef(uint32 InStencilRef)
        : StencilRef(InStencilRef)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetStencilRef(StencilRef);
    }

    uint32 StencilRef;
};

DECLARE_RHICOMMAND(FRHICommandSetDepthBias)
{
    FORCEINLINE FRHICommandSetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias)
        : DepthBias(InDepthBias)
        , DepthBiasClamp(InDepthBiasClamp)
        , SlopeScaledDepthBias(InSlopeScaledDepthBias)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetDepthBias(DepthBias, DepthBiasClamp, SlopeScaledDepthBias);
    }

    float DepthBias;
    float DepthBiasClamp;
    float SlopeScaledDepthBias;
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
        CommandContext.SetVertexBuffers(VertexBuffers, StartSlot);
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
        CommandContext.SetIndexBuffer(IndexBuffer, IndexFormat);
    }

    FRHIBuffer*  IndexBuffer;
    EIndexFormat IndexFormat;
};

DECLARE_RHICOMMAND(FRHICommandSetStreamOutputTargets)
{
    FORCEINLINE FRHICommandSetStreamOutputTargets(const TArrayView<FRHIBuffer* const> InBuffers, const uint64* InOffsets)
        : Buffers(InBuffers)
        , Offsets(InOffsets)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetStreamOutputTargets(Buffers, Offsets);
    }

    TArrayView<FRHIBuffer* const> Buffers;
    const uint64*                 Offsets;
};

DECLARE_RHICOMMAND(FRHICommandSetGraphicsPipelineState)
{
    FORCEINLINE FRHICommandSetGraphicsPipelineState(FRHIGraphicsPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetGraphicsPipelineState(PipelineState);
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
        CommandContext.SetComputePipelineState(PipelineState);
    }

    FRHIComputePipelineState* PipelineState;
};

DECLARE_RHICOMMAND(FRHICommandSetMeshletPipelineState)
{
    FORCEINLINE FRHICommandSetMeshletPipelineState(FRHIMeshletPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetMeshletPipelineState(PipelineState);
    }

    FRHIMeshletPipelineState* PipelineState;
};

DECLARE_RHICOMMAND(FRHICommandSetShaderConstants)
{
    FORCEINLINE FRHICommandSetShaderConstants(FRHIShader* InShader, const void* InShaderConstants, uint32 InNumShaderConstants)
        : Shader(InShader)
        , ShaderConstants(InShaderConstants)
        , NumShaderConstants(InNumShaderConstants)
    { 
        CHECK(InNumShaderConstants <= RHI_MAX_SHADER_CONSTANTS);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderConstants(Shader, ShaderConstants, NumShaderConstants);
    }

    FRHIShader* Shader;
    const void* ShaderConstants;
    uint32      NumShaderConstants;
};

DECLARE_RHICOMMAND(FRHICommandSetShaderResourceView)
{
    FORCEINLINE FRHICommandSetShaderResourceView(FRHIShader* InShader, FRHIShaderResourceView* InShaderResourceView, uint32 InRegisterIndex)
        : Shader(InShader)
        , ShaderResourceView(InShaderResourceView)
        , RegisterIndex(InRegisterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderResourceView(Shader, ShaderResourceView, RegisterIndex);
    }

    FRHIShader*             Shader;
    FRHIShaderResourceView* ShaderResourceView;
    uint32                  RegisterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetShaderResourceViews)
{
    FORCEINLINE FRHICommandSetShaderResourceViews(FRHIShader* InShader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 InStartRegisterIndex)
        : Shader(InShader)
        , ShaderResourceViews(InShaderResourceViews)
        , StartRegisterIndex(InStartRegisterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetShaderResourceViews(Shader, ShaderResourceViews, StartRegisterIndex);
    }

    FRHIShader*                               Shader;
    TArrayView<FRHIShaderResourceView* const> ShaderResourceViews;
    uint32                                    StartRegisterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetUnorderedAccessView)
{
    FORCEINLINE FRHICommandSetUnorderedAccessView(FRHIShader* InShader, FRHIUnorderedAccessView* InUnorderedAccessView, uint32 InRegisterIndex)
        : Shader(InShader)
        , UnorderedAccessView(InUnorderedAccessView)
        , RegisterIndex(InRegisterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetUnorderedAccessView(Shader, UnorderedAccessView, RegisterIndex);
    }

    FRHIShader*              Shader;
    FRHIUnorderedAccessView* UnorderedAccessView;
    uint32                   RegisterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetUnorderedAccessViews)
{
    FORCEINLINE FRHICommandSetUnorderedAccessViews(FRHIShader* InShader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 InStartRegisterIndex)
        : Shader(InShader)
        , UnorderedAccessViews(InUnorderedAccessViews)
        , StartRegisterIndex(InStartRegisterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetUnorderedAccessViews(Shader, UnorderedAccessViews, StartRegisterIndex);
    }

    FRHIShader*                                Shader;
    TArrayView<FRHIUnorderedAccessView* const> UnorderedAccessViews;
    uint32                                     StartRegisterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetConstantBuffer)
{
    FORCEINLINE FRHICommandSetConstantBuffer(FRHIShader* InShader, FRHIBuffer* InConstantBuffer, uint32 InRegisterIndex)
        : Shader(InShader)
        , ConstantBuffer(InConstantBuffer)
        , RegisterIndex(InRegisterIndex)
    {
        if (ConstantBuffer)
        {
            CHECK(ConstantBuffer->GetDesc().IsConstantBuffer());
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetConstantBuffer(Shader, ConstantBuffer, RegisterIndex);
    }

    FRHIShader* Shader;
    FRHIBuffer* ConstantBuffer;
    uint32      RegisterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetConstantBuffers)
{
    FORCEINLINE FRHICommandSetConstantBuffers(FRHIShader* InShader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 InStartRegisterIndex)
        : Shader(InShader)
        , ConstantBuffers(InConstantBuffers)
        , StartRegisterIndex(InStartRegisterIndex)
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
        CommandContext.SetConstantBuffers(Shader, ConstantBuffers, StartRegisterIndex);
    }

    FRHIShader*                   Shader;
    TArrayView<FRHIBuffer* const> ConstantBuffers;
    uint32                        StartRegisterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetSamplerState)
{
    FORCEINLINE FRHICommandSetSamplerState(FRHIShader* InShader, FRHISamplerState* InSamplerState, uint32 InRegisterIndex)
        : Shader(InShader)
        , SamplerState(InSamplerState)
        , RegisterIndex(InRegisterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetSamplerState(Shader, SamplerState, RegisterIndex);
    }

    FRHIShader*       Shader;
    FRHISamplerState* SamplerState;
    uint32            RegisterIndex;
};

DECLARE_RHICOMMAND(FRHICommandSetSamplerStates)
{
    FORCEINLINE FRHICommandSetSamplerStates(FRHIShader* InShader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 InStartRegisterIndex)
        : Shader(InShader)
        , SamplerStates(InSamplerStates)
        , StartRegisterIndex(InStartRegisterIndex)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetSamplerStates(Shader, SamplerStates, StartRegisterIndex);
    }

    FRHIShader*                         Shader;
    TArrayView<FRHISamplerState* const> SamplerStates;
    uint32                              StartRegisterIndex;
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
        CommandContext.UpdateBuffer(Dst, BufferRegion, SrcData);
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
        CommandContext.UpdateTexture2D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch);
    }

    FRHITexture*     Dst;
    FTextureRegion2D TextureRegion;
    uint32           MipLevel;
    const void*      SrcData;
    uint32           SrcRowPitch;
};

DECLARE_RHICOMMAND(FRHICommandUpdateTexture3D)
{
    FORCEINLINE FRHICommandUpdateTexture3D(
        FRHITexture*            InDst,
        const FTextureRegion3D& InTextureRegion,
        uint32                  InMipLevel,
        const void*             InSrcData,
        uint32                  InSrcRowPitch,
        uint32                  InSrcDepthPitch)
        : Dst(InDst)
        , TextureRegion(InTextureRegion)
        , MipLevel(InMipLevel)
        , SrcData(InSrcData)
        , SrcRowPitch(InSrcRowPitch)
        , SrcDepthPitch(InSrcDepthPitch)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UpdateTexture3D(Dst, TextureRegion, MipLevel, SrcData, SrcRowPitch, SrcDepthPitch);
    }

    FRHITexture*     Dst;
    FTextureRegion3D TextureRegion;
    uint32           MipLevel;
    const void*      SrcData;
    uint32           SrcRowPitch;
    uint32           SrcDepthPitch;
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
        CommandContext.ResolveTexture(Dst, Src);
    }

    FRHITexture* Dst;
    FRHITexture* Src;
};

DECLARE_RHICOMMAND(FRHICommandCopyBuffer)
{
    FORCEINLINE FRHICommandCopyBuffer(FRHIBuffer* InDst, FRHIBuffer* InSrc, const FRHIBufferCopyDesc& InCopyBufferDesc)
        : Dst(InDst)
        , Src(InSrc)
        , CopyBufferDesc(InCopyBufferDesc)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyBuffer(Dst, Src, CopyBufferDesc);
    }

    FRHIBuffer*        Dst;
    FRHIBuffer*        Src;
    FRHIBufferCopyDesc CopyBufferDesc;
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
        CommandContext.CopyTexture(Destination, Source);
    }

    FRHITexture* Destination;
    FRHITexture* Source;
};

DECLARE_RHICOMMAND(FRHICommandCopyTextureRegion)
{
    FORCEINLINE FRHICommandCopyTextureRegion(FRHITexture* InDst, FRHITexture* InSrc, const FRHITextureCopyDesc& InCopyDesc)
        : Dst(InDst)
        , Src(InSrc)
        , CopyDesc(InCopyDesc)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTextureRegion(Dst, Src, CopyDesc);
    }

    FRHITexture*        Dst;
    FRHITexture*        Src;
    FRHITextureCopyDesc CopyDesc;
};

DECLARE_RHICOMMAND(FRHICommandCopyTextureRegionToBuffer)
{
    FORCEINLINE FRHICommandCopyTextureRegionToBuffer(FRHIBuffer* InDst, uint64 InDstOffset, FRHITexture* InSrc, const FTextureRegion2D& InSrcRegion, uint32 InSrcMipLevel)
        : Dst(InDst)
        , DstOffset(InDstOffset)
        , Src(InSrc)
        , SrcRegion(InSrcRegion)
        , SrcMipLevel(InSrcMipLevel)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTextureRegionToBuffer(Dst, DstOffset, Src, SrcRegion, SrcMipLevel);
    }

    FRHIBuffer*      Dst;
    uint64           DstOffset;
    FRHITexture*     Src;
    FTextureRegion2D SrcRegion;
    uint32           SrcMipLevel;
};

DECLARE_RHICOMMAND(FRHICommandCopyTextureSubresourceToBuffer)
{
    FORCEINLINE FRHICommandCopyTextureSubresourceToBuffer(FRHIBuffer* InDst, uint64 InDstOffset, FRHITexture* InSrc, const FTextureRegion3D& InSrcRegion, uint32 InSrcMipLevel, uint32 InSrcArraySlice)
        : Dst(InDst)
        , DstOffset(InDstOffset)
        , Src(InSrc)
        , SrcRegion(InSrcRegion)
        , SrcMipLevel(InSrcMipLevel)
        , SrcArraySlice(InSrcArraySlice)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyTextureSubresourceToBuffer(Dst, DstOffset, Src, SrcRegion, SrcMipLevel, SrcArraySlice);
    }

    FRHIBuffer*      Dst;
    uint64           DstOffset;
    FRHITexture*     Src;
    FTextureRegion3D SrcRegion;
    uint32           SrcMipLevel;
    uint32           SrcArraySlice;
};

DECLARE_RHICOMMAND(FRHICommandWriteFence)
{
    FORCEINLINE FRHICommandWriteFence(FRHIFence* InFence)
        : Fence(InFence)
    {
        CHECK(Fence != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.WriteFence(Fence);
    }

    FRHIFence* Fence;
};

DECLARE_RHICOMMAND(FRHICommandDiscardContents)
{
    FORCEINLINE FRHICommandDiscardContents(FRHITexture* InTexture)
        : Texture(InTexture)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DiscardContents(Texture);
    }

    FRHITexture* Texture;
};

DECLARE_RHICOMMAND(FRHICommandBuildSceneAccelerationStructure)
{
    FORCEINLINE FRHICommandBuildSceneAccelerationStructure(FRHISceneAccelerationStructure* InRayTracingScene, const FRHISceneAccelerationStructureBuildDesc& InBuildDesc)
        : RayTracingScene(InRayTracingScene)
        , BuildDesc(InBuildDesc)
    {
        CHECK(RayTracingScene != nullptr);
        CHECK(!BuildDesc.bUpdate || (BuildDesc.bUpdate && IsEnumFlagSet(RayTracingScene->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate)));
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildSceneAccelerationStructure(RayTracingScene, BuildDesc);
    }

    FRHISceneAccelerationStructure*         RayTracingScene;
    FRHISceneAccelerationStructureBuildDesc BuildDesc;
};

DECLARE_RHICOMMAND(FRHICommandBuildGeometryAccelerationStructure)
{
    FORCEINLINE FRHICommandBuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* InRayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& InBuildDesc)
        : RayTracingGeometry(InRayTracingGeometry)
        , BuildDesc(InBuildDesc)
    { 
        CHECK(RayTracingGeometry != nullptr);
        CHECK(!BuildDesc.bUpdate || (BuildDesc.bUpdate && IsEnumFlagSet(RayTracingGeometry->GetFlags(), EAccelerationStructureBuildFlags::AllowUpdate)));
        CHECK(BuildDesc.VertexBuffer && BuildDesc.VertexBuffer->GetDesc().IsVertexBuffer());
        CHECK(BuildDesc.IndexBuffer  && BuildDesc.IndexBuffer->GetDesc().IsIndexBuffer());
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildGeometryAccelerationStructure(RayTracingGeometry, BuildDesc);
    }

    FRHIGeometryAccelerationStructure*         RayTracingGeometry;
    FRHIGeometryAccelerationStructureBuildDesc BuildDesc;
};

DECLARE_RHICOMMAND(FRHICommandTransitionBarrier)
{
    FORCEINLINE FRHICommandTransitionBarrier(const TArrayView<const FRHITransitionBarrierDesc> InTransitionDescs)
        : TransitionDescs(InTransitionDescs)
    {
        for (const FRHITransitionBarrierDesc& Desc : TransitionDescs)
        {
            if (Desc.IsTexture())
            {
                CHECK(Desc.Texture.Resource != nullptr);
            }
            else
            {
                CHECK(Desc.Buffer.Resource != nullptr);
            }

            CHECK(!(Desc.IsSplitBegin() && Desc.IsSplitEnd()));
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.TransitionBarrier(TransitionDescs);
    }

    TArrayView<const FRHITransitionBarrierDesc> TransitionDescs;
};

DECLARE_RHICOMMAND(FRHICommandUnorderedAccessBarrier)
{
    FORCEINLINE FRHICommandUnorderedAccessBarrier(const TArrayView<const FRHIUnorderedAccessBarrierDesc> InBarrierDescs)
        : BarrierDescs(InBarrierDescs)
    {
        for (const FRHIUnorderedAccessBarrierDesc& Desc : BarrierDescs)
        {
            if (Desc.IsTexture())
            {
                CHECK(Desc.Texture.Resource != nullptr);
            }
            else
            {
                CHECK(Desc.Buffer.Resource != nullptr);
            }
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.UnorderedAccessBarrier(BarrierDescs);
    }

    TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs;
};

DECLARE_RHICOMMAND(FRHICommandDraw)
{
    FORCEINLINE FRHICommandDraw(uint32 InVertexCount, uint32 InStartVertexLocation)
        : VertexCount(InVertexCount)
        , StartVertexLocation(InStartVertexLocation)
    {
        CHECK(VertexCount > 0);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Draw(VertexCount, StartVertexLocation);
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
        CHECK(IndexCount > 0);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
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
        CHECK(VertexCountPerInstance > 0 && InstanceCount > 0);
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

DECLARE_RHICOMMAND(FRHICommandDrawIndexedInstanced)
{
    FORCEINLINE FRHICommandDrawIndexedInstanced(uint32 InIndexCountPerInstance, uint32 InInstanceCount, uint32 InStartIndexLocation, uint32 InBaseVertexLocation, uint32 InStartInstanceLocation)
        : IndexCountPerInstance(InIndexCountPerInstance)
        , InstanceCount(InInstanceCount)
        , StartIndexLocation(InStartIndexLocation)
        , BaseVertexLocation(InBaseVertexLocation)
        , StartInstanceLocation(InStartInstanceLocation)
    {
        CHECK(IndexCountPerInstance > 0 && InstanceCount > 0);
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

DECLARE_RHICOMMAND(FRHICommandDispatch)
{
    FORCEINLINE FRHICommandDispatch(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
        : ThreadGroupCountX(InThreadGroupCountX)
        , ThreadGroupCountY(InThreadGroupCountY)
        , ThreadGroupCountZ(InThreadGroupCountZ)
    {
        CHECK(ThreadGroupCountX > 0 || ThreadGroupCountY > 0 || ThreadGroupCountZ > 0);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    uint32 ThreadGroupCountX;
    uint32 ThreadGroupCountY;
    uint32 ThreadGroupCountZ;
};

DECLARE_RHICOMMAND(FRHICommandDispatchMesh)
{
    FORCEINLINE FRHICommandDispatchMesh(uint32 InThreadGroupCountX, uint32 InThreadGroupCountY, uint32 InThreadGroupCountZ)
        : ThreadGroupCountX(InThreadGroupCountX)
        , ThreadGroupCountY(InThreadGroupCountY)
        , ThreadGroupCountZ(InThreadGroupCountZ)
    {
        CHECK(ThreadGroupCountX > 0 || ThreadGroupCountY > 0 || ThreadGroupCountZ > 0);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    }

    uint32 ThreadGroupCountX;
    uint32 ThreadGroupCountY;
    uint32 ThreadGroupCountZ;
};

DECLARE_RHICOMMAND(FRHICommandDrawIndirect)
{
    FORCEINLINE FRHICommandDrawIndirect(FRHIBuffer* InArgumentBuffer, uint64 InArgumentBufferOffset, uint32 InCommandCount)
        : ArgumentBuffer(InArgumentBuffer)
        , ArgumentBufferOffset(InArgumentBufferOffset)
        , CommandCount(InCommandCount)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawIndirect(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
    }

    FRHIBuffer* ArgumentBuffer;
    uint64      ArgumentBufferOffset;
    uint32      CommandCount;
};

DECLARE_RHICOMMAND(FRHICommandDrawIndirectCount)
{
    FORCEINLINE FRHICommandDrawIndirectCount(FRHIBuffer* InArgumentBuffer, uint64 InArgumentBufferOffset, FRHIBuffer* InCountBuffer, uint64 InCountBufferOffset, uint32 InMaxCommandCount)
        : ArgumentBuffer(InArgumentBuffer)
        , ArgumentBufferOffset(InArgumentBufferOffset)
        , CountBuffer(InCountBuffer)
        , CountBufferOffset(InCountBufferOffset)
        , MaxCommandCount(InMaxCommandCount)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawIndirectCount(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
    }

    FRHIBuffer* ArgumentBuffer;
    uint64      ArgumentBufferOffset;
    FRHIBuffer* CountBuffer;
    uint64      CountBufferOffset;
    uint32      MaxCommandCount;
};

DECLARE_RHICOMMAND(FRHICommandDrawIndexedIndirect)
{
    FORCEINLINE FRHICommandDrawIndexedIndirect(FRHIBuffer* InArgumentBuffer, uint64 InArgumentBufferOffset, uint32 InCommandCount)
        : ArgumentBuffer(InArgumentBuffer)
        , ArgumentBufferOffset(InArgumentBufferOffset)
        , CommandCount(InCommandCount)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawIndexedIndirect(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
    }

    FRHIBuffer* ArgumentBuffer;
    uint64      ArgumentBufferOffset;
    uint32      CommandCount;
};

DECLARE_RHICOMMAND(FRHICommandDrawIndexedIndirectCount)
{
    FORCEINLINE FRHICommandDrawIndexedIndirectCount(FRHIBuffer* InArgumentBuffer, uint64 InArgumentBufferOffset, FRHIBuffer* InCountBuffer, uint64 InCountBufferOffset, uint32 InMaxCommandCount)
        : ArgumentBuffer(InArgumentBuffer)
        , ArgumentBufferOffset(InArgumentBufferOffset)
        , CountBuffer(InCountBuffer)
        , CountBufferOffset(InCountBufferOffset)
        , MaxCommandCount(InMaxCommandCount)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DrawIndexedIndirectCount(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
    }

    FRHIBuffer* ArgumentBuffer;
    uint64      ArgumentBufferOffset;
    FRHIBuffer* CountBuffer;
    uint64      CountBufferOffset;
    uint32      MaxCommandCount;
};

DECLARE_RHICOMMAND(FRHICommandDispatchIndirect)
{
    FORCEINLINE FRHICommandDispatchIndirect(FRHIBuffer* InArgumentBuffer, uint64 InArgumentBufferOffset)
        : ArgumentBuffer(InArgumentBuffer)
        , ArgumentBufferOffset(InArgumentBufferOffset)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DispatchIndirect(ArgumentBuffer, ArgumentBufferOffset);
    }

    FRHIBuffer* ArgumentBuffer;
    uint64      ArgumentBufferOffset;
};

DECLARE_RHICOMMAND(FRHICommandDispatchMeshIndirect)
{
    FORCEINLINE FRHICommandDispatchMeshIndirect(FRHIBuffer* InArgumentBuffer, uint64 InArgumentBufferOffset, uint32 InCommandCount)
        : ArgumentBuffer(InArgumentBuffer)
        , ArgumentBufferOffset(InArgumentBufferOffset)
        , CommandCount(InCommandCount)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DispatchMeshIndirect(ArgumentBuffer, ArgumentBufferOffset, CommandCount);
    }

    FRHIBuffer* ArgumentBuffer;
    uint64      ArgumentBufferOffset;
    uint32      CommandCount;
};

DECLARE_RHICOMMAND(FRHICommandDispatchMeshIndirectCount)
{
    FORCEINLINE FRHICommandDispatchMeshIndirectCount(FRHIBuffer* InArgumentBuffer, uint64 InArgumentBufferOffset, FRHIBuffer* InCountBuffer, uint64 InCountBufferOffset, uint32 InMaxCommandCount)
        : ArgumentBuffer(InArgumentBuffer)
        , ArgumentBufferOffset(InArgumentBufferOffset)
        , CountBuffer(InCountBuffer)
        , CountBufferOffset(InCountBufferOffset)
        , MaxCommandCount(InMaxCommandCount)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DispatchMeshIndirectCount(ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset, MaxCommandCount);
    }

    FRHIBuffer* ArgumentBuffer;
    uint64      ArgumentBufferOffset;
    FRHIBuffer* CountBuffer;
    uint64      CountBufferOffset;
    uint32      MaxCommandCount;
};

DECLARE_RHICOMMAND(FRHICommandSetHitRecordLocalShaderBindings)
{
    FORCEINLINE FRHICommandSetHitRecordLocalShaderBindings(FRHIShaderBindingTable* InShaderBindingTable, ERayTracingShaderRecordKind InRecordKind, uint32 InRecordIndex, const FRHIHitGroupLocalShaderBinding* InBindings, uint32 InNumBindings)
        : ShaderBindingTable(InShaderBindingTable)
        , RecordKind(InRecordKind)
        , RecordIndex(InRecordIndex)
        , Bindings()
    {
        Bindings.Reserve(int32(InNumBindings));

        for (uint32 i = 0; i < InNumBindings; ++i)
        {
            Bindings.Emplace(InBindings[i]);
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetHitRecordLocalShaderBindings(ShaderBindingTable, RecordKind, RecordIndex, Bindings.Data(), uint32(Bindings.Size()));
    }

    FRHIShaderBindingTable*                ShaderBindingTable;
    ERayTracingShaderRecordKind            RecordKind;
    uint32                                 RecordIndex;
    TArray<FRHIHitGroupLocalShaderBinding> Bindings;
};

DECLARE_RHICOMMAND(FRHICommandBuildShaderBindingTable)
{
    FORCEINLINE FRHICommandBuildShaderBindingTable(FRHIShaderBindingTable* InShaderBindingTable)
        : ShaderBindingTable(InShaderBindingTable)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildShaderBindingTable(ShaderBindingTable);
    }

    FRHIShaderBindingTable* ShaderBindingTable;
};

DECLARE_RHICOMMAND(FRHICommandResetShaderBindingTable)
{
    FORCEINLINE FRHICommandResetShaderBindingTable(FRHIShaderBindingTable* InShaderBindingTable)
        : ShaderBindingTable(InShaderBindingTable)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ResetShaderBindingTable(ShaderBindingTable);
    }

    FRHIShaderBindingTable* ShaderBindingTable;
};

DECLARE_RHICOMMAND(FRHICommandSetRayTracingPipelineState)
{
    FORCEINLINE FRHICommandSetRayTracingPipelineState(FRHIRayTracingPipelineState* InPipelineState)
        : PipelineState(InPipelineState)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SetRayTracingPipelineState(PipelineState);
    }

    FRHIRayTracingPipelineState* PipelineState;
};

DECLARE_RHICOMMAND(FRHICommandDispatchRaysShaderBindingTable)
{
    FORCEINLINE FRHICommandDispatchRaysShaderBindingTable(FRHIShaderBindingTable* InShaderBindingTable, uint32 InWidth, uint32 InHeight, uint32 InDepth)
        : ShaderBindingTable(InShaderBindingTable)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
        CHECK(Width > 0 || Height > 0 || Depth > 0);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DispatchRays(ShaderBindingTable, Width, Height, Depth);
    }

    FRHIShaderBindingTable*      ShaderBindingTable;
    uint32                       Width;
    uint32                       Height;
    uint32                       Depth;
};

DECLARE_RHICOMMAND(FRHICommandDispatchRaysIndirect)
{
    FORCEINLINE FRHICommandDispatchRaysIndirect(FRHIShaderBindingTable* InShaderBindingTable, FRHIBuffer* InArgumentBuffer, uint64 InArgumentBufferOffset)
        : ShaderBindingTable(InShaderBindingTable)
        , ArgumentBuffer(InArgumentBuffer)
        , ArgumentBufferOffset(InArgumentBufferOffset)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DispatchRaysIndirect(ShaderBindingTable, ArgumentBuffer, ArgumentBufferOffset);
    }

    FRHIShaderBindingTable*      ShaderBindingTable;
    FRHIBuffer*                  ArgumentBuffer;
    uint64                       ArgumentBufferOffset;
};

DECLARE_RHICOMMAND(FRHICommandBuildOpacityMicromap)
{
    FORCEINLINE FRHICommandBuildOpacityMicromap(FRHIOpacityMicromap* InOpacityMicromap, const FRHIOpacityMicromapBuildDesc& InBuildDesc)
        : OpacityMicromap(InOpacityMicromap)
        , BuildDesc(InBuildDesc)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.BuildOpacityMicromap(OpacityMicromap, BuildDesc);
    }

    FRHIOpacityMicromap*         OpacityMicromap;
    FRHIOpacityMicromapBuildDesc BuildDesc;
};

DECLARE_RHICOMMAND(FRHICommandExecuteIndirectRayTracingAccelerationStructureOperations)
{
    FORCEINLINE FRHICommandExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* InOperations, uint32 InNumOperations)
        : Operations()
    {
        Operations.Reserve(int32(InNumOperations));
        for (uint32 Index = 0; Index < InNumOperations; ++Index)
        {
            Operations.Emplace(InOperations[Index]);
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ExecuteIndirectRayTracingAccelerationStructureOperations(Operations.Data(), uint32(Operations.Size()));
    }

    TArray<FRHIRayTracingAccelerationStructureOperationDesc> Operations;
};

DECLARE_RHICOMMAND(FRHICommandWriteAccelerationStructurePostBuildInfo)
{
    FORCEINLINE FRHICommandWriteAccelerationStructurePostBuildInfo(FRHIBuffer* InDestinationBuffer, uint64 InDestinationOffset, EAccelerationStructurePostBuildInfoType InInfoType, FRHIRayTracingAccelerationStructure* const* InSources, uint32 InNumSources)
        : DestinationBuffer(InDestinationBuffer)
        , DestinationOffset(InDestinationOffset)
        , InfoType(InInfoType)
        , Sources()
    {
        Sources.Reserve(int32(InNumSources));
        for (uint32 Index = 0; Index < InNumSources; ++Index)
        {
            Sources.Emplace(InSources[Index]);
        }
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.WriteAccelerationStructurePostBuildInfo(DestinationBuffer, DestinationOffset, InfoType, Sources.Data(), uint32(Sources.Size()));
    }

    FRHIBuffer*                                  DestinationBuffer;
    uint64                                       DestinationOffset;
    EAccelerationStructurePostBuildInfoType      InfoType;
    TArray<FRHIRayTracingAccelerationStructure*> Sources;
};

DECLARE_RHICOMMAND(FRHICommandCopyAccelerationStructure)
{
    FORCEINLINE FRHICommandCopyAccelerationStructure(FRHIRayTracingAccelerationStructure* InDestination, FRHIRayTracingAccelerationStructure* InSource, EAccelerationStructureCopyMode InCopyMode)
        : Destination(InDestination)
        , Source(InSource)
        , CopyMode(InCopyMode)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CopyAccelerationStructure(Destination, Source, CopyMode);
    }

    FRHIRayTracingAccelerationStructure* Destination;
    FRHIRayTracingAccelerationStructure* Source;
    EAccelerationStructureCopyMode       CopyMode;
};

DECLARE_RHICOMMAND(FRHICommandCompactAccelerationStructure)
{
    FORCEINLINE FRHICommandCompactAccelerationStructure(FRHIRayTracingAccelerationStructure* InAccelerationStructure, uint64 InCompactedSizeInBytes)
        : AccelerationStructure(InAccelerationStructure)
        , CompactedSizeInBytes(InCompactedSizeInBytes)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.CompactAccelerationStructure(AccelerationStructure, CompactedSizeInBytes);
    }

    FRHIRayTracingAccelerationStructure* AccelerationStructure;
    uint64                               CompactedSizeInBytes;
};

DECLARE_RHICOMMAND(FRHICommandSerializeAccelerationStructure)
{
    FORCEINLINE FRHICommandSerializeAccelerationStructure(FRHIBuffer* InDestinationBuffer, uint64 InDestinationOffset, FRHIRayTracingAccelerationStructure* InSource)
        : DestinationBuffer(InDestinationBuffer)
        , DestinationOffset(InDestinationOffset)
        , Source(InSource)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.SerializeAccelerationStructure(Source, DestinationBuffer, DestinationOffset);
    }

    FRHIBuffer*                          DestinationBuffer;
    uint64                               DestinationOffset;
    FRHIRayTracingAccelerationStructure* Source;
};

DECLARE_RHICOMMAND(FRHICommandDeserializeAccelerationStructure)
{
    FORCEINLINE FRHICommandDeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* InDestination, FRHIBuffer* InSourceBuffer, uint64 InSourceOffset)
        : Destination(InDestination)
        , SourceBuffer(InSourceBuffer)
        , SourceOffset(InSourceOffset)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.DeserializeAccelerationStructure(Destination, SourceBuffer, SourceOffset);
    }

    FRHIRayTracingAccelerationStructure* Destination;
    FRHIBuffer*                          SourceBuffer;
    uint64                               SourceOffset;
};

DECLARE_RHICOMMAND(FRHICommandPushEvent)
{
    FORCEINLINE FRHICommandPushEvent(const StringView& InName)
        : Name(InName)
    {
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        if (GRHIVerboseEventOutput && Debug::IsDebuggerPresent())
        {
            Debug::OutputDebugString(String(Name) + '\n');
        }

        CommandContext.PushEvent(Name);
    }

    StringView Name;
};

DECLARE_RHICOMMAND(FRHICommandPopEvent)
{
    FRHICommandPopEvent() = default;

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.PopEvent();
    }
};

DECLARE_RHICOMMAND(FRHICommandDebugBreak)
{
    FRHICommandDebugBreak() = default;

    FORCEINLINE void Execute(IRHICommandContext&)
    {
        if (Debug::IsDebuggerPresent())
        {
            DEBUG_BREAK();
        }
    }
};

DECLARE_RHICOMMAND(FRHICommandPresentSwapChain)
{
    FORCEINLINE FRHICommandPresentSwapChain(FRHISwapChain* InSwapChain, bool bInVerticalSync)
        : SwapChain(InSwapChain)
        , bVerticalSync(bInVerticalSync)
    {
        CHECK(InSwapChain != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.PresentSwapChain(SwapChain, bVerticalSync);
    }

    FRHISwapChain* SwapChain;
    bool           bVerticalSync;
};

DECLARE_RHICOMMAND(FRHICommandResizeSwapChain)
{
    FORCEINLINE FRHICommandResizeSwapChain(FRHISwapChain* InSwapChain, uint32 InWidth, uint32 InHeight, EFormat InFormat, EColorSpace InColorSpace)
        : SwapChain(InSwapChain)
        , Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
        , ColorSpace(InColorSpace)
    {
        CHECK(SwapChain != nullptr);
    }

    FORCEINLINE void Execute(IRHICommandContext& CommandContext)
    {
        CommandContext.ResizeSwapChain(SwapChain, Width, Height, Format, ColorSpace);
    }

    FRHISwapChain* SwapChain;
    uint32         Width;
    uint32         Height;
    EFormat        Format;
    EColorSpace    ColorSpace;
};
