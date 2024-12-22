#include "Renderer/RenderPass.h"

FRenderPass::FRenderPass(FSceneRenderer* InRenderer)
    : Renderer(InRenderer)
{
    CHECK(Renderer != nullptr);
}

FRenderPass::~FRenderPass()
{
    Renderer = nullptr;
}