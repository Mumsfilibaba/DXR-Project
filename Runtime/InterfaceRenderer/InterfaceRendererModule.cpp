#include "InterfaceRendererModule.h"
#include "InterfaceRenderer.h"

IMPLEMENT_ENGINE_MODULE( CInterfaceRendererModule, InterfaceRenderer );

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

IInterfaceRenderer* CInterfaceRendererModule::CreateRenderer()
{
    return CInterfaceRenderer::Make();;
}

const char* CInterfaceRendererModule::GetName() const
{
    return "InterfaceRenderer";
}
