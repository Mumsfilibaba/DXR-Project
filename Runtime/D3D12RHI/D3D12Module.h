#pragma once
#include "Core/Core.h"

#include "RHI/RHIModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Module

class CD3D12Module final : public CRHIModule
{
public:

    CD3D12Module()  = default;
    ~CD3D12Module() = default;

     /** @brief: Creates the core RHI object */
    virtual class CRHICoreInterface* CreateInterface() override final;

     /** @brief: Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() override final;

};