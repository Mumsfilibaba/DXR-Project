#include "D3D12Module.h"
#include "D3D12CoreInterface.h"
#include "D3D12RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CD3D12Module, D3D12RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIModule

CRHICoreInterface* CD3D12Module::CreateInterface()
{
    return CD3D12CoreInterface::CreateD3D12Instance();
}

IRHIShaderCompiler* CD3D12Module::CreateCompiler()
{
    return GD3D12ShaderCompiler;
}