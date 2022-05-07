#pragma once
#include "Core/Core.h"

#include "RHI/RHIModule.h"

#if METALRHI_IMPL
    #define METALRHI_API MODULE_EXPORT
#else
    #define METALRHI_API 
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalModule

class CMetalModule final : public CRHIModule
{
public:

    CMetalModule()  = default;
    ~CMetalModule() = default;

    virtual class CRHICoreInterface*  CreateInterface() override final;
    virtual class IRHIShaderCompiler* CreateCompiler()  override final;
};
