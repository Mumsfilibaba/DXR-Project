#include "D3D12Module.h"
#include "D3D12CoreInterface.h"
#include "D3D12RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(FD3D12Module, D3D12RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Module

CRHICoreInterface* FD3D12Module::CreateInterface()
{
    return FD3D12CoreInterface::CreateD3D12Instance();
}

IRHIShaderCompiler* FD3D12Module::CreateCompiler()
{
    return GD3D12ShaderCompiler;
}
