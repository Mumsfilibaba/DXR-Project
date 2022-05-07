#include "NullRHIModule.h"
#include "NullRHICoreInterface.h"
#include "NullRHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CNullRHIModule, NullRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIModule

CRHICoreInterface* CNullRHIModule::CreateInterface()
{
    return CNullRHICoreInterface::CreateNullRHICoreInterface();
}

IRHIShaderCompiler* CNullRHIModule::CreateCompiler()
{
    return dbg_new CNullRHIShaderCompiler();
}