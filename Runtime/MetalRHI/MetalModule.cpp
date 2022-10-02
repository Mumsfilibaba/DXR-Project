#include "MetalModule.h"
#include "MetalInterface.h"

IMPLEMENT_ENGINE_MODULE(FMetalModule, MetalRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalModule

FRHIInterface* FMetalModule::CreateInterface()
{
    return dbg_new FMetalInterface();
}
