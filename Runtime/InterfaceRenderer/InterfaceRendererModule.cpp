#include "InterfaceRendererModule.h"
#include "InterfaceRenderer.h"

IMPLEMENT_ENGINE_MODULE(CInterfaceRendererModule, InterfaceRenderer);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CInterfaceRendererModule

ICanvasRenderer* CInterfaceRendererModule::CreateRenderer()
{
    return CInterfaceRenderer::Make();;
}
