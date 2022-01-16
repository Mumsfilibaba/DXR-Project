#include "NullRHIModule.h"
#include "NullRHIInterface.h"
#include "NullRHIShaderCompiler.h"


IMPLEMENT_ENGINE_MODULE(CNullRHIModule, NullRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

CRHIInterface* CNullRHIModule::CreateInterface()
{
    return dbg_new CNullRHIInterface();
}

IRHIShaderCompiler* CNullRHIModule::CreateCompiler()
{
    return dbg_new CNullRHIShaderCompiler();
}