#pragma once
#include "Core/Memory/Memory.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12DescriptorCache.h"
#include "D3D12RHI/D3D12PipelineState.h"

class FD3D12RayTracingPipelineStateRHI;
typedef TSharedRef<class FD3D12RayTracingPipelineStateRHI> FD3D12RayTracingPipelineStateRHIRef;

class FD3D12CommandContextState : public FD3D12DeviceChild
{
public:
    FD3D12CommandContextState(FD3D12Device* InDevice, FD3D12CommandContext& InContext);
    ~FD3D12CommandContextState();

    bool Initialize();

    void PrepareGraphicsState();
    void PrepareComputeState();
    void PrepareMeshletState();

    void BindGraphicsState();
    void BindComputeState();
    void BindMeshletState();
    void BindRayTracingState();

    void BindShaderConstants(FD3D12RootSignature* InRootSignature, EShaderVisibility::Type ShaderStage);
    void ResetState();
    void ResetStateResources();
    void ResetStateForNewCommandList();

    void SetGraphicsPipelineState(FD3D12GraphicsPipelineStateRHI* InGraphicsPipelineState);
    void SetComputePipelineState(FD3D12ComputePipelineStateRHI* InComputePipelineState);
    void SetMeshletPipelineState(FD3D12MeshletPipelineStateRHI* InMeshletPipelineState);
    void SetRayTracingPipelineState(FD3D12RayTracingPipelineStateRHI* InRayTracingPipelineState);
    void SetRenderTargets(FD3D12RenderTargetViewRHI* const* RenderTargets, uint32 NumRenderTargets, FD3D12DepthStencilViewRHI* DepthStencil);
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
    void SetSRV(FD3D12ShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetUAV(FD3D12UnorderedAccessViewRHI* UnorderedAccessView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetCBV(FD3D12BufferRHI* Buffer, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetSampler(FD3D12SamplerStateRHI* SamplerState, EShaderVisibility::Type ShaderStage, uint32 SamplerIndex);
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

    FORCEINLINE FD3D12MeshletPipelineStateRHI* GetMeshletPipelineState() const
    {
        return MeshletState.PipelineState.Get();
    }

    FORCEINLINE FD3D12RayTracingPipelineStateRHI* GetRayTracingPipelineState() const
    {
        return RayTracingState.PipelineState.Get();
    }

    FORCEINLINE void GetRenderTargets(FD3D12RenderTargetViewRHI** RenderTargetViews, uint32& OutNumRenderTargets, FD3D12DepthStencilViewRHI** DepthStencilView) const
    {
        const uint32 CurrentNumRenderTargets = CommonGraphicsState.RenderTargetCache.NumRenderTargets;
        if (RenderTargetViews)
        {
            Memory::Memcpy(RenderTargetViews, CommonGraphicsState.RenderTargetCache.RenderTargetViews, sizeof(FD3D12RenderTargetViewRHI*) * CurrentNumRenderTargets);
        }

        OutNumRenderTargets = CurrentNumRenderTargets;

        if (DepthStencilView)
        {
            *DepthStencilView = CommonGraphicsState.RenderTargetCache.DepthStencilView;
        }
    }

    FORCEINLINE D3D12_SHADING_RATE GetShadingRate() const
    {
        return CommonGraphicsState.ShadingRate;
    }

    FORCEINLINE FD3D12TextureRHI* GetShadingRateImage() const
    {
        return CommonGraphicsState.ShadingRateImage;
    }

    FORCEINLINE void GetViewports(D3D12_VIEWPORT* Viewports, uint32& OutNumViewports) const
    {
        if (Viewports)
        {
            Memory::Memcpy(Viewports, CommonGraphicsState.Viewports, sizeof(D3D12_VIEWPORT) * CommonGraphicsState.NumViewports);
        }

        OutNumViewports = CommonGraphicsState.NumViewports;
    }

    FORCEINLINE void GetScissorRects(D3D12_RECT* ScissorRects, uint32& OutNumScissorRects) const
    {
        if (ScissorRects)
        {
            Memory::Memcpy(ScissorRects, CommonGraphicsState.ScissorRects, sizeof(D3D12_RECT) * CommonGraphicsState.NumScissorRects);
        }

        OutNumScissorRects = CommonGraphicsState.NumScissorRects;
    }

    FORCEINLINE void GetBlendFactor(float* BlendFactor) const
    {
        if (BlendFactor)
        {
            Memory::Memcpy(BlendFactor, CommonGraphicsState.BlendFactor, sizeof(CommonGraphicsState.BlendFactor));
        }
    }

private:
    bool PrepareResources(FD3D12RootSignature* InRootSignature, const FD3D12EffectiveDescriptorCounts* InPipelineState, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage);
    bool PrepareSamplers(FD3D12RootSignature* InRootSignature, const FD3D12EffectiveDescriptorCounts* InPipelineState, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage);

    void BindResources(FD3D12RootSignature* InRootSignature, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage);
    void BindSamplers(FD3D12RootSignature* InRootSignature, EShaderVisibility::Type StartStage, EShaderVisibility::Type EndStage);

    void InternalSetRootSignature(FD3D12RootSignature* InRootSignature, bool bIsCompute);

    void DirtyAllResources();

    enum class EActivePipeline : uint8
    {
        Graphics,
        Compute,
        Meshlet,
        RayTracing,
    };

    FD3D12CommandContext& Context;
    EActivePipeline       ActivePipeline = EActivePipeline::Graphics;

    struct FCommonGraphicsState
    {
        FCommonGraphicsState()
            : NumViewports(0)
            , NumScissorRects(0)
            , ShadingRateImage(nullptr)
            , ShadingRate(D3D12_SHADING_RATE_1X1)
            , RenderTargetCache()
            , BoundRootSignature(nullptr)
        {
            Memory::Memzero(BlendFactor, sizeof(BlendFactor));
            StencilRef = 0;

            Memory::Memzero(DepthBias, sizeof(DepthBias));
            Memory::Memzero(Viewports, sizeof(Viewports));
            Memory::Memzero(ScissorRects, sizeof(ScissorRects));
        }

        float                   BlendFactor[4];
        uint32                  StencilRef;
        D3D12_VIEWPORT          Viewports[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                  NumViewports;
        D3D12_RECT              ScissorRects[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                  NumScissorRects;
        FD3D12TextureRHI*       ShadingRateImage;
        D3D12_SHADING_RATE      ShadingRate;
        float                   DepthBias[3]; // DepthBias, DepthBiasClamp, SlopeScaledDepthBias
        FD3D12RenderTargetCache RenderTargetCache;
        FD3D12RootSignature*    BoundRootSignature;

        bool bBindRenderTargets    : 1;
        bool bBindBlendFactor      : 1;
        bool bBindStencilRef       : 1;
        bool bBindDepthBias        : 1;
        bool bBindScissorRects     : 1;
        bool bBindViewports        : 1;
        bool bBindShadingRate      : 1;
        bool bBindShadingRateImage : 1;
    } CommonGraphicsState;

    struct FGraphicsState
    {
        FGraphicsState()
            : PipelineState(nullptr)
            , IndexBufferCache()
            , VertexBufferCache()
        {
            Memory::Memzero(SOBufferViews, sizeof(SOBufferViews));
            Memory::Memzero(SOBuffers, sizeof(SOBuffers));
            NumSOBuffers = 0;
        }

        FD3D12GraphicsPipelineStateRHIRef PipelineState;
        D3D12_STREAM_OUTPUT_BUFFER_VIEW   SOBufferViews[4];
        FD3D12BufferRHI*                  SOBuffers[4];
        uint32                            NumSOBuffers;
        FD3D12IndexBufferCache            IndexBufferCache;
        FD3D12VertexBufferCache           VertexBufferCache;

        bool bBindStreamOutputTargets : 1;
        bool bBindPipelineState       : 1;
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
        bool bBindShaderConstants : 1;
    } ComputeState;

    struct FMeshletState
    {
        FMeshletState()
            : PipelineState(nullptr)
        {
        }

        FD3D12MeshletPipelineStateRHIRef PipelineState;

        bool bBindPipelineState   : 1;
        bool bBindShaderConstants : 1;
    } MeshletState;

    struct FRayTracingState
    {
        FD3D12RayTracingPipelineStateRHIRef PipelineState;
    } RayTracingState;

    struct FCommonComputeState
    {
        FD3D12RootSignature* BoundRootSignature = nullptr;
    } ComputeCommonState;

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
