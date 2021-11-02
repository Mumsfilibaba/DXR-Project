#include "NullRHIModule.h"
#include "NullRHICore.h"
#include "NullRHIShaderCompiler.h"

extern "C"
{
    // Function for loading the NullRHI into the application
    NULLRHI_API IEngineModule* LoadEngineModule()
    {
        return dbg_new CNullRHIModule();
    }
}

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
