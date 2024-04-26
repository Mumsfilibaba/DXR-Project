#pragma once
#include "D3D12RootSignature.h"
#include "D3D12DescriptorCache.h"
#include "D3D12PipelineState.h"
#include "Core/Memory/Memory.h"

class FD3D12CommandContextState : public FD3D12DeviceChild
{
public:
    FD3D12CommandContextState(FD3D12Device* InDevice, FD3D12CommandContext& InContext);
    ~FD3D12CommandContextState() = default;

    bool Initialize();
    void BindGraphicsStates();
    void BindComputeState();
    void BindSamplers(FD3D12RootSignature* InRootSignature, EShaderVisibility StartStage, EShaderVisibility EndStage, bool bForceBinding);
    void BindResources(FD3D12RootSignature* InRootSignature, EShaderVisibility StartStage, EShaderVisibility EndStage, bool bForceBinding);
    void BindShaderConstants(FD3D12RootSignature* InRootSignature, EShaderVisibility ShaderStage);
    void ResetState();
    void ResetStateResources();
    void ResetStateForNewCommandList();

    void SetGraphicsPipelineState(FD3D12GraphicsPipelineState* InGraphicsPipelineState);
    void SetComputePipelineState(FD3D12ComputePipelineState* InComputePipelineState);
    void SetRenderTargets(FD3D12RenderTargetView* const* RenderTargets, uint32 NumRenderTargets, FD3D12DepthStencilView* DepthStencil);
    void SetShadingRate(EShadingRate ShadingRate);
    void SetShadingRateImage(FD3D12Texture* ShadingRateImage);
    void SetViewports(D3D12_VIEWPORT* Viewports, uint32 NumViewports);
    void SetScissorRects(D3D12_RECT* ScissorRects, uint32 NumScissorRects);
    void SetBlendFactor(const float BlendFactor[4]);
    void SetVertexBuffer(FD3D12Buffer* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FD3D12Buffer* IndexBuffer, DXGI_FORMAT IndexFormat);
    void SetSRV(FD3D12ShaderResourceView* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetUAV(FD3D12UnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetCBV(FD3D12ConstantBufferView* ConstantBufferView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetSampler(FD3D12SamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex);
    void SetShaderConstants(const uint32* ShaderConstants, uint32 NumShaderConstants);

public:
    FORCEINLINE FD3D12CommandContext& GetContext()
    {
        return Context;
    }

    FORCEINLINE FD3D12DescriptorCache& GetDescriptorCache()
    {
        return CommonState.DescriptorCache;
    }

public:
    FORCEINLINE FD3D12GraphicsPipelineState* GetGraphicsPipelineState() const
    {
        return GraphicsState.PipelineState.Get();
    }

    FORCEINLINE FD3D12ComputePipelineState* GetComputePipelineState() const
    {
        return ComputeState.PipelineState.Get();
    }

    FORCEINLINE void GetRenderTargets(FD3D12RenderTargetView** RenderTargetViews, uint32& OutNumRenderTargets, FD3D12DepthStencilView** DepthStencilView) const
    {
        const uint32 CurrentNumRenderTargets = GraphicsState.RTCache.NumRenderTargets;
        if (RenderTargetViews)
        {
            FMemory::Memcpy(RenderTargetViews, GraphicsState.RTCache.RenderTargetViews, sizeof(FD3D12RenderTargetView*) * CurrentNumRenderTargets);
        }

        OutNumRenderTargets = CurrentNumRenderTargets;

        if (DepthStencilView)
        {
            *DepthStencilView = GraphicsState.RTCache.DepthStencilView;
        }
    }

    FORCEINLINE D3D12_SHADING_RATE GetShadingRate() const
    {
        return GraphicsState.ShadingRate;
    }

    FORCEINLINE FD3D12Texture* GetShadingRateImage() const
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

    FORCEINLINE void GetViewports(D3D12_RECT* ScissorRects, uint32& OutNumScissorRects) const
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
    bool InternalSetRootSignature(FD3D12RootSignature* InRootSignature, EShaderVisibility ShaderStage);
    void InternalSetShaderStageResourceCount(FD3D12Shader* Shader, EShaderVisibility ShaderStage);

    FD3D12CommandContext& Context;

    struct FGraphicsState
    {
        FGraphicsState()
            : PipelineState(nullptr)
            , NumViewports(0)
            , NumScissorRects(0)
            , ShadingRateImage(nullptr)
            , ShadingRate(D3D12_SHADING_RATE_1X1)
            , RTCache()
            , IBCache()
            , VBCache()
        {
            FMemory::Memzero(BlendFactor, sizeof(BlendFactor));
            FMemory::Memzero(Viewports, sizeof(Viewports));
            FMemory::Memzero(ScissorRects, sizeof(ScissorRects));
        }

        FD3D12GraphicsPipelineStateRef PipelineState;

        float BlendFactor[4];

        D3D12_VIEWPORT Viewports[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32         NumViewports;

        D3D12_RECT ScissorRects[D3D12_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32     NumScissorRects;

        FD3D12Texture*     ShadingRateImage;
        D3D12_SHADING_RATE ShadingRate;

        FD3D12RenderTargetCache RTCache;
        FD3D12IndexBufferCache  IBCache;
        FD3D12VertexBufferCache VBCache;

        bool bBindRenderTargets     : 1;
        bool bBindBlendFactor       : 1;
        bool bBindPipelineState     : 1;
        bool bBindScissorRects      : 1;
        bool bBindViewports         : 1;
        bool bBindRootSignature     : 1;
        bool bBindShadingRate       : 1;
        bool bBindShadingRateImage  : 1;
        bool bBindVertexBuffers     : 1;
        bool bBindIndexBuffer       : 1;
        bool bBindShaderConstants   : 1;
        bool bBindPrimitiveTopology : 1;
    } GraphicsState;

    struct FComputeState
    {
        FComputeState()
            : PipelineState(nullptr)
        {
        }

        FD3D12ComputePipelineStateRef PipelineState;

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
        FShaderResourceRange           ShaderResourceCounts[ShaderVisibility_Count];

        FD3D12DescriptorCache          DescriptorCache;
        FD3D12ShaderConstantsCache     ShaderConstantsCache;
    } CommonState;
};