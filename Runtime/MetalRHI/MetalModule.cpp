#include "MetalModule.h"
#include "MetalCoreInterface.h"
#include "MetalShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CMetalModule, MetalRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalModule

FRHICoreInterface* CMetalModule::CreateInterface()
{
    return CMetalCoreInterface::CreateMetalCoreInterface();
}

IRHIShaderCompiler* CMetalModule::CreateCompiler()
{
    return dbg_new CMetalShaderCompiler();
}
