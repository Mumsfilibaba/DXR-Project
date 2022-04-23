#include "NullRHI.h"
#include "RHIInstanceNull.h"
#include "NullRHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CNullRHIModule, NullRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIModule

CRHIInstance* CNullRHIModule::CreateInterface()
{
    return dbg_new CRHIInstanceNull();
}

IRHIShaderCompiler* CNullRHIModule::CreateCompiler()
{
    return dbg_new CNullRHIShaderCompiler();
}
