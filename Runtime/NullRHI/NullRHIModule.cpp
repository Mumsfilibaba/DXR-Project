#include "NullRHIModule.h"
#include "NullRHICoreInterface.h"
#include "NullRHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(FNullRHIModule, NullRHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIModule

FRHICoreInterface* FNullRHIModule::CreateInterface()
{
    return FNullRHICoreInterface::CreateNullRHICoreInterface();
}

IRHIShaderCompiler* FNullRHIModule::CreateCompiler()
{
    return dbg_new CNullRHIShaderCompiler();
}