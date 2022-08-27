#include "MetalModule.h"
#include "MetalCoreInterface.h"

IMPLEMENT_ENGINE_MODULE(FMetalModule, MetalRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalModule

FRHICoreInterface* FMetalModule::CreateInterface()
{
    return FMetalCoreInterface::CreateMetalCoreInterface();
}
