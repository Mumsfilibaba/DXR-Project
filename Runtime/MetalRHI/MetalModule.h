#pragma once
#include "MetalCore.h"

#include "RHI/RHIModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalModule

class CMetalModule final : public FRHIModule
{
public:

    CMetalModule()  = default;
    ~CMetalModule() = default;

    virtual class FRHICoreInterface*  CreateInterface() override final;
    virtual class IRHIShaderCompiler* CreateCompiler()  override final;
};
