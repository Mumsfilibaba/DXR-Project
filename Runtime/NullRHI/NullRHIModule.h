#pragma once
#include "NullAPI.h"

#include "RHI/RHIModule.h"

class NULLRHI_API CNullRHIModule : public CRHIModule
{
public:

    CNullRHIModule() = default;
    ~CNullRHIModule() = default;

    /* Creates the core RHI object */
    virtual class CRHICore* CreateCore() override final;

    /* Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() override final;

    /* Return the NullRHI module name */
    virtual const char* GetName() override final;
};