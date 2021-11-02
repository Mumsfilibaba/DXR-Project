#include "D3D12RHIModule.h"
#include "D3D12RHICore.h"
#include "D3D12RHIShaderCompiler.h"

extern "C"
{
    // Function for loading the D3D12RHI into the application
    D3D12RHI_API IEngineModule* LoadEngineModule()
    {
        return dbg_new CD3D12RHIModule();
    }
}

CRHICore* CD3D12RHIModule::CreateCore()
{
    return CD3D12RHICore::Make();
}

IRHIShaderCompiler* CD3D12RHIModule::CreateCompiler()
{
    return GD3D12ShaderCompiler;
}

const char* CD3D12RHIModule::GetName() const
{
    return "D3D12RHI";
}