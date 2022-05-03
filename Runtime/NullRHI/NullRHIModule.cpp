#include "NullRHIModule.h"
#include "NullRHICoreInterface.h"
#include "NullRHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CNullRHIModule, NullRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIModule

CRHICoreInterface* CNullRHIModule::CreateInterface()
{
    return dbg_new CNullRHICoreInterface();
}

IRHIShaderCompiler* CNullRHIModule::CreateCompiler()
{
    return dbg_new CNullRHIShaderCompiler();
}