#include "Core/Misc/OutputDeviceManager.h"
#include "RHI/RHI.h"
#include "RendererCore/RenderGraph/RenderGraphViewCache.h"

template<typename ViewType>
static ViewType* BorrowExternalView(ViewType* View)
{
    if (View)
    {
        View->AddRef();
    }

    return View;
}

FRenderGraphViewCache::FRenderGraphViewCache()
    : NumViewsCreated(0)
    , NumViewsCacheHits(0)
{
}

FRenderGraphViewCache::~FRenderGraphViewCache() = default;

FRHIShaderResourceViewRef FRenderGraphViewCache::GetCached(FRenderGraphShaderResourceView* View) const
{
    for (int32 Index = 0; Index < ShaderResourceViews.Size(); ++Index)
    {
        if (ShaderResourceViews[Index] == View)
        {
            return ShaderResourceViewRefs[Index];
        }
    }

    return FRHIShaderResourceViewRef();
}

FRHIUnorderedAccessViewRef FRenderGraphViewCache::GetCached(FRenderGraphUnorderedAccessView* View) const
{
    for (int32 Index = 0; Index < UnorderedAccessViews.Size(); ++Index)
    {
        if (UnorderedAccessViews[Index] == View)
        {
            return UnorderedAccessViewRefs[Index];
        }
    }

    return FRHIUnorderedAccessViewRef();
}

FRHIRenderTargetViewRef FRenderGraphViewCache::GetCached(FRenderGraphRenderTargetView* View) const
{
    for (int32 Index = 0; Index < RenderTargetViews.Size(); ++Index)
    {
        if (RenderTargetViews[Index] == View)
        {
            return RenderTargetViewRefs[Index];
        }
    }

    return FRHIRenderTargetViewRef();
}

FRHIDepthStencilViewRef FRenderGraphViewCache::GetCached(FRenderGraphDepthStencilView* View) const
{
    for (int32 Index = 0; Index < DepthStencilViews.Size(); ++Index)
    {
        if (DepthStencilViews[Index] == View)
        {
            return DepthStencilViewRefs[Index];
        }
    }

    return FRHIDepthStencilViewRef();
}

FRHIShaderResourceView* FRenderGraphViewCache::CreateShaderResourceView(FRenderGraphShaderResourceView* View)
{
    if (!View)
    {
        return nullptr;
    }

    if (View->IsExternal())
    {
        return BorrowExternalView(View->GetExternalView());
    }

    if (View->GetParentKind() == ERenderGraphParentKind::Texture)
    {
        FRenderGraphTexture* Parent = View->GetParentTexture();
        if (!Parent || !Parent->GetRHITexture())
        {
            return nullptr;
        }

        return RHI::CreateShaderResourceView(Parent->GetRHITexture(), View->GetDesc());
    }

    FRenderGraphBuffer* Parent = View->GetParentBuffer();
    if (!Parent || !Parent->GetRHIBuffer())
    {
        return nullptr;
    }

    return RHI::CreateShaderResourceView(Parent->GetRHIBuffer(), View->GetDesc());
}

FRHIUnorderedAccessView* FRenderGraphViewCache::CreateUnorderedAccessView(FRenderGraphUnorderedAccessView* View)
{
    if (!View)
    {
        return nullptr;
    }

    if (View->IsExternal())
    {
        return BorrowExternalView(View->GetExternalView());
    }

    if (View->GetParentKind() == ERenderGraphParentKind::Texture)
    {
        FRenderGraphTexture* Parent = View->GetParentTexture();
        if (!Parent || !Parent->GetRHITexture())
        {
            return nullptr;
        }

        return RHI::CreateUnorderedAccessView(Parent->GetRHITexture(), View->GetDesc());
    }

    FRenderGraphBuffer* Parent = View->GetParentBuffer();
    if (!Parent || !Parent->GetRHIBuffer())
    {
        return nullptr;
    }

    return RHI::CreateUnorderedAccessView(Parent->GetRHIBuffer(), View->GetDesc());
}

FRHIRenderTargetView* FRenderGraphViewCache::CreateRenderTargetView(FRenderGraphRenderTargetView* View)
{
    if (!View)
    {
        return nullptr;
    }

    if (View->IsExternal())
    {
        return BorrowExternalView(View->GetExternalView());
    }

    FRenderGraphTexture* Parent = View->GetParent();
    if (!Parent || !Parent->GetRHITexture())
    {
        return nullptr;
    }

    return RHI::CreateRenderTargetView(Parent->GetRHITexture(), View->GetDesc());
}

FRHIDepthStencilView* FRenderGraphViewCache::CreateDepthStencilView(FRenderGraphDepthStencilView* View)
{
    if (!View)
    {
        return nullptr;
    }

    if (View->IsExternal())
    {
        return BorrowExternalView(View->GetExternalView());
    }

    FRenderGraphTexture* Parent = View->GetParent();
    if (!Parent || !Parent->GetRHITexture())
    {
        return nullptr;
    }

    return RHI::CreateDepthStencilView(Parent->GetRHITexture(), View->GetDesc());
}

FRHIShaderResourceView* FRenderGraphViewCache::GetOrCreate(FRenderGraphShaderResourceView* View)
{
    if (FRHIShaderResourceViewRef Cached = GetCached(View))
    {
        ++NumViewsCacheHits;
        return Cached.Get();
    }

    FRHIShaderResourceViewRef Created = CreateShaderResourceView(View);
    if (Created)
    {
        ++NumViewsCreated;
        ShaderResourceViews.Emplace(View);
        ShaderResourceViewRefs.Emplace(Created);
    }

    return Created.Get();
}

FRHIUnorderedAccessView* FRenderGraphViewCache::GetOrCreate(FRenderGraphUnorderedAccessView* View)
{
    if (FRHIUnorderedAccessViewRef Cached = GetCached(View))
    {
        ++NumViewsCacheHits;
        return Cached.Get();
    }

    FRHIUnorderedAccessViewRef Created = CreateUnorderedAccessView(View);
    if (Created)
    {
        ++NumViewsCreated;
        UnorderedAccessViews.Emplace(View);
        UnorderedAccessViewRefs.Emplace(Created);
    }

    return Created.Get();
}

FRHIRenderTargetView* FRenderGraphViewCache::GetOrCreate(FRenderGraphRenderTargetView* View)
{
    if (FRHIRenderTargetViewRef Cached = GetCached(View))
    {
        ++NumViewsCacheHits;
        return Cached.Get();
    }

    FRHIRenderTargetViewRef Created = CreateRenderTargetView(View);
    if (Created)
    {
        ++NumViewsCreated;
        RenderTargetViews.Emplace(View);
        RenderTargetViewRefs.Emplace(Created);
    }

    return Created.Get();
}

FRHIDepthStencilView* FRenderGraphViewCache::GetOrCreate(FRenderGraphDepthStencilView* View)
{
    if (FRHIDepthStencilViewRef Cached = GetCached(View))
    {
        ++NumViewsCacheHits;
        return Cached.Get();
    }

    FRHIDepthStencilViewRef Created = CreateDepthStencilView(View);
    if (Created)
    {
        ++NumViewsCreated;
        DepthStencilViews.Emplace(View);
        DepthStencilViewRefs.Emplace(Created);
    }

    return Created.Get();
}
