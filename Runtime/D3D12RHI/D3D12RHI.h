#pragma once
#include "Core/Core.h"

#include "RHI/RHIModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RHI

class CD3D12RHI final : public CRHIModule
{
public:

    CD3D12RHI() = default;
    ~CD3D12RHI() = default;

    virtual class CRHIInstance*       CreateInterface() override final;
    virtual class IRHIShaderCompiler* CreateCompiler()  override final;
};