#include "NullRHIModule.h"
#include "NullRHIInterface.h"
#include "NullRHIShaderCompiler.h"


IMPLEMENT_ENGINE_MODULE(CNullRHIModule, NullRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIModule

CRHIInstance* CNullRHIModule::CreateInterface()
{
    return dbg_new CNullRHIInstance();
}

IRHIShaderCompiler* CNullRHIModule::CreateCompiler()
{
    return dbg_new CNullRHIShaderCompiler();
}