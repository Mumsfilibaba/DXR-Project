#include "D3D12RHIModule.h"
#include "D3D12RHICore.h"
#include "D3D12RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE( CD3D12RHIModule, D3D12RHI );

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

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