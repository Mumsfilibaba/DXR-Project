#include "InterfaceRendererModule.h"
#include "InterfaceRenderer.h"

IMPLEMENT_ENGINE_MODULE(FApplicationInterfaceRendererModule, InterfaceRenderer);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FApplicationInterfaceRendererModule

IApplicationRenderer* FApplicationInterfaceRendererModule::CreateRenderer()
{
    return FInterfaceRenderer::Make();;
}
