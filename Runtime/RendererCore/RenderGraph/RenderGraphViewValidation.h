#pragma once
#include "RendererCore/RenderGraph/RenderGraphViews.h"

class FRenderGraphBuilder;
class FRenderGraphPass;

struct RenderGraphViewValidation
{
    static bool ValidateShaderResourceView(FRenderGraphBuilder& Builder, FRenderGraphShaderResourceView* View);
    static bool ValidateUnorderedAccessView(FRenderGraphBuilder& Builder, FRenderGraphUnorderedAccessView* View);
    static bool ValidateRenderTargetView(FRenderGraphBuilder& Builder, FRenderGraphRenderTargetView* View);
    static bool ValidateDepthStencilView(FRenderGraphBuilder& Builder, FRenderGraphDepthStencilView* View);
    static bool ValidatePassViewAccesses(FRenderGraphBuilder& Builder, const FRenderGraphPass& Pass);
};
