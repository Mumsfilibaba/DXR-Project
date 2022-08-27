#include "D3D12Module.h"
#include "D3D12Interface.h"
#include "D3D12RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(FD3D12Module, D3D12RHI);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Module

FRHIInterface* FD3D12Module::CreateInterface()
{
    return dbg_new FD3D12Interface();
}
