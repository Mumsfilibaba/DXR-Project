#include "D3D12RHIModule.h"
#include "D3D12Instance.h"
#include "D3D12ShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CD3D12RHIModule, D3D12RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIModule

CRHIInstance* CD3D12RHIModule::CreateInterface()
{
    return CD3D12Instance::CreateInstance();
}

IRHIShaderCompiler* CD3D12RHIModule::CreateCompiler()
{
    return GD3D12ShaderCompiler;
}
