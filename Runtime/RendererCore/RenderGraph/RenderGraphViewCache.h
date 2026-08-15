#pragma once
#include "RHI/RHIResourceViews.h"
#include "RendererCore/RenderGraph/RenderGraphViews.h"

class FRenderGraphViewCache
{
public:
    FRenderGraphViewCache();
    ~FRenderGraphViewCache();

    FRHIShaderResourceView*  GetOrCreate(FRenderGraphShaderResourceView* View);
    FRHIUnorderedAccessView* GetOrCreate(FRenderGraphUnorderedAccessView* View);
    FRHIRenderTargetView*    GetOrCreate(FRenderGraphRenderTargetView* View);
    FRHIDepthStencilView*    GetOrCreate(FRenderGraphDepthStencilView* View);

    int32 GetNumViewsCreated() const
    {
        return NumViewsCreated;
    }

    int32 GetNumViewsCacheHits() const
    {
        return NumViewsCacheHits;
    }

private:
    FRHIShaderResourceView*    CreateShaderResourceView(FRenderGraphShaderResourceView* View);
    FRHIUnorderedAccessView*   CreateUnorderedAccessView(FRenderGraphUnorderedAccessView* View);
    FRHIRenderTargetView*      CreateRenderTargetView(FRenderGraphRenderTargetView* View);
    FRHIDepthStencilView*      CreateDepthStencilView(FRenderGraphDepthStencilView* View);

    FRHIShaderResourceViewRef  GetCached(FRenderGraphShaderResourceView* View) const;
    FRHIUnorderedAccessViewRef GetCached(FRenderGraphUnorderedAccessView* View) const;
    FRHIRenderTargetViewRef    GetCached(FRenderGraphRenderTargetView* View) const;
    FRHIDepthStencilViewRef    GetCached(FRenderGraphDepthStencilView* View) const;

    TArray<FRenderGraphShaderResourceView*>  ShaderResourceViews;
    TArray<FRHIShaderResourceViewRef>        ShaderResourceViewRefs;
    TArray<FRenderGraphUnorderedAccessView*> UnorderedAccessViews;
    TArray<FRHIUnorderedAccessViewRef>       UnorderedAccessViewRefs;
    TArray<FRenderGraphRenderTargetView*>    RenderTargetViews;
    TArray<FRHIRenderTargetViewRef>          RenderTargetViewRefs;
    TArray<FRenderGraphDepthStencilView*>    DepthStencilViews;
    TArray<FRHIDepthStencilViewRef>          DepthStencilViewRefs;
    int32                                    NumViewsCreated;
    int32                                    NumViewsCacheHits;
};
