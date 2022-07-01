#pragma once
#include "Core/Core.h"

#include "RHI/RHIModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Module

class FD3D12Module final : public FRHIModule
{
public:

    FD3D12Module()  = default;
    ~FD3D12Module() = default;

     /** @brief: Creates the core RHI object */
    virtual class FRHICoreInterface* CreateInterface() override final;

     /** @brief: Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() override final;

};