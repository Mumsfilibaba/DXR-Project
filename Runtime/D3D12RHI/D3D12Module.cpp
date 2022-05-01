#include "D3D12Module.h"
#include "D3D12CoreInstance.h"
#include "D3D12RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CD3D12Module, D3D12RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIModule

CRHICoreInstance* CD3D12Module::CreateInterface()
{
    return CD3D12CoreInstance::CreateD3D12Instance();
}

IRHIShaderCompiler* CD3D12Module::CreateCompiler()
{
    return GD3D12ShaderCompiler;
}
