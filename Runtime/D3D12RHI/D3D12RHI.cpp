#include "D3D12RHI.h"
#include "RHIInstanceD3D12.h"
#include "D3D12ShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CD3D12RHI, D3D12RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIModule

CRHIInstance* CD3D12RHI::CreateInterface()
{
    return CRHIInstanceD3D12::CreateInstance();
}

IRHIShaderCompiler* CD3D12RHI::CreateCompiler()
{
    return GD3D12ShaderCompiler;
}
