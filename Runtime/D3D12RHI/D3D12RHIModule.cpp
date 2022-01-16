#include "D3D12RHIModule.h"
#include "D3D12RHIInterface.h"
#include "D3D12RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(CD3D12RHIModule, D3D12RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

CRHIInterface* CD3D12RHIModule::CreateInterface()
{
    return CD3D12RHIInterface::Make();
}

IRHIShaderCompiler* CD3D12RHIModule::CreateCompiler()
{
    return GD3D12ShaderCompiler;
}
