#pragma once
#include "Core/Memory/Memory.h"
#include "D3D11RHI/D3D11Buffer.h"
#include "D3D11RHI/D3D11PipelineState.h"
#include "D3D11RHI/D3D11ResourceViews.h"
#include "D3D11RHI/D3D11SamplerState.h"

class FD3D11CommandContext;

struct EShaderConstantsPipeline
{
    enum Type : int32
    {
        Graphics = 0,
        Compute,
        Count
    };
};

FORCEINLINE EShaderConstantsPipeline::Type GetShaderConstantsPipeline(EShaderStage ShaderStage)
{
    if (ShaderStage == EShaderStage::Compute)
    {
        return EShaderConstantsPipeline::Compute;
    }
    else
    {
        return EShaderConstantsPipeline::Graphics;
    }
}

struct FD3D11VertexBufferCache
{
    FD3D11VertexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        Memory::Memzero(VertexBuffers, sizeof(VertexBuffers));
        Memory::Memzero(Strides, sizeof(Strides));
        Memory::Memzero(Offsets, sizeof(Offsets));
        NumVertexBuffers = 0;
    }

    ID3D11Buffer* VertexBuffers[D3D11_MAX_VERTEX_BUFFER_SLOTS];
    UINT          Strides[D3D11_MAX_VERTEX_BUFFER_SLOTS];
    UINT          Offsets[D3D11_MAX_VERTEX_BUFFER_SLOTS];
    uint32        NumVertexBuffers;
};

struct FD3D11IndexBufferCache
{
    FD3D11IndexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        IndexBuffer = nullptr;
        IndexFormat = DXGI_FORMAT_UNKNOWN;
    }

    ID3D11Buffer* IndexBuffer;
    DXGI_FORMAT   IndexFormat;
};

struct FD3D11RenderTargetCache
{
    FD3D11RenderTargetCache()
    {
        Clear();
    }

    void Clear()
    {
        for (uint32 Index = 0; Index < D3D11_MAX_RENDER_TARGET_COUNT; Index++)
        {
            RenderTargetViews[Index].Reset();
            RenderTargetRanges[Index]      = FD3D11SubresourceRange();
            BoundRenderTargetViews[Index]  = nullptr;
            BoundRenderTargetRanges[Index] = FD3D11SubresourceRange();
        }

        DepthStencilView.Reset();

        DepthStencilRange          = FD3D11SubresourceRange();
        bDepthStencilReadOnly      = false;
        BoundDepthStencilView      = nullptr;
        BoundDepthStencilRange     = FD3D11SubresourceRange();
        bBoundDepthStencilReadOnly = false;
        NumRenderTargets           = 0;
        NumBoundRenderTargets      = 0;
    }

    // A read-only depth-stencil view can be bound together with shader resource views of the same texture
    bool IsBoundForWrite(const FD3D11SubresourceRange& Range) const
    {
        for (uint32 Index = 0; Index < NumBoundRenderTargets; Index++)
        {
            if (BoundRenderTargetRanges[Index].Overlaps(Range))
            {
                return true;
            }
        }

        return !bBoundDepthStencilReadOnly && BoundDepthStencilRange.Overlaps(Range);
    }

    TComPtr<ID3D11RenderTargetView> RenderTargetViews[D3D11_MAX_RENDER_TARGET_COUNT];
    FD3D11SubresourceRange          RenderTargetRanges[D3D11_MAX_RENDER_TARGET_COUNT];
    TComPtr<ID3D11DepthStencilView> DepthStencilView;
    FD3D11SubresourceRange          DepthStencilRange;
    bool                            bDepthStencilReadOnly;
    uint32                          NumRenderTargets;
    ID3D11RenderTargetView*         BoundRenderTargetViews[D3D11_MAX_RENDER_TARGET_COUNT];
    FD3D11SubresourceRange          BoundRenderTargetRanges[D3D11_MAX_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView*         BoundDepthStencilView;
    FD3D11SubresourceRange          BoundDepthStencilRange;
    bool                            bBoundDepthStencilReadOnly;
    uint32                          NumBoundRenderTargets;
};

struct FD3D11ResourceCache
{
    bool IsResourcesDirty(EShaderVisibility::Type ShaderStage) const
    {
        return bResourcesDirty[ShaderStage];
    }

    void DirtyResources(EShaderVisibility::Type ShaderStage)
    {
        bResourcesDirty[ShaderStage] = true;
    }

    void DirtyResourcesAll()
    {
        for (int32 Index = 0; Index < EShaderVisibility::Count; Index++)
        {
            bResourcesDirty[Index] = true;
        }
    }

    void ClearResourcesDirty(EShaderVisibility::Type ShaderStage)
    {
        bResourcesDirty[ShaderStage] = false;
    }

    bool bResourcesDirty[EShaderVisibility::Count];
};

struct FD3D11ConstantBufferCache : public FD3D11ResourceCache
{
    FD3D11ConstantBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();
        Memory::Memzero(ConstantBuffers, sizeof(ConstantBuffers));
        Memory::Memzero(NumBuffers, sizeof(NumBuffers));
    }

