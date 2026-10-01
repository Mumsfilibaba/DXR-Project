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
        Memory::Memzero(RenderTargetViews, sizeof(RenderTargetViews));
        Memory::Memzero(RenderTargetResources, sizeof(RenderTargetResources));
        DepthStencilView     = nullptr;
        DepthStencilResource = nullptr;
        NumRenderTargets     = 0;
    }

    ID3D11RenderTargetView* RenderTargetViews[D3D11_MAX_RENDER_TARGET_COUNT];
    FRHIResource*           RenderTargetResources[D3D11_MAX_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView* DepthStencilView;
    FRHIResource*           DepthStencilResource;
    uint32                  NumRenderTargets;
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
        Memory::Memzero(ResourceViews, sizeof(ResourceViews));
        Memory::Memzero(Resources, sizeof(Resources));
        Memory::Memzero(NumViews, sizeof(NumViews));
    }

    ID3D11ShaderResourceView* ResourceViews[EShaderVisibility::Count][D3D11_MAX_SHADER_RESOURCE_VIEWS];
    FRHIResource*             Resources[EShaderVisibility::Count][D3D11_MAX_SHADER_RESOURCE_VIEWS];
    uint8                     NumViews[EShaderVisibility::Count];
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
    void ResetState();

    void DirtyRenderTargets();

    void SetGraphicsPipelineState(FD3D11GraphicsPipelineStateRHI* InGraphicsPipelineState);
    void SetRenderTargets(FD3D11RenderTargetViewRHI* const* RenderTargets, uint32 NumRenderTargets, FD3D11DepthStencilViewRHI* DepthStencil);
    void SetViewports(const D3D11_VIEWPORT* Viewports, uint32 NumViewports);
    void SetScissorRects(const D3D11_RECT* ScissorRects, uint32 NumScissorRects);
    void SetBlendFactor(const float BlendFactor[4]);
    void SetStencilRef(uint32 InStencilRef);
    void SetVertexBuffer(FD3D11BufferRHI* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FD3D11BufferRHI* IndexBuffer, DXGI_FORMAT IndexFormat);
    void SetSRV(FD3D11ShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
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

private:
    void BindRenderTargets();
    void BindShaderConstants(EShaderConstantsPipeline::Type Pipeline);
    void BindConstantBuffers(EShaderVisibility::Type ShaderStage, const FD3D11Shader* Shader);
    void BindShaderResourceViews(EShaderVisibility::Type ShaderStage);
    void BindSamplers(EShaderVisibility::Type ShaderStage);

    void UnbindRenderTargetResources();
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

    struct FCommonState
    {
        FD3D11ConstantBufferCache     ConstantBufferCache;
        FD3D11ShaderResourceViewCache ShaderResourceViewCache;
        FD3D11SamplerStateCache       SamplerStateCache;
        FD3D11ShaderConstantsCache    ShaderConstantsCache[EShaderConstantsPipeline::Count];
        TComPtr<ID3D11Buffer>         ShaderConstantsBuffers[EShaderConstantsPipeline::Count];
    } CommonState;
};
