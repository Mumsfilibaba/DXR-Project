#include "D3D12RHI.h"
#include "D3D12Instance.h"
#include "D3D12ShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CD3D12RHI, D3D12RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIModule

CRHIInstance* CD3D12RHI::CreateInterface()
{
    return CD3D12Instance::CreateInstance();
}

IRHIShaderCompiler* CD3D12RHI::CreateCompiler()
{
    return GD3D12ShaderCompiler;
}
