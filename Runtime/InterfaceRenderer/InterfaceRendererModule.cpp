#include "InterfaceRendererModule.h"
#include "InterfaceRenderer.h"

IMPLEMENT_ENGINE_MODULE(FApplicationRendererModule, InterfaceRenderer);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FApplicationRendererModule

IApplicationRenderer* FApplicationRendererModule::CreateRenderer()
{
    return FInterfaceRenderer::Make();;
}
