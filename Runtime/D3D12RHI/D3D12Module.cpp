#include "D3D12Module.h"
#include "D3D12Interface.h"
#include "D3D12RHIShaderCompiler.h"

IMPLEMENT_ENGINE_MODULE(FD3D12InterfaceModule, D3D12RHI);

FRHIInterface* FD3D12InterfaceModule::CreateInterface()
{
    return dbg_new FD3D12Interface();
}
