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

template<typename DescType, typename RHIViewType>
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

    /**
     * @return The view registered with the graph, or nullptr when the graph is to create its own.
     * A registered view stays owned by whoever handed it over, so the graph only borrows it.
     */
    NODISCARD RHIViewType* GetExternalView() const
    {
        return ExternalView;
    }

    NODISCARD bool IsExternal() const
    {
        return ExternalView != nullptr;
    }

private:
    friend class FRenderGraphBuilder;

    FRenderGraphTexture* Parent       = nullptr;
    const CHAR*          Name         = nullptr;
    RHIViewType*         ExternalView = nullptr;
    DescType             Desc;
};

template<typename DescType, typename RHIViewType>
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

    /**
     * @return The view registered with the graph, or nullptr when the graph is to create its own.
     * A registered view stays owned by whoever handed it over, so the graph only borrows it.
     */
    NODISCARD RHIViewType* GetExternalView() const
    {
        return ExternalView;
    }

    NODISCARD bool IsExternal() const
    {
        return ExternalView != nullptr;
    }

private:
    friend class FRenderGraphBuilder;

    union
    {
        FRenderGraphTexture* Texture;
        FRenderGraphBuffer*  Buffer;
    } Parent;

    ERenderGraphParentKind ParentKind   = ERenderGraphParentKind::Texture;
    const CHAR*            Name         = nullptr;
    RHIViewType*           ExternalView = nullptr;
    DescType               Desc;
};

using FRenderGraphShaderResourceView  = TRenderGraphShaderAccessView<FRHIShaderResourceViewDesc, FRHIShaderResourceView>;
using FRenderGraphUnorderedAccessView = TRenderGraphShaderAccessView<FRHIUnorderedAccessViewDesc, FRHIUnorderedAccessView>;
using FRenderGraphRenderTargetView    = TRenderGraphTextureView<FRHIRenderTargetViewDesc, FRHIRenderTargetView>;
using FRenderGraphDepthStencilView    = TRenderGraphTextureView<FRHIDepthStencilViewDesc, FRHIDepthStencilView>;

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
