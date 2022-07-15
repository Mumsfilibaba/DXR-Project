#pragma once
#include "Core/Core.h"

#include "RHI/RHIModule.h"

#if NULLRHI_IMPL
    #define NULLRHI_API MODULE_EXPORT
#else
    #define NULLRHI_API 
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIModule

class FNullRHIModule final : public FRHIModule
{
public:

    FNullRHIModule()  = default;
    ~FNullRHIModule() = default;

    virtual class FRHICoreInterface*  CreateInterface() override final;
    virtual class IRHIShaderCompiler* CreateCompiler() override final;
};
