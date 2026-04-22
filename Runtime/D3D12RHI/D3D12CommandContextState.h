#pragma once
#include "Core/Memory/Memory.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12DescriptorCache.h"
#include "D3D12RHI/D3D12PipelineState.h"

class FD3D12CommandContextState : public FD3D12DeviceChild
{
public:
    FD3D12CommandContextState(FD3D12Device* InDevice, FD3D12CommandContext& InContext);
    ~FD3D12CommandContextState() = default;

    bool Initialize();

    void PrepareGraphicsState();
    void PrepareComputeState();

    void BindGraphicsState();
    void BindComputeState();

    void BindShaderConstants(FD3D12RootSignature* InRootSignature, EShaderVisibility ShaderStage);
    void ResetState();
    void ResetStateResources();
    void ResetStateForNewCommandList();

    void SetGraphicsPipelineState(FD3D12GraphicsPipelineStateRHI* InGraphicsPipelineState);
    void SetComputePipelineState(FD3D12ComputePipelineStateRHI* InComputePipelineState);
    void SetRenderTargets(FD3D12RenderTargetView* const* RenderTargets, uint32 NumRenderTargets, FD3D12DepthStencilView* DepthStencil);
    void SetShadingRate(EShadingRate ShadingRate);
    void SetShadingRateImage(FD3D12TextureRHI* ShadingRateImage);
    void SetViewports(D3D12_VIEWPORT* Viewports, uint32 NumViewports);
    void SetScissorRects(D3D12_RECT* ScissorRects, uint32 NumScissorRects);
    void SetBlendFactor(const float BlendFactor[4]);
    void SetStencilRef(uint32 InStencilRef);
    void SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias);
    void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets);
    void SetVertexBuffer(FD3D12BufferRHI* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FD3D12BufferRHI* IndexBuffer, DXGI_FORMAT IndexFormat);
    void SetSRV(FD3D12ShaderResourceViewRHI* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetUAV(FD3D12UnorderedAccessViewRHI* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetCBV(FD3D12BufferRHI* Buffer, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetSampler(FD3D12SamplerStateRHI* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex);
    void SetShaderConstants(const uint32* ShaderConstants, uint32 NumShaderConstants);

    FORCEINLINE FD3D12CommandContext& GetContext()
    {
        return Context;
    }

    FORCEINLINE FD3D12DescriptorCache& GetDescriptorCache()
    {
        return CommonState.DescriptorCache;
    }

    FORCEINLINE FD3D12GraphicsPipelineStateRHI* GetGraphicsPipelineState() const
    {
        return GraphicsState.PipelineState.Get();
    }

    FORCEINLINE FD3D12ComputePipelineStateRHI* GetComputePipelineState() const
    {
        return ComputeState.PipelineState.Get();
    }

    FORCEINLINE void GetRenderTargets(FD3D12RenderTargetView** RenderTargetViews, uint32& OutNumRenderTargets, FD3D12DepthStencilView** DepthStencilView) const
    {
        const uint32 CurrentNumRenderTargets = GraphicsState.RenderTargetCache.NumRenderTargets;
        if (RenderTargetViews)
        {
            FMemory::Memcpy(RenderTargetViews, GraphicsState.RenderTargetCache.RenderTargetViews, sizeof(FD3D12RenderTargetView*) * CurrentNumRenderTargets);
        }

        OutNumRenderTargets = CurrentNumRenderTargets;

        if (DepthStencilView)
        {
            *DepthStencilView = GraphicsState.RenderTargetCache.DepthStencilView;
        }
    }

    FORCEINLINE D3D12_SHADING_RATE GetShadingRate() const
    {
        return GraphicsState.ShadingRate;
    }

    FORCEINLINE FD3D12TextureRHI* GetShadingRateImage() const
    {
        return GraphicsState.ShadingRateImage;
    }

    FORCEINLINE void GetViewports(D3D12_VIEWPORT* Viewports, uint32& OutNumViewports) const
    {
        if (Viewports)
        {
            FMemory::Memcpy(Viewports, GraphicsState.Viewports, sizeof(D3D12_VIEWPORT) * GraphicsState.NumViewports);
        }

        OutNumViewports = GraphicsState.NumViewports;
    }

    FORCEINLINE void GetScissorRects(D3D12_RECT* ScissorRects, uint32& OutNumScissorRects) const
    {
        if (ScissorRects)
        {
            FMemory::Memcpy(ScissorRects, GraphicsState.ScissorRects, sizeof(D3D12_RECT) * GraphicsState.NumScissorRects);
        }

        OutNumScissorRects = GraphicsState.NumScissorRects;
    }

    FORCEINLINE void GetBlendFactor(float* BlendFactor) const
    {
        if (BlendFactor)
        {
            FMemory::Memcpy(BlendFactor, GraphicsState.BlendFactor, sizeof(GraphicsState.BlendFactor));
        }
    }

private:
    bool PrepareResources(FD3D12RootSignature* InRootSignature, FD3D12PipelineState* InPipelineState, EShaderVisibility StartStage, EShaderVisibility EndStage);
    bool PrepareSamplers(FD3D12RootSignature* InRootSignature, FD3D12PipelineState* InPipelineState, EShaderVisibility StartStage, EShaderVisibility EndStage);

    void BindResources(FD3D12RootSignature* InRootSignature, EShaderVisibility StartStage, EShaderVisibility EndStage);
    void BindSamplers(FD3D12RootSignature* InRootSignature, EShaderVisibility StartStage, EShaderVisibility EndStage);

    void InternalSetRootSignature(FD3D12RootSignature* InRootSignature, EShaderVisibility ShaderStage);

    FD3D12CommandContext& Context;

    struct FGraphicsState
    {
        FGraphicsState()
            : PipelineState(nullptr)
            , NumViewports(0)
            , NumScissorRects(0)
            , ShadingRateImage(nullptr)
            , ShadingRate(D3D12_SHADING_RATE_1X1)
            , RenderTargetCache()
            , IndexBufferCache()
            , VertexBufferCache()
        {
            FMemory::Memzero(BlendFactor, sizeof(BlendFactor));
            StencilRef = 0;
            
            FMemory::Memzero(DepthBias, sizeof(DepthBias));
            FMemory::Memzero(SOBufferViews, sizeof(SOBufferViews));
            FMemory::Memzero(SOBuffers, sizeof(SOBuffers));
            NumSOBuffers = 0;

            FMemory::Memzero(Viewports, sizeof(Viewports));
            FMemory::Memzero(ScissorRects, sizeof(ScissorRects));
        }

        FD3D12GraphicsPipelineStateRHIRef PipelineState;
        float                             BlendFactor[4];
        uint32                            StencilRef;
        D3D12_VIEWPORT                    Viewports[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                            NumViewports;
        D3D12_RECT                        ScissorRects[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                            NumScissorRects;
        FD3D12TextureRHI*                 ShadingRateImage;
        D3D12_SHADING_RATE                ShadingRate;
        float                             DepthBias[3]; // DepthBias, DepthBiasClamp, SlopeScaledDepthBias
        D3D12_STREAM_OUTPUT_BUFFER_VIEW   SOBufferViews[4];
        FD3D12BufferRHI*                  SOBuffers[4];
        uint32                            NumSOBuffers;
        FD3D12RenderTargetCache           RenderTargetCache;
        FD3D12IndexBufferCache            IndexBufferCache;
        FD3D12VertexBufferCache           VertexBufferCache;

        bool bBindRenderTargets       : 1;
        bool bBindBlendFactor         : 1;
        bool bBindStencilRef          : 1;
        bool bBindDepthBias           : 1;
        bool bBindStreamOutputTargets : 1;
        bool bBindPipelineState       : 1;
        bool bBindScissorRects        : 1;
        bool bBindViewports           : 1;
        bool bBindRootSignature       : 1;
        bool bBindShadingRate         : 1;
        bool bBindShadingRateImage    : 1;
        bool bBindVertexBuffers       : 1;
        bool bBindIndexBuffer         : 1;
        bool bBindShaderConstants     : 1;
        bool bBindPrimitiveTopology   : 1;
    } GraphicsState;

    struct FComputeState
    {
        FComputeState()
            : PipelineState(nullptr)
        {
        }

        FD3D12ComputePipelineStateRHIRef PipelineState;

        bool bBindPipelineState   : 1;
        bool bBindRootSignature   : 1;
        bool bBindShaderConstants : 1;
    } ComputeState;

    struct FCommonState
    {
        FCommonState(FD3D12Device* InDevice, FD3D12CommandContext& InContext)
            : DescriptorCache(InDevice, InContext)
        {
        }

        FD3D12ConstantBufferCache      ConstantBufferCache;
        FD3D12ShaderResourceViewCache  ShaderResourceViewCache;
        FD3D12UnorderedAccessViewCache UnorderedAccessViewCache;
        FD3D12SamplerStateCache        SamplerStateCache;
        FD3D12DescriptorCache          DescriptorCache;
        FD3D12ShaderConstantsCache     ShaderConstantsCache;
    } CommonState;
};
