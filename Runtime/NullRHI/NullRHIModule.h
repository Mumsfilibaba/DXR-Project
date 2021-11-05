#pragma once
#include "Core.h"

#if PLATFORM_WINDOWS

#if NULLRHI_API_EXPORT
#define NULLRHI_API __declspec(dllexport)
#else
#define NULLRHI_API 
#endif

#else
#define NULLRHI_API 
#endif

#include "RHI/RHIModule.h"

class CNullRHIModule final : public CRHIModule
{
public:

    CNullRHIModule() = default;
    ~CNullRHIModule() = default;

    /* Creates the core RHI object */
    virtual class CRHICore* CreateCore() override final;

    /* Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() override final;

    /* Return the NullRHI module name */
    virtual const char* GetName() const override final;
};