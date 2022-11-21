#include "MetalModule.h"
#include "MetalInterface.h"

IMPLEMENT_ENGINE_MODULE(FMetalModule, MetalRHI);

FRHIInterface* FMetalModule::CreateInterface()
{
    return new FMetalInterface();
}
