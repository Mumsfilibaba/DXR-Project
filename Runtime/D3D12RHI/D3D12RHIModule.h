#pragma once
#include "Core/Core.h"

#include "RHI/RHIModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHIModule

class CD3D12RHIModule final : public CRHIModule
{
public:

    CD3D12RHIModule() = default;
    ~CD3D12RHIModule() = default;

     /** @brief: Creates the core RHI object */
    virtual class CRHIInstance* CreateInterface() override final;

     /** @brief: Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() override final;

};