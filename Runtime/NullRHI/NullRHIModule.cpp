#include "NullRHIModule.h"
#include "NullRHICoreInterface.h"
#include "NullRHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CNullRHIModule, NullRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIModule

CRHICoreInstance* CNullRHIModule::CreateInterface()
{
    return dbg_new CNullRHICoreInstance();
}

IRHIShaderCompiler* CNullRHIModule::CreateCompiler()
{
    return dbg_new CNullRHIShaderCompiler();
}