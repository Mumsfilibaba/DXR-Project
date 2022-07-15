#include "MetalModule.h"
#include "MetalCoreInterface.h"
#include "MetalShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(FMetalModule, MetalRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalModule

FRHICoreInterface* FMetalModule::CreateInterface()
{
    return FMetalCoreInterface::CreateMetalCoreInterface();
}

IRHIShaderCompiler* FMetalModule::CreateCompiler()
{
    return dbg_new FMetalShaderCompiler();
}