    ID3D11Buffer* ConstantBuffers[EShaderVisibility::Count][D3D11_MAX_CONSTANT_BUFFERS];
    uint8         NumBuffers[EShaderVisibility::Count];
};

struct FD3D11ShaderResourceViewCache : public FD3D11ResourceCache
{
    FD3D11ShaderResourceViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (int32 StageIndex = 0; StageIndex < EShaderVisibility::Count; StageIndex++)
        {
            for (int32 Index = 0; Index < D3D11_MAX_SHADER_RESOURCE_VIEWS; Index++)
            {
                ResourceViews[StageIndex][Index].Reset();

                Ranges[StageIndex][Index]      = FD3D11SubresourceRange();
                Dimensions[StageIndex][Index]  = D3D11_SRV_DIMENSION_UNKNOWN;
                BoundViews[StageIndex][Index]  = nullptr;
                BoundRanges[StageIndex][Index] = FD3D11SubresourceRange();
            }

            NumViews[StageIndex]      = 0;
            NumBoundViews[StageIndex] = 0;
        }
    }

    TComPtr<ID3D11ShaderResourceView> ResourceViews[EShaderVisibility::Count][D3D11_MAX_SHADER_RESOURCE_VIEWS];
    FD3D11SubresourceRange            Ranges[EShaderVisibility::Count][D3D11_MAX_SHADER_RESOURCE_VIEWS];
    D3D11_SRV_DIMENSION               Dimensions[EShaderVisibility::Count][D3D11_MAX_SHADER_RESOURCE_VIEWS];
    uint8                             NumViews[EShaderVisibility::Count];
    ID3D11ShaderResourceView*         BoundViews[EShaderVisibility::Count][D3D11_MAX_SHADER_RESOURCE_VIEWS];
    FD3D11SubresourceRange            BoundRanges[EShaderVisibility::Count][D3D11_MAX_SHADER_RESOURCE_VIEWS];
    uint8                             NumBoundViews[EShaderVisibility::Count];
};

struct FD3D11UnorderedAccessViewCache
{
    FD3D11UnorderedAccessViewCache()
    {
        Clear();
    }

    void Clear()
    {
        for (int32 Index = 0; Index < D3D11_MAX_UNORDERED_ACCESS_VIEWS; Index++)
        {
            UnorderedAccessViews[Index].Reset();
            Ranges[Index]      = FD3D11SubresourceRange();
            BoundViews[Index]  = nullptr;
            BoundRanges[Index] = FD3D11SubresourceRange();
        }

        NumViews      = 0;
        NumBoundViews = 0;
        bDirty        = true;
    }

    bool IsBound(const FD3D11SubresourceRange& Range) const
    {
        for (uint32 Index = 0; Index < NumBoundViews; Index++)
        {
            if (BoundRanges[Index].Overlaps(Range))
            {
                return true;
            }
        }

        return false;
    }

    TComPtr<ID3D11UnorderedAccessView> UnorderedAccessViews[D3D11_MAX_UNORDERED_ACCESS_VIEWS];
    FD3D11SubresourceRange             Ranges[D3D11_MAX_UNORDERED_ACCESS_VIEWS];
    uint32                             NumViews;
    ID3D11UnorderedAccessView*         BoundViews[D3D11_MAX_UNORDERED_ACCESS_VIEWS];
    FD3D11SubresourceRange             BoundRanges[D3D11_MAX_UNORDERED_ACCESS_VIEWS];
    uint32                             NumBoundViews;
    bool                               bDirty;
};

struct FD3D11SamplerStateCache : public FD3D11ResourceCache
{
    FD3D11SamplerStateCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();
        Memory::Memzero(SamplerStates, sizeof(SamplerStates));
        Memory::Memzero(NumSamplers, sizeof(NumSamplers));
    }

    ID3D11SamplerState* SamplerStates[EShaderVisibility::Count][D3D11_MAX_SAMPLER_STATES];
    uint8               NumSamplers[EShaderVisibility::Count];
};

struct FD3D11ShaderConstantsCache
{
    FD3D11ShaderConstantsCache()
    {
        Clear();
    }

    void Clear()
    {
        Memory::Memzero(Constants, sizeof(Constants));
        NumConstants = 0;
    }

    uint32 Constants[D3D11_MAX_32BIT_SHADER_CONSTANTS_COUNT];
    uint32 NumConstants;
};

class FD3D11CommandContextState : public FD3D11DeviceChild
{
public:
    FD3D11CommandContextState(FD3D11Device* InDevice, FD3D11CommandContext& InContext);
    ~FD3D11CommandContextState();

    bool Initialize();

    void BindGraphicsState();
    void BindComputeState();
    void ResetState();

    void DirtyRenderTargets();

