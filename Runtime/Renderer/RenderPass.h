#pragma once

class FSceneRenderer;

class FRenderPass
{
public:
    FRenderPass(FSceneRenderer* InRenderer)
        : Renderer(InRenderer)
    {
    }

    FSceneRenderer* GetRenderer() const
    {
        return Renderer;
    }

private:
    FSceneRenderer* Renderer;
};