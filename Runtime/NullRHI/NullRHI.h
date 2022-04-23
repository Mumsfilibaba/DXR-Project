#pragma once
#include "Core/Core.h"

#include "RHI/RHIModule.h"

#if NULLRHI_IMPL
    #define NULLRHI_API MODULE_EXPORT
#else
    #define NULLRHI_API 
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIModule

class CNullRHIModule final : public CRHIModuleInterface
{
public:

    CNullRHIModule() = default;
    ~CNullRHIModule() = default;

    virtual class CRHIInstance*       CreateInterface() override final;
    virtual class IRHIShaderCompiler* CreateCompiler()  override final;
};
