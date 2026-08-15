#pragma once
#include "RHI/RHIResourceViews.h"
#include "RHI/RHITypes.h"
#include "RendererCore/RenderGraph/RenderGraphResources.h"

enum class ERenderGraphParentKind : uint8
{
    Texture,
    Buffer,
};

class FRenderGraphBuilder;

template<typename DescType>
class TRenderGraphTextureView
{
public:
    NODISCARD FRenderGraphTexture* GetParent() const
    {
        return Parent;
    }

    NODISCARD const DescType& GetDesc() const
    {
        return Desc;
    }

    NODISCARD const CHAR* GetName() const
    {
        return Name;
    }

private:
    friend class FRenderGraphBuilder;

    FRenderGraphTexture* Parent = nullptr;
    const CHAR*          Name   = nullptr;
    DescType             Desc;
};

template<typename DescType>
class TRenderGraphShaderAccessView
{
public:
    NODISCARD ERenderGraphParentKind GetParentKind() const
    {
        return ParentKind;
    }

    NODISCARD FRenderGraphTexture* GetParentTexture() const
    {
        return ParentKind == ERenderGraphParentKind::Texture ? Parent.Texture : nullptr;
    }

    NODISCARD FRenderGraphBuffer* GetParentBuffer() const
    {
        return ParentKind == ERenderGraphParentKind::Buffer ? Parent.Buffer : nullptr;
    }

    NODISCARD const DescType& GetDesc() const
    {
        return Desc;
    }

    NODISCARD const CHAR* GetName() const
    {
        return Name;
    }

private:
    friend class FRenderGraphBuilder;

    union
    {
        FRenderGraphTexture* Texture;
        FRenderGraphBuffer*  Buffer;
    } Parent;

    ERenderGraphParentKind ParentKind = ERenderGraphParentKind::Texture;
    const CHAR*            Name       = nullptr;
    DescType               Desc;
};

using FRenderGraphShaderResourceView  = TRenderGraphShaderAccessView<FRHIShaderResourceViewDesc>;
using FRenderGraphUnorderedAccessView = TRenderGraphShaderAccessView<FRHIUnorderedAccessViewDesc>;
using FRenderGraphRenderTargetView    = TRenderGraphTextureView<FRHIRenderTargetViewDesc>;
using FRenderGraphDepthStencilView    = TRenderGraphTextureView<FRHIDepthStencilViewDesc>;

enum class ERenderGraphViewAccessType : uint8
{
    ShaderResource,
    UnorderedAccess,
    RenderTarget,
    DepthStencil,
};

struct FRenderGraphViewAccess
{
    ERenderGraphViewAccessType       Type                = ERenderGraphViewAccessType::ShaderResource;
    FRenderGraphShaderResourceView*  ShaderResourceView  = nullptr;
    FRenderGraphUnorderedAccessView* UnorderedAccessView = nullptr;
    FRenderGraphRenderTargetView*    RenderTargetView    = nullptr;
    FRenderGraphDepthStencilView*    DepthStencilView    = nullptr;
    ERHIResourceState                State               = ERHIResourceState::Common;
    bool                             bIsWrite            = false;
    FRHITextureSubresourceRange      SubresourceRange    = FRHITextureSubresourceRange::All();
    FBufferRegion                    BufferRange         = FBufferRegion::Whole();
    bool                             bIsTextureParent    = true;
    FRenderGraphTexture*             ParentTexture       = nullptr;
    FRenderGraphBuffer*              ParentBuffer        = nullptr;
};

struct RenderGraphDefaultViewDescs
{
    static FRHIShaderResourceViewDesc  ShaderResourceForTexture(const FRHITextureDesc& TextureDesc);
    static FRHIUnorderedAccessViewDesc UnorderedAccessForTexture(const FRHITextureDesc& TextureDesc);
    static FRHIRenderTargetViewDesc    RenderTargetForTexture(const FRHITextureDesc& TextureDesc);
    static FRHIDepthStencilViewDesc    DepthStencilForTexture(const FRHITextureDesc& TextureDesc);

    static FRHIShaderResourceViewDesc  ShaderResourceForBuffer(const FRHIBufferDesc& BufferDesc);
    static FRHIUnorderedAccessViewDesc UnorderedAccessForBuffer(const FRHIBufferDesc& BufferDesc);
};

struct RenderGraphViewRanges
{
    static FRHITextureSubresourceRange SubresourceRangeFor(FRenderGraphTexture* Parent, const FRHIShaderResourceViewDesc& Desc);
    static FRHITextureSubresourceRange SubresourceRangeFor(FRenderGraphTexture* Parent, const FRHIUnorderedAccessViewDesc& Desc);
    static FRHITextureSubresourceRange SubresourceRangeFor(FRenderGraphTexture* Parent, const FRHIRenderTargetViewDesc& Desc);
    static FRHITextureSubresourceRange SubresourceRangeFor(FRenderGraphTexture* Parent, const FRHIDepthStencilViewDesc& Desc);

    static FBufferRegion BufferRangeFor(FRenderGraphBuffer* Parent, const FRHIShaderResourceViewDesc& Desc);
    static FBufferRegion BufferRangeFor(FRenderGraphBuffer* Parent, const FRHIUnorderedAccessViewDesc& Desc);

    static bool Overlap(const FRHITextureSubresourceRange& A, const FRHITextureSubresourceRange& B);
};
