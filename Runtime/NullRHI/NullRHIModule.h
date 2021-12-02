#pragma once
#include "Core.h"

#include "RHI/RHIModule.h"

#if NULLRHI_API_EXPORT
#define NULLRHI_API MODULE_EXPORT
#else
#define NULLRHI_API 
#endif

class CNullRHIModule final : public CRHIModule
{
public:

    CNullRHIModule() = default;
    ~CNullRHIModule() = default;

    /* Creates the core RHI object */
    virtual class CRHIInterface* CreateInterface() override final;

    /* Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() override final;
};