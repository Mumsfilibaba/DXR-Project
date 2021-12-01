#include "NullRHIModule.h"
#include "NullRHICore.h"
#include "NullRHIShaderCompiler.h"


IMPLEMENT_ENGINE_MODULE( CNullRHIModule, NullRHI );

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

CRHICore* CNullRHIModule::CreateCore()
{
    return dbg_new CNullRHICore();
}

IRHIShaderCompiler* CNullRHIModule::CreateCompiler()
{
    return dbg_new CNullRHIShaderCompiler();
}

const char* CNullRHIModule::GetName() const
{
    return "NullRHI";
}