    void SetGraphicsPipelineState(FD3D11GraphicsPipelineStateRHI* InGraphicsPipelineState);
    void SetComputePipelineState(FD3D11ComputePipelineStateRHI* InComputePipelineState);
    void SetRenderTargets(FD3D11RenderTargetViewRHI* const* RenderTargets, uint32 NumRenderTargets, FD3D11DepthStencilViewRHI* DepthStencil);
    void SetViewports(const D3D11_VIEWPORT* Viewports, uint32 NumViewports);
    void SetScissorRects(const D3D11_RECT* ScissorRects, uint32 NumScissorRects);
    void SetBlendFactor(const float BlendFactor[4]);
    void SetStencilRef(uint32 InStencilRef);
    void SetVertexBuffer(FD3D11BufferRHI* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FD3D11BufferRHI* IndexBuffer, DXGI_FORMAT IndexFormat);
    void SetSRV(FD3D11ShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetUAV(FD3D11UnorderedAccessViewRHI* UnorderedAccessView, uint32 ResourceIndex);
    void SetCBV(FD3D11BufferRHI* Buffer, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetSampler(FD3D11SamplerStateRHI* SamplerState, EShaderVisibility::Type ShaderStage, uint32 SamplerIndex);
    void SetShaderConstants(EShaderStage ShaderStage, const uint32* ShaderConstants, uint32 NumShaderConstants);

    FORCEINLINE FD3D11CommandContext& GetContext()
    {
        return Context;
    }

    FORCEINLINE FD3D11GraphicsPipelineStateRHI* GetGraphicsPipelineState() const
    {
        return GraphicsState.PipelineState.Get();
    }

    FORCEINLINE FD3D11ComputePipelineStateRHI* GetComputePipelineState() const
    {
        return ComputeState.PipelineState.Get();
    }

private:
    void BindRenderTargets();
    void BindUnorderedAccessViews(const FD3D11Shader* Shader);
    void BindShaderConstants(EShaderConstantsPipeline::Type Pipeline);
    void BindConstantBuffers(EShaderVisibility::Type ShaderStage, const FD3D11Shader* Shader);
    void BindShaderResourceViews(EShaderVisibility::Type ShaderStage, const FD3D11Shader* Shader);
    void BindSamplers(EShaderVisibility::Type ShaderStage, const TArray<FD3D11StaticSampler>& StaticSamplers);

    void UnbindRenderTargets(const FD3D11SubresourceRange& Range);
    void UnbindUnorderedAccessViews(const FD3D11SubresourceRange& Range);
    void UnbindShaderResourceViews(const FD3D11SubresourceRange& Range);

    void DirtyShaderConstants(EShaderConstantsPipeline::Type Pipeline);
    void DirtyAllResources();

    FD3D11CommandContext& Context;

    struct FCommonGraphicsState
    {
        FCommonGraphicsState()
            : NumViewports(0)
            , NumScissorRects(0)
            , StencilRef(0)
            , RenderTargetCache()
        {
            Memory::Memzero(BlendFactor, sizeof(BlendFactor));
            Memory::Memzero(Viewports, sizeof(Viewports));
            Memory::Memzero(ScissorRects, sizeof(ScissorRects));
        }

        float                   BlendFactor[4];
        uint32                  StencilRef;
        D3D11_VIEWPORT          Viewports[D3D11_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                  NumViewports;
        D3D11_RECT              ScissorRects[D3D11_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                  NumScissorRects;
        FD3D11RenderTargetCache RenderTargetCache;

        bool bBindRenderTargets     : 1;
        bool bBindBlendState        : 1;
        bool bBindDepthStencilState : 1;
        bool bBindScissorRects      : 1;
        bool bBindViewports         : 1;
    } CommonGraphicsState;

    struct FGraphicsState
    {
        FGraphicsState()
            : PipelineState(nullptr)
            , IndexBufferCache()
            , VertexBufferCache()
        {
        }

        FD3D11GraphicsPipelineStateRHIRef PipelineState;
        FD3D11IndexBufferCache            IndexBufferCache;
        FD3D11VertexBufferCache           VertexBufferCache;

        bool bBindPipelineState   : 1;
        bool bBindVertexBuffers   : 1;
        bool bBindIndexBuffer     : 1;
        bool bBindShaderConstants : 1;
    } GraphicsState;

    struct FComputeState
    {
        FComputeState()
            : PipelineState(nullptr)
            , UnorderedAccessViewCache()
        {
        }

        FD3D11ComputePipelineStateRHIRef PipelineState;
        FD3D11UnorderedAccessViewCache   UnorderedAccessViewCache;

        bool bBindPipelineState   : 1;
        bool bBindShaderConstants : 1;
    } ComputeState;

    struct FCommonState
    {
        FD3D11ConstantBufferCache     ConstantBufferCache;
        FD3D11ShaderResourceViewCache ShaderResourceViewCache;
        FD3D11SamplerStateCache       SamplerStateCache;
        FD3D11ShaderConstantsCache    ShaderConstantsCache[EShaderConstantsPipeline::Count];
        TComPtr<ID3D11Buffer>         ShaderConstantsBuffers[EShaderConstantsPipeline::Count];
    } CommonState;
};
