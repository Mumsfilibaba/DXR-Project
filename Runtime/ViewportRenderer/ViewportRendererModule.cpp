#include "ViewportRendererModule.h"
#include "ViewportRenderer.h"

IMPLEMENT_ENGINE_MODULE(FViewportRendererModule, ViewportRenderer);

IViewportRenderer* FViewportRendererModule::CreateRenderer()
{
    return new FViewportRenderer();
}
